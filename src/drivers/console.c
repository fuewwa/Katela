#include "console.h"
#include "vga.h"
#include "framebuffer.h"

#define MULTIBOOT_MAGIC 0x2BADB002

extern unsigned int multiboot_magic;
extern unsigned int multiboot_info;

static int graphical;

void console_init(void) {
    graphical = 0;

    if (multiboot_magic == MULTIBOOT_MAGIC && multiboot_info != 0) {
        graphical = framebuffer_init(multiboot_info);
    }
}

void print(const char *str) {
    if (graphical) {
        framebuffer_print(str);
    } else {
        vga_print(str);
    }
}

void backspace(void) {
    if (graphical) {
        framebuffer_backspace();
    } else {
        vga_backspace();
    }
}

void clear(void) {
    if (graphical) {
        framebuffer_clear();
    } else {
        vga_clear();
    }
}

int console_cols(void) {
    return graphical ? framebuffer_cols() : VGA_COLS;
}

int console_rows(void) {
    return graphical ? framebuffer_rows() : VGA_ROWS;
}

void console_put(int x, int y, char c, unsigned char attr) {
    if (graphical) {
        framebuffer_put(x, y, c, attr);
    } else {
        vga_put(x, y, c, attr);
    }
}

void console_hide_cursor(void) {
    if (graphical) {
        framebuffer_hide_cursor();
    } else {
        vga_hide_cursor();
    }
}
