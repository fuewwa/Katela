#ifndef LZ4_H
#define LZ4_H

#define LZ4_MAX_INPUT_SIZE 0x7E000000

unsigned int lz4_bound(unsigned int size);
int lz4_compress(const void *source, unsigned int size, void *dest, unsigned int capacity);
int lz4_decompress(const void *source, unsigned int size, void *dest, unsigned int capacity);

#endif
