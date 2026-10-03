#include "pic.h"
#include "io.h"

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
