#ifndef DISK_H
#define DISK_H

#define DISK_SECTOR_SIZE 512

int disk_read_sector(unsigned int lba, unsigned char *buffer);
int disk_write_sector(unsigned int lba, const unsigned char *buffer);
int disk_read_sectors(unsigned int lba, unsigned int count, unsigned char *buffer);
int disk_write_sectors(unsigned int lba, unsigned int count, const unsigned char *buffer);
unsigned int disk_total_sectors(void);
const char *disk_name(void);

#endif
