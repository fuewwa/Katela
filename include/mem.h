#ifndef MEM_H
#define MEM_H

void *memcpy(void *dest, const void *src, unsigned int count);
void *memmove(void *dest, const void *src, unsigned int count);
void *memset(void *dest, int value, unsigned int count);
int memcmp(const void *a, const void *b, unsigned int count);

int strlen(const char *str);
char *strcpy(char *dest, const char *src);
int strcmp(const char *a, const char *b);

#endif
