#ifndef ATA_H
#define ATA_H

#define ATA_SECTOR_SIZE 512

int ata_read_sector(unsigned int lba, unsigned char *buffer);
int ata_write_sector(unsigned int lba, const unsigned char *buffer);
int ata_read_sectors(unsigned int lba, unsigned int count, unsigned char *buffer);
int ata_write_sectors(unsigned int lba, unsigned int count, const unsigned char *buffer);

#endif
