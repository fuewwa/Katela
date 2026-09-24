#include "nvme.h"
#include "pci.h"
#include "../../include/mem.h"

#define REG_CAP_LOW     0x00
#define REG_CAP_HIGH    0x04
#define REG_INTMS       0x0C
#define REG_CC          0x14
#define REG_CSTS        0x1C
#define REG_AQA         0x24
#define REG_ASQ_LOW     0x28
#define REG_ASQ_HIGH    0x2C
#define REG_ACQ_LOW     0x30
#define REG_ACQ_HIGH    0x34
#define REG_DOORBELL    0x1000

#define CAP_HIGH_CSS_NVM    (1u << 5)

#define CC_ENABLE   (1u << 0)
#define CC_IOSQES   (6u << 16)
#define CC_IOCQES   (4u << 20)

#define CSTS_READY  (1u << 0)
#define CSTS_FATAL  (1u << 1)

#define ADMIN_CREATE_SQ 0x01
#define ADMIN_CREATE_CQ 0x05
#define ADMIN_IDENTIFY  0x06

#define IO_FLUSH    0x00
#define IO_WRITE    0x01
#define IO_READ     0x02

#define IDENTIFY_NAMESPACE  0
#define IDENTIFY_CONTROLLER 1

#define PAGE_SIZE       4096u
#define QUEUE_ENTRIES   64u
#define BOUNCE_SIZE     65536u
#define NAMESPACE_ID    1u
#define IO_QUEUE_ID     1u
#define MAX_SECTORS     0x0FFFFFFFu
#define COMPLETION_TIMEOUT 200000000u
#define TIMEOUT_UNIT_LOOPS 500000u

#define STATE_UNTRIED   0
#define STATE_READY     1
#define STATE_FAILED    2

struct nvme_command {
    unsigned int cdw0;
    unsigned int nsid;
    unsigned int reserved[2];
    unsigned int metadata[2];
    unsigned int prp1[2];
    unsigned int prp2[2];
    unsigned int cdw10;
    unsigned int cdw11;
    unsigned int cdw12;
    unsigned int cdw13;
    unsigned int cdw14;
    unsigned int cdw15;
};

struct nvme_completion {
    unsigned int result;
    unsigned int reserved;
    unsigned short sq_head;
    unsigned short sq_id;
    unsigned short command_id;
    unsigned short status;
};

struct queue {
    struct nvme_command *sq;
    volatile struct nvme_completion *cq;
    unsigned int sq_doorbell;
    unsigned int cq_doorbell;
    unsigned int sq_tail;
    unsigned int cq_head;
    unsigned int phase;
};

static unsigned char admin_sq[PAGE_SIZE] __attribute__((aligned(4096)));
static unsigned char admin_cq[PAGE_SIZE] __attribute__((aligned(4096)));
static unsigned char io_sq[PAGE_SIZE] __attribute__((aligned(4096)));
static unsigned char io_cq[PAGE_SIZE] __attribute__((aligned(4096)));
static unsigned char identify_buf[PAGE_SIZE] __attribute__((aligned(4096)));
static unsigned int prp_list[PAGE_SIZE / 4] __attribute__((aligned(4096)));
static unsigned char bounce[BOUNCE_SIZE] __attribute__((aligned(4096)));

static struct queue admin;
static struct queue io;

static int state = STATE_UNTRIED;
static unsigned int regs_base;
static unsigned int doorbell_stride;
static unsigned int ready_timeout;
static unsigned int command_id;
static unsigned int lba_shift;
static unsigned int max_transfer;
static unsigned int total_sectors;

static inline void barrier(void) {
    asm volatile("lock; addl $0, (%%esp)" : : : "memory", "cc");
}

static unsigned int reg_read(unsigned int offset) {
    return *(volatile unsigned int *)(regs_base + offset);
}

static void reg_write(unsigned int offset, unsigned int value) {
    *(volatile unsigned int *)(regs_base + offset) = value;
}

static unsigned int load_u32(unsigned int offset) {
    return (unsigned int)identify_buf[offset]
        | ((unsigned int)identify_buf[offset + 1] << 8)
        | ((unsigned int)identify_buf[offset + 2] << 16)
        | ((unsigned int)identify_buf[offset + 3] << 24);
}

static unsigned int doorbell(unsigned int queue_id, unsigned int completion) {
    return REG_DOORBELL + (queue_id * 2 + completion) * doorbell_stride;
}

static void queue_setup(struct queue *q, void *sq, void *cq, unsigned int queue_id) {
    memset(sq, 0, PAGE_SIZE);
    memset(cq, 0, PAGE_SIZE);

    q->sq = (struct nvme_command *)sq;
    q->cq = (volatile struct nvme_completion *)cq;
    q->sq_doorbell = doorbell(queue_id, 0);
    q->cq_doorbell = doorbell(queue_id, 1);
    q->sq_tail = 0;
    q->cq_head = 0;
    q->phase = 1;
}

