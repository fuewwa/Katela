#include "gdt.h"

struct gdt_entry {
	unsigned short limit_low;
	unsigned short base_low;
	unsigned char base_middle;
	unsigned char access;
	unsigned char granularity;
	unsigned char base_high;
} __attribute__((packed));

struct tss_descriptor {
	unsigned short limit_low;
	unsigned short base_low;
	unsigned char base_middle;
	unsigned char access;
	unsigned char granularity;
	unsigned char base_high;
	unsigned int base_upper;
	unsigned int reserved;
} __attribute__((packed));

struct gdt_ptr {
	unsigned short limit;
	unsigned long base;
} __attribute__((packed));

struct tss_entry {
	unsigned int reserved0;
	unsigned long rsp0;
	unsigned long rsp1;
	unsigned long rsp2;
	unsigned long reserved1;
	unsigned long ist[7];
	unsigned long reserved2;
	unsigned short reserved3;
	unsigned short iomap_base;
} __attribute__((packed));

extern void gdt_flush(unsigned long gdt_ptr_addr);
extern void tss_flush(void);

static struct gdt_entry entries[7];
static struct gdt_ptr pointer;
static struct tss_entry tss;

unsigned long g_kernel_resume_sp;

static void gdt_set_gate(int index, unsigned char access, unsigned char granularity) {
	entries[index].base_low = 0;
	entries[index].base_middle = 0;
	entries[index].base_high = 0;

	entries[index].limit_low = 0xFFFF;
	entries[index].granularity = (unsigned char)(0x0F | (granularity & 0xF0));

	entries[index].access = access;
}

static void tss_setup(unsigned long rsp0) {
	struct tss_descriptor *descriptor = (struct tss_descriptor *)&entries[5];
	unsigned long base = (unsigned long)&tss;
	unsigned int limit = sizeof(struct tss_entry) - 1;
	unsigned int i;
	unsigned char *raw = (unsigned char *)&tss;

	for (i = 0; i < sizeof(struct tss_entry); i++) {
		raw[i] = 0;
	}

	tss.rsp0 = rsp0;
	tss.iomap_base = sizeof(struct tss_entry);

	descriptor->limit_low = (unsigned short)(limit & 0xFFFF);
	descriptor->base_low = (unsigned short)(base & 0xFFFF);
	descriptor->base_middle = (unsigned char)((base >> 16) & 0xFF);
	descriptor->access = 0x89;
	descriptor->granularity = (unsigned char)((limit >> 16) & 0x0F);
	descriptor->base_high = (unsigned char)((base >> 24) & 0xFF);
	descriptor->base_upper = (unsigned int)(base >> 32);
	descriptor->reserved = 0;
}

void set_kernel_stack(unsigned long rsp) {
	tss.rsp0 = rsp;
	g_kernel_resume_sp = rsp;
}

void gdt_init(void) {
	unsigned int i;
	unsigned char *raw = (unsigned char *)entries;

	for (i = 0; i < sizeof(entries); i++) {
		raw[i] = 0;
	}

	pointer.limit = sizeof(entries) - 1;
	pointer.base = (unsigned long)&entries;

	gdt_set_gate(1, 0x9A, 0xA0);
	gdt_set_gate(2, 0x92, 0xC0);
	gdt_set_gate(3, 0xFA, 0xA0);
	gdt_set_gate(4, 0xF2, 0xC0);

	tss_setup(0);

	gdt_flush((unsigned long)&pointer);
	tss_flush();
}
