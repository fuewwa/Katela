#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

int framebuffer_init(unsigned int info_address);
void framebuffer_print(const char *str);
void framebuffer_backspace(void);
void framebuffer_clear(void);
int framebuffer_cols(void);
int framebuffer_rows(void);
void framebuffer_put(int x, int y, char c, unsigned char attr);
void framebuffer_hide_cursor(void);

#endif
