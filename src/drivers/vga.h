#ifndef VGA_H
#define VGA_H

#define VGA_COLS 80
#define VGA_ROWS 25

void vga_print(const char* str);
void vga_backspace();
void vga_clear();
void vga_put(int x, int y, char c, unsigned char attr);
void vga_hide_cursor();

#endif
