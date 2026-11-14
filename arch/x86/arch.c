#include "arch.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "../../include/panic.h"
#include "../../include/syscall.h"

extern void task_trampoline(void);

struct regs {
	unsigned int edi;
	unsigned int esi;
	unsigned int ebp;
	unsigned int esp_dummy;
	unsigned int ebx;
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;
};

void cpu_init(void) {
	gdt_init();
}

void interrupts_init(void) {
	idt_init();
	pic_remap();
}

void timer_init(unsigned int hz) {
	pit_init(hz);
}

unsigned long context_init(unsigned char *stack, unsigned int size, void (*entry)(void)) {
	unsigned short selector;
	unsigned int *sp;

	asm volatile("mov %%cs, %0" : "=r"(selector));

	sp = (unsigned int *)(((unsigned int)(stack + size)) & ~15u);

	*(--sp) = 0x202;
	*(--sp) = selector;
	*(--sp) = (unsigned int)task_trampoline;

	*(--sp) = 0;
	*(--sp) = 0;
	*(--sp) = 0;
	*(--sp) = 0;
	*(--sp) = 0;
	*(--sp) = 0;
	*(--sp) = 0;
	*(--sp) = (unsigned int)entry;

	return (unsigned long)sp;
}

void syscall_entry(struct regs *r) {
	r->eax = syscall_dispatch(r->eax, r->ebx);
}

void gpf_handler(void) {
	panic("general protection fault");
}
