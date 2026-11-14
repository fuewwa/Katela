#ifndef ARCH_H
#define ARCH_H

#define IRQ_COUNT 16
#define ARCH_STACK_SIZE 8192

static inline unsigned int irq_save(void) {
	unsigned int flags;

	asm volatile("pushfl; popl %0; cli" : "=r"(flags) : : "memory");
	return flags;
}

static inline void irq_restore(unsigned int flags) {
	asm volatile("pushl %0; popfl" : : "r"(flags) : "memory", "cc");
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
	asm volatile("lock; addl $0, (%%esp)" : : : "memory", "cc");
}

static inline void context_yield(void) {
	asm volatile("int $0x81" : : : "memory");
}

static inline unsigned long user_syscall(unsigned long number, unsigned long argument) {
	unsigned long result;

	asm volatile("int $0x80" : "=a"(result) : "a"(number), "b"(argument));
	return result;
}

void cpu_init(void);
void interrupts_init(void);
void timer_init(unsigned int hz);
unsigned long context_init(unsigned char *stack, unsigned int size, void (*entry)(void));

#endif
