#ifndef NVME_H
#define NVME_H

#define NVME_SECTOR_SIZE 512

int nvme_read_sector(unsigned int lba, unsigned char *buffer);
int nvme_write_sector(unsigned int lba, const unsigned char *buffer);
int nvme_read_sectors(unsigned int lba, unsigned int count, unsigned char *buffer);
int nvme_write_sectors(unsigned int lba, unsigned int count, const unsigned char *buffer);
unsigned int nvme_total_sectors(void);

#endif
