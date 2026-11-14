#include "idt.h"
#include "gdt.h"

extern void isr_timer(void);
extern void isr_yield(void);
extern void isr_ignore(void);
extern void isr_syscall(void);
extern void isr_gpf(void);
extern void isr_double_fault(void);
extern void isr_page_fault(void);
extern void isr_irq1(void);
extern void isr_irq2(void);
extern void isr_irq3(void);
extern void isr_irq4(void);
extern void isr_irq5(void);
extern void isr_irq6(void);
extern void isr_irq7(void);
extern void isr_irq8(void);
extern void isr_irq9(void);
extern void isr_irq10(void);
extern void isr_irq11(void);
extern void isr_irq12(void);
extern void isr_irq13(void);
extern void isr_irq14(void);
extern void isr_irq15(void);

struct idt_entry {
	unsigned short offset_low;
	unsigned short selector;
	unsigned char ist;
	unsigned char flags;
	unsigned short offset_middle;
	unsigned int offset_high;
	unsigned int zero;
} __attribute__((packed));

struct idt_ptr {
	unsigned short limit;
	unsigned long base;
} __attribute__((packed));

static void (*const irq_entries[15])(void) = {
	isr_irq1, isr_irq2, isr_irq3, isr_irq4, isr_irq5, isr_irq6, isr_irq7, isr_irq8,
	isr_irq9, isr_irq10, isr_irq11, isr_irq12, isr_irq13, isr_irq14, isr_irq15
};

static struct idt_entry entries[256];
static struct idt_ptr pointer;

static void set_gate(int index, void (*handler)(void), unsigned char flags) {
	unsigned long address = (unsigned long)handler;

	entries[index].offset_low = (unsigned short)(address & 0xFFFF);
	entries[index].selector = KERNEL_CODE_SEL;
	entries[index].ist = 0;
	entries[index].flags = flags;
	entries[index].offset_middle = (unsigned short)((address >> 16) & 0xFFFF);
	entries[index].offset_high = (unsigned int)(address >> 32);
	entries[index].zero = 0;
}

void idt_init(void) {
	int i;

	for (i = 0; i < 256; i++) {
		set_gate(i, isr_ignore, 0x8E);
	}

	for (i = 0; i < 15; i++) {
		set_gate(0x21 + i, irq_entries[i], 0x8E);
	}

	set_gate(0x20, isr_timer, 0x8E);
	set_gate(0x81, isr_yield, 0x8E);
	set_gate(0x08, isr_double_fault, 0x8E);
	set_gate(0x0D, isr_gpf, 0x8E);
	set_gate(0x0E, isr_page_fault, 0x8E);
	set_gate(0x80, isr_syscall, 0xEE);

	pointer.limit = sizeof(entries) - 1;
	pointer.base = (unsigned long)&entries;

	asm volatile("lidt %0" : : "m"(pointer));
}
