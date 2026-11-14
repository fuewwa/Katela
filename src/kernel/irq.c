#include "../../include/irq.h"

struct irq_slot {
	void (*handler)(void *context);
	void *context;
};

static struct irq_slot slots[IRQ_COUNT][IRQ_HANDLERS_MAX];

int irq_register(unsigned int irq, void (*handler)(void *context), void *context)
{
	unsigned int flags;
	unsigned int i;
	int free_index = -1;

	if (!irq_valid(irq) || handler == 0) {
		return -1;
	}

	flags = irq_save();

	for (i = 0; i < IRQ_HANDLERS_MAX; i++) {
		struct irq_slot *slot = &slots[irq][i];

		if (slot->handler == handler && slot->context == context) {
			irq_restore(flags);
			return -1;
		}

		if (slot->handler == 0 && free_index < 0) {
			free_index = (int)i;
		}
	}

	if (free_index < 0) {
		irq_restore(flags);
		return -1;
	}

	slots[irq][free_index].handler = handler;
	slots[irq][free_index].context = context;
	irq_unmask(irq);

	irq_restore(flags);
	return 0;
}

void irq_unregister(unsigned int irq, void (*handler)(void *context), void *context)
{
	unsigned int flags;
	unsigned int i;
	int remaining = 0;

	if (!irq_valid(irq)) {
		return;
	}

	flags = irq_save();

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
		irq_mask(irq);
	}

	irq_restore(flags);
}

void irq_dispatch(unsigned int irq)
{
	unsigned int i;

	if (irq >= IRQ_COUNT) {
		return;
	}

	if (irq_spurious(irq)) {
		return;
	}

	for (i = 0; i < IRQ_HANDLERS_MAX; i++) {
		struct irq_slot *slot = &slots[irq][i];

		if (slot->handler != 0) {
			slot->handler(slot->context);
		}
	}

	irq_eoi(irq);
}