static int wait_ready(unsigned int expected) {
    unsigned int timeout = ready_timeout;

    while (timeout-- != 0) {
        unsigned int status = reg_read(REG_CSTS);

        if (expected != 0 && (status & CSTS_FATAL)) {
            return -1;
        }

        if ((status & CSTS_READY) == expected) {
            return 0;
        }
    }

    return -1;
}

static int execute(struct queue *q, struct nvme_command *cmd) {
    volatile struct nvme_completion *entry;
    unsigned int timeout = COMPLETION_TIMEOUT;
    unsigned int id = command_id++ & 0xFFFFu;
    unsigned short status;
    unsigned short completed_id;

    cmd->cdw0 = (cmd->cdw0 & 0xFFFFu) | (id << 16);

    memcpy(&q->sq[q->sq_tail], cmd, sizeof(*cmd));
    q->sq_tail = (q->sq_tail + 1) & (QUEUE_ENTRIES - 1);

    barrier();
    reg_write(q->sq_doorbell, q->sq_tail);

    entry = &q->cq[q->cq_head];

    while ((unsigned int)(entry->status & 1u) != q->phase) {
        if (--timeout == 0) {
            state = STATE_FAILED;
            return -1;
        }
    }

    barrier();
    status = entry->status;
    completed_id = entry->command_id;

    q->cq_head = (q->cq_head + 1) & (QUEUE_ENTRIES - 1);

    if (q->cq_head == 0) {
        q->phase ^= 1u;
    }

    reg_write(q->cq_doorbell, q->cq_head);

    if (completed_id != id || (status >> 1) != 0) {
        return -1;
    }

    return 0;
}

static int identify(unsigned int cns, unsigned int nsid) {
    struct nvme_command cmd;

    memset(&cmd, 0, sizeof(cmd));
    memset(identify_buf, 0, PAGE_SIZE);

    cmd.cdw0 = ADMIN_IDENTIFY;
    cmd.nsid = nsid;
    cmd.prp1[0] = (unsigned int)identify_buf;
    cmd.cdw10 = cns;

    return execute(&admin, &cmd);
}

