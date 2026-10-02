#include "keyboard.h"
#include "../../include/scheduler.h"

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

char get_key() {
    static int shift = 0;
    unsigned char scancode;

    while (1) {
        if (inb(0x64) & 1) {
            scancode = inb(0x60);

            if (scancode & 0x80) {
		if (scancode == 0xAA || scancode == 0xB6) {
		     shift = 0;
		}
                continue;
	    }

	    if (scancode == 0x2A || scancode == 0x36) {
    		shift = 1;
    		continue;
	    }

            switch (scancode) {
		case 0x01: return 27;
		case 0x02: return shift ? '!' : '1';
		case 0x03: return shift ? '@' : '2';
		case 0x04: return shift ? '#' : '3';
		case 0x05: return shift ? '$' : '4';
		case 0x06: return shift ? '%' : '5';
		case 0x07: return shift ? '^' : '6';
		case 0x08: return shift ? '&' : '7';
		case 0x09: return shift ? '*' : '8';
		case 0x0A: return shift ? '(' : '9';
		case 0x0B: return shift ? ')' : '0';
                case 0x1E: return shift ? 'A' : 'a';
		case 0x30: return shift ? 'B' : 'b';
		case 0x2E: return shift ? 'C' : 'c';
		case 0x20: return shift ? 'D' : 'd';
		case 0x12: return shift ? 'E' : 'e';
		case 0x21: return shift ? 'F' : 'f';
		case 0x22: return shift ? 'G' : 'g';
		case 0x23: return shift ? 'H' : 'h';
		case 0x17: return shift ? 'I' : 'i';
		case 0x24: return shift ? 'J' : 'j';
		case 0x25: return shift ? 'K' : 'k';
		case 0x26: return shift ? 'L' : 'l';
		case 0x32: return shift ? 'M' : 'm';
		case 0x31: return shift ? 'N' : 'n';
		case 0x18: return shift ? 'O' : 'o';
		case 0x19: return shift ? 'P' : 'p';
		case 0x10: return shift ? 'Q' : 'q';
		case 0x13: return shift ? 'R' : 'r';
		case 0x1F: return shift ? 'S' : 's';
		case 0x14: return shift ? 'T' : 't';
		case 0x16: return shift ? 'U' : 'u';
		case 0x2F: return shift ? 'V' : 'v';
		case 0x11: return shift ? 'W' : 'w';
		case 0x2D: return shift ? 'X' : 'x';
		case 0x15: return shift ? 'Y' : 'y';
		case 0x2C: return shift ? 'Z' : 'z';
                case 0x1C: return '\n';
                case 0x39: return ' ';
		case 0x0E: return '\b';
		case 0x0F: return '\t';
		case 0x0D: return shift ? '+' : '=';
		case 0x1A: return shift ? '{' : '[';
		case 0x1B: return shift ? '}' : ']';
		case 0x27: return shift ? ':' : ';';
		case 0x28: return shift ? '"' : 0x27;
		case 0x29: return shift ? '~' : '`';
		case 0x33: return shift ? '<' : ',';
		case 0x34: return shift ? '>' : '.';
		case 0x35: return shift ? '?' : '/';
		case 0x0C: return shift ? '_' : '-';
		default:
    		     continue;
            }
        } else {
            task_sleep(10);
        }
    }
}
