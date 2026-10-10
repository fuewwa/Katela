#ifndef CHECKSUM_H
#define CHECKSUM_H

unsigned int checksum_add(unsigned int sum, const void *data, unsigned int size);
unsigned short checksum_finish(unsigned int sum);
unsigned short checksum(const void *data, unsigned int size);

#endif
