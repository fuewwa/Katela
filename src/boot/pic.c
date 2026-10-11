#include "pic.h"
#include "io.h"

void pic_remap(void) {
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();

    outb(0x21, 0xFE);
    outb(0xA1, 0xFF);
}

void pic_mask(unsigned int irq) {
    unsigned short port = irq < 8 ? 0x21 : 0xA1;

    outb(port, (unsigned char)(inb(port) | (1u << (irq & 7))));
}

void pic_unmask(unsigned int irq) {
    unsigned short port = irq < 8 ? 0x21 : 0xA1;

    outb(port, (unsigned char)(inb(port) & ~(1u << (irq & 7))));

    if (irq >= 8) {
        outb(0x21, (unsigned char)(inb(0x21) & ~0x04));
    }
}

void pic_eoi(unsigned int irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }

    outb(0x20, 0x20);
}

int pic_spurious(unsigned int irq) {
    unsigned short port;

    if (irq != 7 && irq != 15) {
        return 0;
    }

    port = irq == 7 ? 0x20 : 0xA0;
    outb(port, 0x0B);

    if (inb(port) & 0x80) {
        return 0;
    }

    if (irq == 15) {
        outb(0x20, 0x20);
    }

    return 1;
}
