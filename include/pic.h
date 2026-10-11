#ifndef PIC_H
#define PIC_H

void pic_remap(void);
void pic_mask(unsigned int irq);
void pic_unmask(unsigned int irq);
void pic_eoi(unsigned int irq);
int pic_spurious(unsigned int irq);

#endif
