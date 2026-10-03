#include "idt.h"

extern void isr_timer(void);
extern void isr_yield(void);
extern void isr_ignore(void);
extern void isr_syscall(void);
extern void isr_gpf(void);

struct idt_entry {
    unsigned short base_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char flags;
    unsigned short base_high;
} __attribute__((packed));

struct idt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

static struct idt_entry entries[256];
static struct idt_ptr pointer;

static inline unsigned short read_cs(void) {
    unsigned short selector;
    asm volatile("mov %%cs, %0" : "=r"(selector));
    return selector;
}

static void set_gate(int index, unsigned int handler, unsigned short selector, unsigned char flags) {
    entries[index].base_low = (unsigned short)(handler & 0xFFFF);
    entries[index].base_high = (unsigned short)((handler >> 16) & 0xFFFF);
    entries[index].selector = selector;
    entries[index].zero = 0;
    entries[index].flags = flags;
}

void idt_init(void) {
    unsigned short selector = read_cs();
    int i;

    for (i = 0; i < 256; i++) {
        set_gate(i, (unsigned int)isr_ignore, selector, 0x8E);
    }

    set_gate(0x20, (unsigned int)isr_timer, selector, 0x8E);
    set_gate(0x81, (unsigned int)isr_yield, selector, 0x8E);
    set_gate(0x0D, (unsigned int)isr_gpf, selector, 0x8E);
    set_gate(0x80, (unsigned int)isr_syscall, selector, 0xEE);

    pointer.limit = sizeof(entries) - 1;
    pointer.base = (unsigned int)&entries;

    asm volatile("lidt %0" : : "m"(pointer));
}
