#ifndef ARCH_H
#define ARCH_H

#define IRQ_COUNT 16
#define ARCH_STACK_SIZE 16384

static inline unsigned int irq_save(void) {
	unsigned long flags;

	asm volatile("pushfq; popq %0; cli" : "=r"(flags) : : "memory");
	return (unsigned int)flags;
}

static inline void irq_restore(unsigned int flags) {
	asm volatile("pushq %0; popfq" : : "r"((unsigned long)flags) : "memory", "cc");
}

static inline void irq_enable(void) {
	asm volatile("sti" : : : "memory");
}

static inline void cpu_idle(void) {
	asm volatile("hlt");
}

static inline void cpu_stop(void) {
	asm volatile("cli");

	while (1) {
		asm volatile("hlt");
	}
}

static inline void memory_barrier(void) {
	asm volatile("mfence" : : : "memory");
}

static inline void context_yield(void) {
	asm volatile("int $0x81" : : : "memory");
}

static inline unsigned long user_syscall(unsigned long number, unsigned long argument) {
	unsigned long result;

	asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(argument) : "memory");
	return result;
}

void cpu_init(void);
void interrupts_init(void);
void timer_init(unsigned int hz);
unsigned long context_init(unsigned char *stack, unsigned int size, void (*entry)(void));

#endif
