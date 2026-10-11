#ifndef IRQ_H
#define IRQ_H

#define IRQ_COUNT 16
#define IRQ_HANDLERS_MAX 4

int irq_register(unsigned int irq, void (*handler)(void *context), void *context);
void irq_unregister(unsigned int irq, void (*handler)(void *context), void *context);
void irq_dispatch(unsigned int irq);

#endif
