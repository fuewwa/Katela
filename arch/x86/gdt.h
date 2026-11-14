#ifndef GDT_H
#define GDT_H

#define KERNEL_CODE_SEL 0x08
#define KERNEL_DATA_SEL 0x10
#define USER_CODE_SEL   0x18
#define USER_DATA_SEL   0x20
#define TSS_SEL         0x28

void gdt_init(void);
void set_kernel_stack(unsigned int esp);

#endif
