#ifndef LZ4HC_H
#define LZ4HC_H

#define LZ4HC_MIN_LEVEL 1
#define LZ4HC_MAX_LEVEL 9
#define LZ4HC_DEFAULT_LEVEL 6

int lz4hc_compress(const void *source, unsigned int size, void *dest, unsigned int capacity, int level);

#endif
