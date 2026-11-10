#ifndef CONSOLE_H
#define CONSOLE_H

void console_init(void);
void print(const char *str);
void backspace(void);
void clear(void);
int console_cols(void);
int console_rows(void);
void console_put(int x, int y, char c, unsigned char attr);
void console_hide_cursor(void);

#endif
