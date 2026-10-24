#ifndef IRQ_H
#define IRQ_H

#define IRQ_COUNT 16
#define IRQ_HANDLERS_MAX 4

static inline unsigned int irq_save(void) {
    unsigned int flags;

    asm volatile("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_restore(unsigned int flags) {
    asm volatile("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
}

int irq_register(unsigned int irq, void (*handler)(void *context), void *context);
void irq_unregister(unsigned int irq, void (*handler)(void *context), void *context);
void irq_dispatch(unsigned int irq);

#endif
