#include "../../include/fs.h"
#include "../drivers/ata.h"

#define FS_IMAGE_SECTORS 10
#define FS_IMAGE_SIZE (FS_IMAGE_SECTORS * ATA_SECTOR_SIZE)

struct file files[MAX_FILES];
int file_count = 0;

static unsigned char fs_buffer[FS_IMAGE_SIZE];

void strcpy(char *dest, const char *src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int find_file(const char *name) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void put_u32(unsigned char *buffer, unsigned int offset, unsigned int value) {
    buffer[offset] = (unsigned char)(value & 0xFF);
    buffer[offset + 1] = (unsigned char)((value >> 8) & 0xFF);
    buffer[offset + 2] = (unsigned char)((value >> 16) & 0xFF);
    buffer[offset + 3] = (unsigned char)((value >> 24) & 0xFF);
}

static unsigned int get_u32(const unsigned char *buffer, unsigned int offset) {
    unsigned int value = buffer[offset];
    value |= (unsigned int)buffer[offset + 1] << 8;
    value |= (unsigned int)buffer[offset + 2] << 16;
    value |= (unsigned int)buffer[offset + 3] << 24;
    return value;
}

void fs_load(void) {
    unsigned int i;
    unsigned int j;
    unsigned char *cursor;

    if (ata_read_sectors(FS_LBA_START, FS_IMAGE_SECTORS, fs_buffer) != 0) {
        file_count = 0;
        return;
    }

    if (get_u32(fs_buffer, 0) != FS_MAGIC) {
        file_count = 0;
        return;
    }

    file_count = (int)get_u32(fs_buffer, 4);
    if (file_count < 0 || file_count > MAX_FILES) {
        file_count = 0;
        return;
    }

    cursor = fs_buffer + 8;

    for (i = 0; i < (unsigned int)file_count; i++) {
        for (j = 0; j < sizeof(files[i].name); j++) {
            files[i].name[j] = (char)cursor[j];
        }
        cursor += sizeof(files[i].name);

        for (j = 0; j < sizeof(files[i].data); j++) {
            files[i].data[j] = (char)cursor[j];
        }
        cursor += sizeof(files[i].data);
    }
}

void fs_save(void) {
    unsigned int i;
    unsigned int j;
    unsigned char *cursor;

    for (i = 0; i < FS_IMAGE_SIZE; i++) {
        fs_buffer[i] = 0;
    }

    put_u32(fs_buffer, 0, FS_MAGIC);
    put_u32(fs_buffer, 4, (unsigned int)file_count);

    cursor = fs_buffer + 8;

    for (i = 0; i < (unsigned int)file_count; i++) {
        for (j = 0; j < sizeof(files[i].name); j++) {
            cursor[j] = (unsigned char)files[i].name[j];
        }
        cursor += sizeof(files[i].name);

        for (j = 0; j < sizeof(files[i].data); j++) {
            cursor[j] = (unsigned char)files[i].data[j];
        }
        cursor += sizeof(files[i].data);
    }

    ata_write_sectors(FS_LBA_START, FS_IMAGE_SECTORS, fs_buffer);
}
