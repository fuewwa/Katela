#include "checksum.h"

unsigned int checksum_add(unsigned int sum, const void *data, unsigned int size)
{
	const unsigned char *bytes = (const unsigned char *)data;

	while (size > 1) {
		unsigned int word = ((unsigned int)bytes[0] << 8) | bytes[1];

		sum += word;
		if (sum < word) {
			sum++;
		}

		bytes += 2;
		size -= 2;
	}

	if (size == 1) {
		unsigned int word = (unsigned int)bytes[0] << 8;

		sum += word;
		if (sum < word) {
			sum++;
		}
	}

	return sum;
}

unsigned short checksum_finish(unsigned int sum)
{
	while (sum >> 16) {
		sum = (sum & 0xFFFFu) + (sum >> 16);
	}

	return (unsigned short)~sum;
}

unsigned short checksum(const void *data, unsigned int size)
{
	return checksum_finish(checksum_add(0, data, size));
}
