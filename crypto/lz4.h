#ifndef LZ4_H
#define LZ4_H

#define LZ4_MAX_INPUT_SIZE 0x7E000000
#define LZ4_MIN_MATCH 4
#define LZ4_LAST_LITERALS 5
#define LZ4_MATCH_FIND_LIMIT 12
#define LZ4_MAX_OFFSET 65535

unsigned int lz4_bound(unsigned int size);
int lz4_compress(const void *source, unsigned int size, void *dest, unsigned int capacity);
int lz4_decompress(const void *source, unsigned int size, void *dest, unsigned int capacity);
int lz4_write_sequence(unsigned char **out, unsigned char *end,
	const unsigned char *literals, unsigned int literal_length,
	unsigned int offset, unsigned int match_length);

#endif
