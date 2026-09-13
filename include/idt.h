#ifndef IDT_H
#define IDT_H

void idt_init();
void pic_remap();
void pit_init(unsigned int frequency);

#endif
