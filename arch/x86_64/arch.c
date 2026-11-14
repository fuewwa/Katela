#include "arch.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "../../include/panic.h"
#include "../../include/syscall.h"

extern void task_trampoline(void);

struct regs {
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long rdi;
	unsigned long rsi;
	unsigned long rbp;
	unsigned long rbx;
	unsigned long rdx;
	unsigned long rcx;
	unsigned long rax;
};

struct frame {
	struct regs regs;
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
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
	unsigned long top = ((unsigned long)(stack + size)) & ~15ul;
	struct frame *frame = (struct frame *)(top - sizeof(struct frame));
	unsigned char *raw = (unsigned char *)frame;
	unsigned int i;

	for (i = 0; i < sizeof(struct frame); i++) {
		raw[i] = 0;
	}

	frame->regs.rdi = (unsigned long)entry;
	frame->rip = (unsigned long)task_trampoline;
	frame->cs = KERNEL_CODE_SEL;
	frame->rflags = 0x202;
	frame->rsp = top;
	frame->ss = KERNEL_DATA_SEL;

	return (unsigned long)frame;
}

void syscall_entry(struct regs *r) {
	r->rax = syscall_dispatch(r->rax, r->rbx);
}

void gpf_handler(void) {
	panic("general protection fault");
}

void double_fault_handler(void) {
	panic("double fault");
}

void page_fault_handler(void) {
	panic("page fault");
}
