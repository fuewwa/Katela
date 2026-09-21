#pragma GCC optimize("no-tree-loop-distribute-patterns")

#include "../../include/mem.h"

void *memcpy(void *dest, const void *src, unsigned int count)
{
    unsigned char *to = ((unsigned char *)(dest));
    const unsigned char *from = ((const unsigned char *)(src));

    for (unsigned int i = 0; i < count; i++) {
        to[i] = from[i];
    }

    return dest;
}

void *memmove(void *dest, const void *src, unsigned int count)
{
    unsigned char *to = ((unsigned char *)(dest));
    const unsigned char *from = ((const unsigned char *)(src));

    if (to < from) {
        for (unsigned int i = 0; i < count; i++) {
            to[i] = from[i];
        }
    } else {
        for (unsigned int i = count; i > 0; i--) {
            to[i - 1] = from[i - 1];
        }
    }

    return dest;
}

void *memset(void *dest, int value, unsigned int count)
{
    unsigned char *to = ((unsigned char *)(dest));

    for (unsigned int i = 0; i < count; i++) {
        to[i] = ((unsigned char)(value));
    }

    return dest;
}

int memcmp(const void *a, const void *b, unsigned int count)
{
    const unsigned char *left = ((const unsigned char *)(a));
    const unsigned char *right = ((const unsigned char *)(b));

    for (unsigned int i = 0; i < count; i++) {
        if (left[i] != right[i]) {
            return left[i] - right[i];
        }
    }

    return 0;
}

int strlen(const char *str)
{
    int length = 0;

    while (str[length] != '\0') {
        length++;
    }

    return length;
}

char *strcpy(char *dest, const char *src)
{
    unsigned int i = 0;

    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';

    return dest;
}

int strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b)) {
        a++;
        b++;
    }

    return *(const unsigned char *)a - *(const unsigned char *)b;
}
