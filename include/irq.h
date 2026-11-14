#ifndef IRQ_H
#define IRQ_H

#include "arch.h"

#define IRQ_HANDLERS_MAX 4

int irq_register(unsigned int irq, void (*handler)(void *context), void *context);
void irq_unregister(unsigned int irq, void (*handler)(void *context), void *context);
void irq_dispatch(unsigned int irq);

void irq_mask(unsigned int irq);
void irq_unmask(unsigned int irq);
void irq_eoi(unsigned int irq);
int irq_spurious(unsigned int irq);
int irq_valid(unsigned int irq);

#endif
