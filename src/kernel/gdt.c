#include "../../include/gdt.h"

struct gdt_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));

struct gdt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct tss_entry {
    unsigned int prev_tss;
    unsigned int esp0;
    unsigned int ss0;
    unsigned int esp1;
    unsigned int ss1;
    unsigned int esp2;
    unsigned int ss2;
    unsigned int cr3;
    unsigned int eip;
    unsigned int eflags;
    unsigned int eax;
    unsigned int ecx;
    unsigned int edx;
    unsigned int ebx;
    unsigned int esp;
    unsigned int ebp;
    unsigned int esi;
    unsigned int edi;
    unsigned int es;
    unsigned int cs;
    unsigned int ss;
    unsigned int ds;
    unsigned int fs;
    unsigned int gs;
    unsigned int ldt;
    unsigned short trap;
    unsigned short iomap_base;
} __attribute__((packed));

extern void gdt_flush(unsigned int gdt_ptr_addr);
extern void tss_flush(void);

static struct gdt_entry entries[6];
static struct gdt_ptr pointer;
static struct tss_entry tss;

unsigned int g_kernel_resume_esp;

static void gdt_set_gate(int index, unsigned int base, unsigned int limit, unsigned char access, unsigned char granularity) {
    entries[index].base_low = (unsigned short)(base & 0xFFFF);
    entries[index].base_middle = (unsigned char)((base >> 16) & 0xFF);
    entries[index].base_high = (unsigned char)((base >> 24) & 0xFF);

    entries[index].limit_low = (unsigned short)(limit & 0xFFFF);
    entries[index].granularity = (unsigned char)((limit >> 16) & 0x0F);
    entries[index].granularity |= granularity & 0xF0;

    entries[index].access = access;
}

static void tss_setup(unsigned short ss0, unsigned int esp0) {
    unsigned int base = (unsigned int)&tss;
    unsigned int limit = sizeof(struct tss_entry) - 1;
    unsigned int i;
    unsigned char *raw = (unsigned char *)&tss;

    gdt_set_gate(5, base, limit, 0x89, 0x00);

    for (i = 0; i < sizeof(struct tss_entry); i++) {
        raw[i] = 0;
    }

    tss.ss0 = ss0;
    tss.esp0 = esp0;
    tss.iomap_base = sizeof(struct tss_entry);
}

void set_kernel_stack(unsigned int esp) {
    tss.esp0 = esp;
    g_kernel_resume_esp = esp;
}

void gdt_init(void) {
    pointer.limit = sizeof(entries) - 1;
    pointer.base = (unsigned int)&entries;

    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    tss_setup(KERNEL_DATA_SEL, 0);

    gdt_flush((unsigned int)&pointer);
    tss_flush();
}
