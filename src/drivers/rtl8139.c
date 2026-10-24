#include "rtl8139.h"
#include "pci.h"
#include "../../include/io.h"
#include "../../include/irq.h"
#include "../../include/scheduler.h"

#define VENDOR_REALTEK 0x10EC
#define DEVICE_RTL8139 0x8139

#define REG_IDR0 0x00
#define REG_TSD0 0x10
#define REG_TSAD0 0x20
#define REG_RBSTART 0x30
#define REG_CR 0x37
#define REG_CAPR 0x38
#define REG_IMR 0x3C
#define REG_ISR 0x3E
#define REG_TCR 0x40
#define REG_RCR 0x44
#define REG_CONFIG1 0x52

#define CR_BUFE 0x01
#define CR_TE 0x04
#define CR_RE 0x08
#define CR_RST 0x10

#define INT_ROK 0x0001
#define INT_RER 0x0002
#define INT_TOK 0x0004
#define INT_TER 0x0008
#define INT_RXOVW 0x0010
#define INT_FOVW 0x0040
#define INT_ENABLED (INT_ROK | INT_RER | INT_TOK | INT_TER | INT_RXOVW | INT_FOVW)
#define INT_RX_EVENTS (INT_ROK | INT_RER | INT_RXOVW | INT_FOVW)

#define TSD_OWN 0x2000

#define RCR_APM 0x00000002
#define RCR_AM 0x00000004
#define RCR_AB 0x00000008
#define RCR_WRAP 0x00000080
#define RCR_MXDMA_UNLIMITED 0x00000700
#define RCR_NO_THRESHOLD 0x0000E000
#define RCR_VALUE (RCR_APM | RCR_AM | RCR_AB | RCR_WRAP | RCR_MXDMA_UNLIMITED | RCR_NO_THRESHOLD)

#define TCR_IFG_NORMAL 0x03000000
#define TCR_MXDMA_UNLIMITED 0x00000700
#define TCR_VALUE (TCR_IFG_NORMAL | TCR_MXDMA_UNLIMITED)

#define RX_STATUS_OK 0x0001
#define RX_LENGTH_PENDING 0xFFF0
#define RX_FRAME_MIN 64
#define RX_FRAME_MAX 1522
#define RX_HEADER_SIZE 4
#define RX_CRC_SIZE 4

#define RX_RING_SIZE 8192
#define RX_BUFFER_SIZE (RX_RING_SIZE + 16 + 1536)

#define TX_SLOTS 4
#define TX_BUFFER_SIZE 1792
#define TX_FRAME_MIN 60
#define TX_FRAME_MAX 1514
#define TX_WAIT_MS 100

#define QUEUE_SLOTS 16
#define QUEUE_FRAME_SIZE 1536

#define RESET_WAIT_MS 100

struct queued_frame {
	unsigned int size;
	unsigned char data[QUEUE_FRAME_SIZE];
};

static volatile unsigned char rx_ring[RX_BUFFER_SIZE] __attribute__((aligned(4)));
static unsigned char tx_buffers[TX_SLOTS][TX_BUFFER_SIZE] __attribute__((aligned(4)));
static struct queued_frame queue[QUEUE_SLOTS];

static volatile unsigned int queue_head;
static volatile unsigned int queue_tail;
static unsigned int rx_offset;
static unsigned int tx_next;
static unsigned short io_base;
static volatile int active;
static struct mac_addr mac;
static struct wait_queue rx_wait;
static struct mutex tx_lock;

static unsigned short ring_read16(unsigned int offset)
{
	return (unsigned short)(rx_ring[offset] | (rx_ring[offset + 1] << 8));
}

static void rx_start(void)
{
	rx_offset = 0;
	outb(io_base + REG_CR, CR_TE | CR_RE);
	outl(io_base + REG_RCR, RCR_VALUE);
	outl(io_base + REG_TCR, TCR_VALUE);
	outl(io_base + REG_RBSTART, (unsigned int)rx_ring);
	outw(io_base + REG_CAPR, (unsigned short)(rx_offset - 16));
}

static void rx_restart(void)
{
	outb(io_base + REG_CR, CR_TE);
	rx_start();
}

static void enqueue(unsigned int offset, unsigned int size)
{
	unsigned int next = (queue_head + 1) % QUEUE_SLOTS;
	struct queued_frame *slot;
	unsigned int i;

	if (next == queue_tail) {
		return;
	}

	slot = &queue[queue_head];

	for (i = 0; i < size; i++) {
		slot->data[i] = rx_ring[offset + i];
	}

	slot->size = size;
	queue_head = next;
}

