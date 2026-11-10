#include "vga.h"
#include "../../include/io.h"

#define WIDTH VGA_COLS
#define HEIGHT VGA_ROWS

static int pos = 0;
static char* video = (char*)0xB8000;

static inline void update_cursor() {
    int cursor = pos / 2;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(cursor & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((cursor >> 8) & 0xFF));
}

static void scroll() {
    for (int i = 0; i < (HEIGHT - 1) * WIDTH * 2; i++) {
        video[i] = video[i + WIDTH * 2];
    }

    for (int i = (HEIGHT - 1) * WIDTH * 2; i < HEIGHT * WIDTH * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x07;
    }

    pos -= WIDTH * 2;
    if (pos < 0) pos = 0;
}

void vga_backspace() {
    if (pos <= 0) return;

    pos -= 2;

    video[pos] = ' ';
    video[pos + 1] = 0x07;
    update_cursor();
}

void vga_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {

        if (str[i] == '\n') {
            pos += WIDTH * 2 - (pos % (WIDTH * 2));
	} else if (str[i] == '\t') {
	    for (int j = 0; j < 5; j++) {
		video[pos++] = ' ';
		video[pos++] = 0x07;
	    }
        } else {
            video[pos++] = str[i];
            video[pos++] = 0x07;
        }

        if (pos >= WIDTH * HEIGHT * 2) {
            scroll();
        }
    }
    update_cursor();
}

void vga_clear() {
    for (int i = 0; i < WIDTH * HEIGHT * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x07;
    }
    pos = 0;
    update_cursor();
}

void vga_put(int x, int y, char c, unsigned char attr) {
    int index = (y * WIDTH + x) * 2;

    video[index] = c;
    video[index + 1] = (char)attr;
}

void vga_hide_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}