static int create_io_queues(void) {
    struct nvme_command cmd;

    queue_setup(&io, io_sq, io_cq, IO_QUEUE_ID);

    memset(&cmd, 0, sizeof(cmd));
    cmd.cdw0 = ADMIN_CREATE_CQ;
    cmd.prp1[0] = (unsigned int)io_cq;
    cmd.cdw10 = ((QUEUE_ENTRIES - 1) << 16) | IO_QUEUE_ID;
    cmd.cdw11 = 1u;

    if (execute(&admin, &cmd) != 0) {
        return -1;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.cdw0 = ADMIN_CREATE_SQ;
    cmd.prp1[0] = (unsigned int)io_sq;
    cmd.cdw10 = ((QUEUE_ENTRIES - 1) << 16) | IO_QUEUE_ID;
    cmd.cdw11 = (IO_QUEUE_ID << 16) | 1u;

    return execute(&admin, &cmd);
}

static int read_namespace(void) {
    unsigned int nsze_low;
    unsigned int nsze_high;
    unsigned int flbas;
    unsigned int format;
    unsigned int lbads;
    unsigned int diff;

    if (identify(IDENTIFY_NAMESPACE, NAMESPACE_ID) != 0) {
        return -1;
    }

    nsze_low = load_u32(0);
    nsze_high = load_u32(4);
    flbas = identify_buf[26];
    format = flbas & 0x0Fu;

    if (identify_buf[25] > 15) {
        format |= ((flbas >> 5) & 3u) << 4;
    }

    if ((flbas & 0x10u) != 0 || (nsze_low == 0 && nsze_high == 0)) {
        return -1;
    }

    lbads = (load_u32(128 + format * 4) >> 16) & 0xFFu;

    if (lbads < 9 || lbads > 12) {
        return -1;
    }

    lba_shift = lbads;
    diff = lba_shift - 9;

    if (nsze_high != 0 || nsze_low > (MAX_SECTORS >> diff)) {
        total_sectors = MAX_SECTORS;
    } else {
        total_sectors = nsze_low << diff;
    }

    return 0;
}

static int controller_init(void) {
    struct pci_device dev;
    unsigned int bar0;
    unsigned int cap_low;
    unsigned int cap_high;
    unsigned int mdts;

    if (pci_find_class(0x01, 0x08, 0x02, &dev) != 0) {
        return -1;
    }

    bar0 = pci_read32(&dev, PCI_REG_BAR0);

    if ((bar0 & 1u) != 0) {
        return -1;
    }

    if (((bar0 >> 1) & 3u) == 2u && pci_read32(&dev, PCI_REG_BAR1) != 0) {
        return -1;
    }

    regs_base = bar0 & 0xFFFFFFF0u;

    if (regs_base == 0) {
        return -1;
    }

    pci_enable(&dev, PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER | PCI_COMMAND_NO_INTX);

    cap_low = reg_read(REG_CAP_LOW);
    cap_high = reg_read(REG_CAP_HIGH);

    if ((cap_low & 0xFFFFu) + 1 < QUEUE_ENTRIES) {
        return -1;
    }

    if ((cap_high & CAP_HIGH_CSS_NVM) == 0 || ((cap_high >> 16) & 0x0Fu) != 0) {
        return -1;
    }

    doorbell_stride = 4u << (cap_high & 0x0Fu);
    ready_timeout = (((cap_low >> 24) & 0xFFu) + 1) * TIMEOUT_UNIT_LOOPS;

    reg_write(REG_CC, 0);

    if (wait_ready(0) != 0) {
        return -1;
    }

    reg_write(REG_INTMS, 0xFFFFFFFFu);

    queue_setup(&admin, admin_sq, admin_cq, 0);

    reg_write(REG_AQA, ((QUEUE_ENTRIES - 1) << 16) | (QUEUE_ENTRIES - 1));
    reg_write(REG_ASQ_LOW, (unsigned int)admin_sq);
    reg_write(REG_ASQ_HIGH, 0);
    reg_write(REG_ACQ_LOW, (unsigned int)admin_cq);
    reg_write(REG_ACQ_HIGH, 0);

    reg_write(REG_CC, CC_ENABLE | CC_IOSQES | CC_IOCQES);

    if (wait_ready(1) != 0) {
        return -1;
    }

    if (identify(IDENTIFY_CONTROLLER, 0) != 0) {
        return -1;
    }

    mdts = identify_buf[77];
    max_transfer = BOUNCE_SIZE;

    if (mdts != 0 && mdts < 4) {
        max_transfer = PAGE_SIZE << mdts;
    }

    if (read_namespace() != 0) {
        return -1;
    }

    return create_io_queues();
}

static int run_io(unsigned int opcode, unsigned int block, unsigned int blocks) {
    struct nvme_command cmd;
    unsigned int pages = ((blocks << lba_shift) + PAGE_SIZE - 1) / PAGE_SIZE;
    unsigned int i;

    memset(&cmd, 0, sizeof(cmd));

    cmd.cdw0 = opcode;
    cmd.nsid = NAMESPACE_ID;
    cmd.prp1[0] = (unsigned int)bounce;

    if (pages == 2) {
        cmd.prp2[0] = (unsigned int)bounce + PAGE_SIZE;
    } else if (pages > 2) {
        for (i = 1; i < pages; i++) {
            prp_list[(i - 1) * 2] = (unsigned int)bounce + i * PAGE_SIZE;
            prp_list[(i - 1) * 2 + 1] = 0;
        }

        cmd.prp2[0] = (unsigned int)prp_list;
    }

    cmd.cdw10 = block;
    cmd.cdw11 = 0;
    cmd.cdw12 = blocks - 1;

    return execute(&io, &cmd);
}

static int flush(void) {
    struct nvme_command cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.cdw0 = IO_FLUSH;
    cmd.nsid = NAMESPACE_ID;

    return execute(&io, &cmd);
}

static int transfer(int write, unsigned int lba, unsigned int count, unsigned char *buffer) {
    unsigned int diff = lba_shift - 9;
    unsigned int per_block = 1u << diff;
    unsigned int max_blocks = max_transfer >> lba_shift;

    if (count == 0 || lba >= total_sectors || count > total_sectors - lba) {
        return -1;
    }

    while (count > 0) {
        unsigned int block = lba >> diff;
        unsigned int offset = lba & (per_block - 1);
        unsigned int blocks = (offset + count + per_block - 1) >> diff;
        unsigned int span;
        int partial;

        if (blocks > max_blocks) {
            blocks = max_blocks;
        }

        span = (blocks << diff) - offset;

        if (span > count) {
            span = count;
        }

        partial = offset != 0 || span != (blocks << diff);

        if (write) {
            if (partial && run_io(IO_READ, block, blocks) != 0) {
                return -1;
            }

            memcpy(bounce + (offset << 9), buffer, span << 9);

            if (run_io(IO_WRITE, block, blocks) != 0) {
                return -1;
            }
        } else {
            if (run_io(IO_READ, block, blocks) != 0) {
                return -1;
            }

            memcpy(buffer, bounce + (offset << 9), span << 9);
        }

        lba += span;
        count -= span;
        buffer += span << 9;
    }

    return 0;
}

unsigned int nvme_total_sectors(void) {
    if (state == STATE_UNTRIED) {
        state = controller_init() == 0 ? STATE_READY : STATE_FAILED;
    }

    return state == STATE_READY ? total_sectors : 0;
}

int nvme_read_sectors(unsigned int lba, unsigned int count, unsigned char *buffer) {
    if (nvme_total_sectors() == 0) {
        return -1;
    }

    return transfer(0, lba, count, buffer);
}

int nvme_write_sectors(unsigned int lba, unsigned int count, const unsigned char *buffer) {
    if (nvme_total_sectors() == 0) {
        return -1;
    }

    if (transfer(1, lba, count, (unsigned char *)buffer) != 0) {
        return -1;
    }

    return flush();
}

int nvme_read_sector(unsigned int lba, unsigned char *buffer) {
    return nvme_read_sectors(lba, 1, buffer);
}

int nvme_write_sector(unsigned int lba, const unsigned char *buffer) {
    return nvme_write_sectors(lba, 1, buffer);
}
