#include "ata.h"

#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30
#define ATA_CMD_FLUSH   0xE7
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_SR_BSY  0x80
#define ATA_SR_DRQ  0x08
#define ATA_SR_ERR  0x01

static inline void outb(unsigned short port, unsigned char value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(unsigned short port, unsigned short value) {
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned short inw(unsigned short port) {
    unsigned short value;
    asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static int ata_wait_bsy(void) {
    int timeout = 100000;

    while (timeout--) {
        if (!(inb(ATA_STATUS) & ATA_SR_BSY)) {
            return 0;
        }
    }

    return -1;
}

static int ata_wait_drq(void) {
    int timeout = 100000;

    while (timeout--) {
        unsigned char status = inb(ATA_STATUS);

        if (status & ATA_SR_ERR) {
            return -1;
        }

        if (status & ATA_SR_DRQ) {
            return 0;
        }
    }

    return -1;
}

static void ata_select(unsigned int lba) {
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
}

int ata_read_sectors(unsigned int lba, unsigned int count, unsigned char *buffer) {
    unsigned int i;
    unsigned int j;

    if (count == 0 || count > 255) {
        return -1;
    }

    if (ata_wait_bsy() != 0) {
        return -1;
    }

    ata_select(lba);

    outb(ATA_SECCOUNT, (unsigned char)count);
    outb(ATA_LBA_LOW, (unsigned char)(lba & 0xFF));
    outb(ATA_LBA_MID, (unsigned char)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (unsigned char)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_READ);

    for (i = 0; i < count; i++) {
        if (ata_wait_bsy() != 0 || ata_wait_drq() != 0) {
            return -1;
        }

        for (j = 0; j < ATA_SECTOR_SIZE / 2; j++) {
            unsigned short word = inw(ATA_DATA);
            buffer[i * ATA_SECTOR_SIZE + j * 2] = (unsigned char)(word & 0xFF);
            buffer[i * ATA_SECTOR_SIZE + j * 2 + 1] = (unsigned char)((word >> 8) & 0xFF);
        }
    }

    return 0;
}

int ata_write_sectors(unsigned int lba, unsigned int count, const unsigned char *buffer) {
    unsigned int i;
    unsigned int j;

    if (count == 0 || count > 255) {
        return -1;
    }

    if (ata_wait_bsy() != 0) {
        return -1;
    }

    ata_select(lba);

    outb(ATA_SECCOUNT, (unsigned char)count);
    outb(ATA_LBA_LOW, (unsigned char)(lba & 0xFF));
    outb(ATA_LBA_MID, (unsigned char)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (unsigned char)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    for (i = 0; i < count; i++) {
        unsigned short word;

        if (ata_wait_bsy() != 0 || ata_wait_drq() != 0) {
            return -1;
        }

        for (j = 0; j < ATA_SECTOR_SIZE / 2; j++) {
            word = (unsigned short)buffer[i * ATA_SECTOR_SIZE + j * 2];
            word |= (unsigned short)buffer[i * ATA_SECTOR_SIZE + j * 2 + 1] << 8;
            outw(ATA_DATA, word);
        }
    }

    outb(ATA_COMMAND, ATA_CMD_FLUSH);
    ata_wait_bsy();

    return 0;
}

int ata_read_sector(unsigned int lba, unsigned char *buffer) {
    return ata_read_sectors(lba, 1, buffer);
}

int ata_write_sector(unsigned int lba, const unsigned char *buffer) {
    return ata_write_sectors(lba, 1, buffer);
}

unsigned int ata_total_sectors(void) {
    unsigned short identify[256];
    unsigned int i;
    unsigned char status;

    outb(ATA_DRIVE_HEAD, 0xA0);

    for (i = 0; i < 4; i++) {
        inb(ATA_STATUS);
    }

    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);

    status = inb(ATA_STATUS);
    if (status == 0 || status == 0xFF) {
        return 0;
    }

    if (ata_wait_bsy() != 0) {
        return 0;
    }

    if (inb(ATA_LBA_MID) != 0 || inb(ATA_LBA_HIGH) != 0) {
        return 0;
    }

    if (ata_wait_drq() != 0) {
        return 0;
    }

    for (i = 0; i < 256; i++) {
        identify[i] = inw(ATA_DATA);
    }

    if (!(identify[49] & (1 << 9))) {
        return 0;
    }

    return ((unsigned int)identify[61] << 16) | identify[60];
}
