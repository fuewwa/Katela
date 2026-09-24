#include "disk.h"
#include "ata.h"
#include "nvme.h"

#define BACKEND_NONE 0
#define BACKEND_ATA  1
#define BACKEND_NVME 2

static int backend = BACKEND_NONE;
static unsigned int total_sectors;

static void probe(void) {
    unsigned int sectors = ata_total_sectors();

    if (sectors != 0) {
        backend = BACKEND_ATA;
        total_sectors = sectors;
        return;
    }

    sectors = nvme_total_sectors();

    if (sectors != 0) {
        backend = BACKEND_NVME;
        total_sectors = sectors;
    }
}

unsigned int disk_total_sectors(void) {
    if (backend == BACKEND_NONE) {
        probe();
    }

    return backend == BACKEND_NONE ? 0 : total_sectors;
}

const char *disk_name(void) {
    if (backend == BACKEND_NONE) {
        probe();
    }

    if (backend == BACKEND_ATA) {
        return "ata";
    }

    if (backend == BACKEND_NVME) {
        return "nvme";
    }

    return "none";
}

int disk_read_sectors(unsigned int lba, unsigned int count, unsigned char *buffer) {
    if (backend == BACKEND_NONE) {
        probe();
    }

    if (backend == BACKEND_ATA) {
        return ata_read_sectors(lba, count, buffer);
    }

    if (backend == BACKEND_NVME) {
        return nvme_read_sectors(lba, count, buffer);
    }

    return -1;
}

int disk_write_sectors(unsigned int lba, unsigned int count, const unsigned char *buffer) {
    if (backend == BACKEND_NONE) {
        probe();
    }

    if (backend == BACKEND_ATA) {
        return ata_write_sectors(lba, count, buffer);
    }

    if (backend == BACKEND_NVME) {
        return nvme_write_sectors(lba, count, buffer);
    }

    return -1;
}

int disk_read_sector(unsigned int lba, unsigned char *buffer) {
    return disk_read_sectors(lba, 1, buffer);
}

int disk_write_sector(unsigned int lba, const unsigned char *buffer) {
    return disk_write_sectors(lba, 1, buffer);
}
