#include "../../include/recovery.h"
#include "../../include/layout.h"
#include "../drivers/ata.h"

#define SECTOR 512
#define CHUNK_SECTORS 64
#define INFO_FLAG_MODS (1u << 3)

_Static_assert(KERNEL_LBA + KERNEL_SECTORS == FACTORY_SECTORS, "layout mismatch");
_Static_assert(BOOT_CFG_LBA + BOOT_CFG_SECTORS <= KERNEL_LBA, "layout mismatch");

extern unsigned int multiboot_info;

struct info {
	unsigned int flags;
	unsigned int mem_lower;
	unsigned int mem_upper;
	unsigned int boot_device;
	unsigned int cmdline;
	unsigned int mods_count;
	unsigned int mods_addr;
};

struct module {
	unsigned int start;
	unsigned int end;
	unsigned int string;
	unsigned int reserved;
};

static const unsigned char *find_image(void) {
	const struct info *info = (const struct info *)multiboot_info;

	if (!info || !(info->flags & INFO_FLAG_MODS) || info->mods_count == 0) {
		return 0;
	}

	const struct module *modules = (const struct module *)info->mods_addr;

	for (unsigned int i = 0; i < info->mods_count; i++) {
		if (modules[i].end - modules[i].start == FACTORY_SECTORS * SECTOR) {
			return (const unsigned char *)modules[i].start;
		}
	}
	return 0;
}

static int image_valid(const unsigned char *image) {
	if (image[510] != 0x55 || image[511] != 0xAA) {
		return 0;
	}

	const unsigned char *kernel = image + KERNEL_LBA * SECTOR;

	return kernel[0] == 0x7F && kernel[1] == 'E' && kernel[2] == 'L' && kernel[3] == 'F';
}

int recovery_image_present(void) {
	return find_image() != 0;
}

int recovery_install(void (*progress)(unsigned int done, unsigned int total)) {
	const unsigned char *image = find_image();

	if (!image) {
		return INSTALL_NO_IMAGE;
	}
	if (!image_valid(image)) {
		return INSTALL_BAD_IMAGE;
	}

	unsigned int disk_sectors = ata_total_sectors();

	if (disk_sectors == 0) {
		return INSTALL_NO_DISK;
	}
	if (disk_sectors < FACTORY_SECTORS) {
		return INSTALL_DISK_TOO_SMALL;
	}

	unsigned int total = FACTORY_SECTORS * 2;
	unsigned int done = 0;

	for (unsigned int lba = 0; lba < FACTORY_SECTORS; lba += CHUNK_SECTORS) {
		if (ata_write_sectors(lba, CHUNK_SECTORS, image + lba * SECTOR) != 0) {
			return INSTALL_WRITE_FAILED;
		}
		done += CHUNK_SECTORS;
		if (progress) {
			progress(done, total);
		}
	}

	unsigned char buffer[SECTOR];

	for (unsigned int lba = 0; lba < FACTORY_SECTORS; lba++) {
		if (ata_read_sector(lba, buffer) != 0) {
			return INSTALL_VERIFY_FAILED;
		}

		const unsigned char *expected = image + lba * SECTOR;

		for (unsigned int i = 0; i < SECTOR; i++) {
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

const char *recovery_install_text(int result) {
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
