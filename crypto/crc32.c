#include "crc32.h"

static const unsigned int table[16] = {
	0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
	0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
	0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
	0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

unsigned int crc32_update(unsigned int crc, const void *data, unsigned int size)
{
	const unsigned char *input = (const unsigned char *)data;
	unsigned int i;

	crc = ~crc;

	for (i = 0; i < size; i++) {
		crc ^= input[i];
		crc = (crc >> 4) ^ table[crc & 0x0f];
		crc = (crc >> 4) ^ table[crc & 0x0f];
	}

	return ~crc;
}

unsigned int crc32(const void *data, unsigned int size)
{
	return crc32_update(0, data, size);
}
