#ifndef CRC32_H
#define CRC32_H

unsigned int crc32_update(unsigned int crc, const void *data, unsigned int size);
unsigned int crc32(const void *data, unsigned int size);

#endif
