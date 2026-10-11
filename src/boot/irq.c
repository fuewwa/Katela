#include "irq.h"
#include "pic.h"

struct irq_slot {
	void (*handler)(void *context);
	void *context;
};

static struct irq_slot slots[IRQ_COUNT][IRQ_HANDLERS_MAX];

static inline unsigned int save_flags(void)
{
	unsigned int flags;

	asm volatile("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
	return flags;
}

static inline void restore_flags(unsigned int flags)
{
	asm volatile("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
}

int irq_register(unsigned int irq, void (*handler)(void *context), void *context)
{
	unsigned int flags;
	unsigned int i;
	int free_index = -1;

	if (irq == 0 || irq == 2 || irq >= IRQ_COUNT || handler == 0) {
		return -1;
	}

	flags = save_flags();

	for (i = 0; i < IRQ_HANDLERS_MAX; i++) {
		struct irq_slot *slot = &slots[irq][i];

		if (slot->handler == handler && slot->context == context) {
			restore_flags(flags);
			return -1;
		}

		if (slot->handler == 0 && free_index < 0) {
			free_index = (int)i;
		}
	}

	if (free_index < 0) {
		restore_flags(flags);
		return -1;
	}

	slots[irq][free_index].handler = handler;
	slots[irq][free_index].context = context;
	pic_unmask(irq);

	restore_flags(flags);
	return 0;
}

void irq_unregister(unsigned int irq, void (*handler)(void *context), void *context)
{
	unsigned int flags;
	unsigned int i;
	int remaining = 0;

	if (irq == 0 || irq == 2 || irq >= IRQ_COUNT) {
		return;
	}

	flags = save_flags();

	for (i = 0; i < IRQ_HANDLERS_MAX; i++) {
		struct irq_slot *slot = &slots[irq][i];

		if (slot->handler == handler && slot->context == context) {
			slot->handler = 0;
			slot->context = 0;
		}

		if (slot->handler != 0) {
			remaining = 1;
		}
	}

	if (!remaining) {
		pic_mask(irq);
	}

	restore_flags(flags);
}

void irq_dispatch(unsigned int irq)
{
	unsigned int i;

	if (irq >= IRQ_COUNT) {
		return;
	}

	if (pic_spurious(irq)) {
		return;
	}

	for (i = 0; i < IRQ_HANDLERS_MAX; i++) {
		struct irq_slot *slot = &slots[irq][i];

		if (slot->handler != 0) {
			slot->handler(slot->context);
		}
	}

	pic_eoi(irq);
}
