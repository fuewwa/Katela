#include "idt.h"

extern void isr_timer(void);
extern void isr_ignore(void);

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

static inline void outb(unsigned short port, unsigned char value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

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

    pointer.limit = sizeof(entries) - 1;
    pointer.base = (unsigned int)&entries;

    asm volatile("lidt %0" : : "m"(pointer));
}

void pic_remap(void) {
    unsigned char mask1 = inb(0x21);
    unsigned char mask2 = inb(0xA1);

    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();

    outb(0x21, mask1);
    outb(0xA1, mask2);

    outb(0x21, (unsigned char)(inb(0x21) & ~0x01));
}

void pit_init(unsigned int frequency) {
    unsigned int divisor = 1193182 / frequency;

    outb(0x43, 0x36);
    outb(0x40, (unsigned char)(divisor & 0xFF));
    outb(0x40, (unsigned char)((divisor >> 8) & 0xFF));
}
