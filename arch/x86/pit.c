#include "pit.h"
#include "io.h"

void pit_init(unsigned int frequency) {
    unsigned int divisor = 1193182 / frequency;

    outb(0x43, 0x36);
    outb(0x40, (unsigned char)(divisor & 0xFF));
    outb(0x40, (unsigned char)((divisor >> 8) & 0xFF));
}