static void drain(void)
{
	while (!(inb(io_base + REG_CR) & CR_BUFE)) {
		unsigned short status = ring_read16(rx_offset);
		unsigned short length = ring_read16(rx_offset + 2);

		if (length == RX_LENGTH_PENDING) {
			return;
		}

		if (!(status & RX_STATUS_OK) || length < RX_FRAME_MIN || length > RX_FRAME_MAX) {
			rx_restart();
			return;
		}

		enqueue(rx_offset + RX_HEADER_SIZE, (unsigned int)length - RX_CRC_SIZE);

		rx_offset = (rx_offset + length + RX_HEADER_SIZE + 3) & ~3u;
		if (rx_offset >= RX_RING_SIZE) {
			rx_offset -= RX_RING_SIZE;
		}

		outw(io_base + REG_CAPR, (unsigned short)(rx_offset - 16));
	}
}

static void interrupt_handler(void *context)
{
	unsigned short status;

	(void)context;

	if (!active) {
		return;
	}

	status = inw(io_base + REG_ISR);
	if (status == 0) {
		return;
	}

	outw(io_base + REG_ISR, status);

	if (status & INT_RX_EVENTS) {
		drain();
	}

	if (queue_head != queue_tail) {
		wait_queue_wake_all(&rx_wait);
	}
}

static int has_frame(void *arg)
{
	(void)arg;
	return queue_head != queue_tail;
}

static int wait_reset(void)
{
	unsigned int waited;

	for (waited = 0; waited < RESET_WAIT_MS; waited++) {
		if (!(inb(io_base + REG_CR) & CR_RST)) {
			return 0;
		}
		task_sleep(1);
	}

	return -1;
}

int rtl8139_init(void)
{
	struct pci_device dev;
	unsigned int bar;
	unsigned int line;
	unsigned int i;

	if (active) {
		return RTL8139_OK;
	}

	if (pci_find_device(VENDOR_REALTEK, DEVICE_RTL8139, &dev) != 0) {
		return RTL8139_ERR_NOT_FOUND;
	}

	bar = pci_read32(&dev, PCI_REG_BAR0);
	if (!(bar & 1u)) {
		return RTL8139_ERR_BAR;
	}

	line = pci_read32(&dev, PCI_REG_INTERRUPT) & 0xFFu;
	if (line == 0 || line >= IRQ_COUNT) {
		return RTL8139_ERR_IRQ;
	}

	io_base = (unsigned short)(bar & ~3u);
	pci_enable(&dev, PCI_COMMAND_IO | PCI_COMMAND_MASTER);

	outb(io_base + REG_CONFIG1, 0x00);
	outb(io_base + REG_CR, CR_RST);

	if (wait_reset() != 0) {
		return RTL8139_ERR_RESET;
	}

	for (i = 0; i < MAC_SIZE; i++) {
		mac.bytes[i] = inb(io_base + REG_IDR0 + i);
	}

	wait_queue_init(&rx_wait);
	mutex_init(&tx_lock);
	queue_head = 0;
	queue_tail = 0;
	tx_next = 0;

	rx_start();

	if (irq_register(line, interrupt_handler, 0) != 0) {
		outb(io_base + REG_CR, 0);
		return RTL8139_ERR_IRQ;
	}

	active = 1;
	outw(io_base + REG_IMR, INT_ENABLED);

	return RTL8139_OK;
}

int rtl8139_ready(void)
{
	return active;
}

const struct mac_addr *rtl8139_mac(void)
{
	return &mac;
}

int rtl8139_send(const void *frame, unsigned int size)
{
	const unsigned char *bytes = (const unsigned char *)frame;
	unsigned int slot;
	unsigned int length;
	unsigned int waited = 0;
	unsigned int i;

	if (!active || size == 0 || size > TX_FRAME_MAX) {
		return -1;
	}

	mutex_lock(&tx_lock);

	slot = tx_next;

	while (!(inl(io_base + REG_TSD0 + slot * 4) & TSD_OWN)) {
		if (++waited > TX_WAIT_MS) {
			mutex_unlock(&tx_lock);
			return -1;
		}
		task_sleep(1);
	}

	for (i = 0; i < size; i++) {
		tx_buffers[slot][i] = bytes[i];
	}

	length = size;
	while (length < TX_FRAME_MIN) {
		tx_buffers[slot][length++] = 0;
	}

	outl(io_base + REG_TSAD0 + slot * 4, (unsigned int)tx_buffers[slot]);
	outl(io_base + REG_TSD0 + slot * 4, length);

	tx_next = (slot + 1) % TX_SLOTS;

	mutex_unlock(&tx_lock);
	return (int)size;
}

int rtl8139_receive(void *buffer, unsigned int capacity, unsigned int timeout_ms)
{
	unsigned char *out = (unsigned char *)buffer;
	struct queued_frame *slot;
	unsigned int size;
	unsigned int i;

	if (!active) {
		return -1;
	}

	if (wait_queue_wait_until(&rx_wait, has_frame, 0, timeout_ms) != WAIT_OK) {
		return -1;
	}

	slot = &queue[queue_tail];
	size = slot->size;

	if (size > capacity) {
		size = capacity;
	}

	for (i = 0; i < size; i++) {
		out[i] = slot->data[i];
	}

	queue_tail = (queue_tail + 1) % QUEUE_SLOTS;

	return (int)size;
}
