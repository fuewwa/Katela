#include "recovery.hpp"
#include "../../include/layout.h"

extern "C" {
#include "../drivers/ata.h"
}

extern "C" unsigned multiboot_info;

namespace {

const unsigned SECTOR = 512;
const unsigned CHUNK_SECTORS = 64;
const unsigned INFO_FLAG_MODS = 1u << 3;

static_assert(KERNEL_LBA + KERNEL_SECTORS == FACTORY_SECTORS, "layout mismatch");
static_assert(BOOT_CFG_LBA + BOOT_CFG_SECTORS <= KERNEL_LBA, "layout mismatch");

struct Info {
    unsigned flags;
    unsigned mem_lower;
    unsigned mem_upper;
    unsigned boot_device;
    unsigned cmdline;
    unsigned mods_count;
    unsigned mods_addr;
};

struct Module {
    unsigned start;
    unsigned end;
    unsigned string;
    unsigned reserved;
};

const unsigned char *find_image() {
    const Info *info = reinterpret_cast<const Info *>(multiboot_info);
    if (!info || !(info->flags & INFO_FLAG_MODS) || info->mods_count == 0) {
        return 0;
    }

    const Module *modules = reinterpret_cast<const Module *>(info->mods_addr);
    for (unsigned i = 0; i < info->mods_count; i++) {
        if (modules[i].end - modules[i].start == FACTORY_SECTORS * SECTOR) {
            return reinterpret_cast<const unsigned char *>(modules[i].start);
        }
    }
    return 0;
}

bool image_valid(const unsigned char *image) {
    if (image[510] != 0x55 || image[511] != 0xAA) {
        return false;
    }
    const unsigned char *kernel = image + KERNEL_LBA * SECTOR;
    return kernel[0] == 0x7F && kernel[1] == 'E' && kernel[2] == 'L' && kernel[3] == 'F';
}

}

namespace recovery {

bool image_present() {
    return find_image() != 0;
}

int install(ProgressFn progress) {
    const unsigned char *image = find_image();
    if (!image) {
        return INSTALL_NO_IMAGE;
    }
    if (!image_valid(image)) {
        return INSTALL_BAD_IMAGE;
    }

    unsigned disk_sectors = ata_total_sectors();
    if (disk_sectors == 0) {
        return INSTALL_NO_DISK;
    }
    if (disk_sectors < FACTORY_SECTORS) {
        return INSTALL_DISK_TOO_SMALL;
    }

    unsigned total = FACTORY_SECTORS * 2;
    unsigned done = 0;

    for (unsigned lba = 0; lba < FACTORY_SECTORS; lba += CHUNK_SECTORS) {
        if (ata_write_sectors(lba, CHUNK_SECTORS, image + lba * SECTOR) != 0) {
            return INSTALL_WRITE_FAILED;
        }
        done += CHUNK_SECTORS;
        if (progress) {
            progress(done, total);
        }
    }

    unsigned char buffer[SECTOR];
    for (unsigned lba = 0; lba < FACTORY_SECTORS; lba++) {
        if (ata_read_sector(lba, buffer) != 0) {
            return INSTALL_VERIFY_FAILED;
        }
        const unsigned char *expected = image + lba * SECTOR;
        for (unsigned i = 0; i < SECTOR; i++) {
            if (buffer[i] != expected[i]) {
                return INSTALL_VERIFY_FAILED;
            }
        }
        if ((lba % CHUNK_SECTORS) == CHUNK_SECTORS - 1) {
            done += CHUNK_SECTORS;
            if (progress) {
                progress(done, total);
            }
        }
    }

    return INSTALL_OK;
}

const char *install_text(int result) {
    switch (result) {
        case INSTALL_OK: return "Factory configuration installed and verified.";
        case INSTALL_NO_IMAGE: return "Factory image was not loaded by the bootloader.";
        case INSTALL_BAD_IMAGE: return "Factory image is damaged.";
        case INSTALL_NO_DISK: return "No ATA disk found.";
        case INSTALL_DISK_TOO_SMALL: return "Disk is too small.";
        case INSTALL_WRITE_FAILED: return "Disk write failed.";
        case INSTALL_VERIFY_FAILED: return "Verification failed.";
        default: return "Unknown error.";
    }
}

}
