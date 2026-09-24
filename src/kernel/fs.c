#include "../../include/fs.h"
#include "../../include/mem.h"
#include "../../include/mm.h"
#include "../drivers/disk.h"

#define SECTOR_SIZE ((unsigned int)DISK_SECTOR_SIZE)
#define SECTOR_SHIFT 9u
#define FAT_PER_SECTOR (SECTOR_SIZE / 4)
#define CHAIN_END 0xFFFFFFFFu
#define SLOT_USED 1u
#define DIR_ENTRY_SIZE 128u
#define SLOTS_PER_SECTOR (SECTOR_SIZE / DIR_ENTRY_SIZE)
#define TARGET_CLUSTERS (1u << 20)
#define MAX_CLUSTER_SECTORS 128u
#define MIN_CLUSTERS 8u
#define FORMAT_VERSION 1u
#define MAGIC_VALUE 0x4B465331u
#define OLD_MAGIC_VALUE 0x4B41544Cu
#define OLD_IMAGE_SECTORS 10u
#define OLD_MAX_FILES 16u
#define OLD_NAME_SIZE 32u
#define OLD_DATA_SIZE 256u
#define ZERO_BATCH 8u
#define MAX_IO 0x7FFFFFFFu
#define MIN_DISK_SECTORS (FS_LBA_START + 64)
#define NO_SECTOR 0xFFFFFFFFu

struct super_block {
    unsigned int magic;
    unsigned int version;
    unsigned int total_sectors;
    unsigned int cluster_sectors;
    unsigned int cluster_count;
    unsigned int fat_sectors;
    unsigned int data_start;
    unsigned int root_cluster;
};

struct dir_entry {
    char name[FS_NAME_MAX + 1];
    unsigned int first;
    unsigned int size;
    unsigned int flags;
};

_Static_assert(sizeof(struct dir_entry) == DIR_ENTRY_SIZE, "directory entry must be 128 bytes");
_Static_assert(sizeof(struct super_block) <= SECTOR_SIZE, "super block must fit in one sector");

struct location {
    unsigned int sector;
    unsigned int slot;
};

struct lookup_result {
    int found;
    struct location at;
    struct dir_entry entry;
    int has_free;
    struct location free_at;
    unsigned int last_cluster;
};

static struct super_block sb;
static int mounted = 0;
static unsigned int disk_sectors = 0;
static unsigned int cluster_shift = 0;
static unsigned int next_free = 1;
static unsigned int fat_cache_sector = NO_SECTOR;
static int fat_dirty = 0;

static unsigned char fat_cache[SECTOR_SIZE] __attribute__((aligned(4)));
static unsigned char dir_buf[SECTOR_SIZE] __attribute__((aligned(4)));
static unsigned char io_buf[SECTOR_SIZE] __attribute__((aligned(4)));
static unsigned char zero_block[SECTOR_SIZE * ZERO_BATCH] __attribute__((aligned(4)));

static int block_read(unsigned int sector, void *buffer)
{
    return disk_read_sector(FS_LBA_START + sector, ((unsigned char *)(buffer))) == 0 ? FS_OK : FS_ERR_IO;
}

static int block_write(unsigned int sector, const void *buffer)
{
    return disk_write_sector(FS_LBA_START + sector, ((const unsigned char *)(buffer))) == 0 ? FS_OK : FS_ERR_IO;
}

static int blocks_read(unsigned int sector, unsigned int count, void *buffer)
{
    return disk_read_sectors(FS_LBA_START + sector, count, ((unsigned char *)(buffer))) == 0 ? FS_OK : FS_ERR_IO;
}

static int blocks_write(unsigned int sector, unsigned int count, const void *buffer)
{
    return disk_write_sectors(FS_LBA_START + sector, count, ((const unsigned char *)(buffer))) == 0 ? FS_OK : FS_ERR_IO;
}

static unsigned int load_u32(const unsigned char *bytes)
{
    return ((unsigned int)(bytes[0]))
        | (((unsigned int)(bytes[1])) << 8)
        | (((unsigned int)(bytes[2])) << 16)
        | (((unsigned int)(bytes[3])) << 24);
}

static unsigned int fat_sectors_for(unsigned int clusters)
{
    return (clusters + 1 + FAT_PER_SECTOR - 1) / FAT_PER_SECTOR;
}

static unsigned int cluster_sector(unsigned int cluster)
{
    return sb.data_start + ((cluster - 1) << cluster_shift);
}

static int fat_flush(void)
{
    if (!fat_dirty) {
        return FS_OK;
    }

    int result = block_write(fat_cache_sector, fat_cache);

    if (result == FS_OK) {
        fat_dirty = 0;
    }

    return result;
}

static int fat_load(unsigned int sector)
{
    if (fat_cache_sector == sector) {
        return FS_OK;
    }

    int result = fat_flush();

    if (result != FS_OK) {
        return result;
    }

    result = block_read(sector, fat_cache);

    if (result != FS_OK) {
        fat_cache_sector = NO_SECTOR;
        return result;
    }

    fat_cache_sector = sector;

    return FS_OK;
}

static int fat_get(unsigned int cluster, unsigned int *value)
{
    if (cluster == 0 || cluster > sb.cluster_count) {
        return FS_ERR_CORRUPT;
    }

    int result = fat_load(1 + cluster / FAT_PER_SECTOR);

    if (result != FS_OK) {
        return result;
    }

    *value = ((unsigned int *)(fat_cache))[cluster % FAT_PER_SECTOR];

    return FS_OK;
}

static int fat_set(unsigned int cluster, unsigned int value)
{
    if (cluster == 0 || cluster > sb.cluster_count) {
        return FS_ERR_CORRUPT;
    }

    unsigned int sector = 1 + cluster / FAT_PER_SECTOR;
    int result = fat_load(sector);

    if (result != FS_OK) {
        return result;
    }

    ((unsigned int *)(fat_cache))[cluster % FAT_PER_SECTOR] = value;
    fat_dirty = 1;

    return FS_OK;
}

static int chain_next(unsigned int cluster, unsigned int *next)
{
    unsigned int value;
    int result = fat_get(cluster, &value);

    if (result != FS_OK) {
        return result;
    }

    if (value == CHAIN_END) {
        *next = 0;
        return FS_OK;
    }

    if (value < 1 || value > sb.cluster_count) {
        return FS_ERR_CORRUPT;
    }

    *next = value;

    return FS_OK;
}

static int cluster_alloc(unsigned int *out)
{
    unsigned int candidate = next_free;

    if (candidate < 1 || candidate > sb.cluster_count) {
        candidate = 1;
    }

    for (unsigned int i = 0; i < sb.cluster_count; i++) {
        unsigned int value;
        int result = fat_get(candidate, &value);

        if (result != FS_OK) {
            return result;
        }

        if (value == 0) {
            result = fat_set(candidate, CHAIN_END);

            if (result != FS_OK) {
                return result;
            }

            next_free = candidate + 1;
            *out = candidate;

            return FS_OK;
        }

        candidate++;

        if (candidate > sb.cluster_count) {
            candidate = 1;
        }
    }

    return FS_ERR_NOSPC;
}

static int cluster_free(unsigned int cluster)
{
    int result = fat_set(cluster, 0);

    if (result == FS_OK && cluster < next_free) {
        next_free = cluster;
    }

    return result;
}

static int chain_free(unsigned int first)
{
    unsigned int cluster = first;
    unsigned int steps = 0;

    while (cluster != 0) {
        unsigned int next;
        int result = chain_next(cluster, &next);

        if (result != FS_OK) {
            return result;
        }

        result = cluster_free(cluster);

        if (result != FS_OK) {
            return result;
        }

        cluster = next;
        steps++;

        if (steps > sb.cluster_count) {
            return FS_ERR_CORRUPT;
        }
    }

    return FS_OK;
}

static int cluster_zero(unsigned int cluster)
{
    unsigned int start = cluster_sector(cluster);

    for (unsigned int i = 0; i < (1u << cluster_shift); i++) {
        int result = block_write(start + i, zero_block);

        if (result != FS_OK) {
            return result;
        }
    }

    return FS_OK;
}

static int name_check(const char *name)
{
    if (name == 0 || name[0] == '\0') {
        return FS_ERR_NAME;
    }

    unsigned int length = 0;

    while (name[length] != '\0') {
        unsigned char c = ((unsigned char)(name[length]));

        if (c <= ' ' || c == 127 || c == '/') {
            return FS_ERR_NAME;
        }

        length++;

        if (length > FS_NAME_MAX) {
            return FS_ERR_NAME;
        }
    }

    return FS_OK;
}

static int same_name(const char *stored, const char *name)
{
    for (unsigned int i = 0; i <= FS_NAME_MAX; i++) {
        if (stored[i] != name[i]) {
            return 0;
        }

        if (name[i] == '\0') {
            return 1;
        }
    }

    return 0;
}

static int slot_store(const struct location *where, const struct dir_entry *entry)
{
    int result = fat_flush();

    if (result != FS_OK) {
        return result;
    }

    result = block_read(where->sector, dir_buf);

    if (result != FS_OK) {
        return result;
    }

    memcpy(dir_buf + where->slot * DIR_ENTRY_SIZE, entry, DIR_ENTRY_SIZE);

    return block_write(where->sector, dir_buf);
}

static int dir_lookup(const char *name, struct lookup_result *res)
{
    unsigned int cluster = sb.root_cluster;
    unsigned int steps = 0;

    res->found = 0;
    res->has_free = 0;
    res->last_cluster = sb.root_cluster;

    while (cluster != 0) {
        unsigned int first = cluster_sector(cluster);

        res->last_cluster = cluster;

        for (unsigned int s = 0; s < (1u << cluster_shift); s++) {
            int result = block_read(first + s, dir_buf);

            if (result != FS_OK) {
                return result;
            }

            struct dir_entry *list = ((struct dir_entry *)(dir_buf));

            for (unsigned int i = 0; i < SLOTS_PER_SECTOR; i++) {
                if (!(list[i].flags & SLOT_USED)) {
                    if (!res->has_free) {
                        res->has_free = 1;
                        res->free_at.sector = first + s;
                        res->free_at.slot = i;
                    }

                    continue;
                }

                if (same_name(list[i].name, name)) {
                    res->found = 1;
                    res->at.sector = first + s;
                    res->at.slot = i;
                    memcpy(&res->entry, &list[i], DIR_ENTRY_SIZE);

                    return FS_OK;
                }
            }
        }

        unsigned int next;
        int result = chain_next(cluster, &next);

        if (result != FS_OK) {
            return result;
        }

        cluster = next;
        steps++;

        if (steps > sb.cluster_count) {
            return FS_ERR_CORRUPT;
        }
    }

    return FS_OK;
}

static int entry_create(const char *name, struct lookup_result *res)
{
    struct location target;

    if (res->has_free) {
        target = res->free_at;
    } else {
        unsigned int cluster;
        int result = cluster_alloc(&cluster);

        if (result != FS_OK) {
            return result;
        }

        result = cluster_zero(cluster);

        if (result == FS_OK) {
            result = fat_set(res->last_cluster, cluster);
        }

        if (result != FS_OK) {
            cluster_free(cluster);
            return result;
        }

        target.sector = cluster_sector(cluster);
        target.slot = 0;
    }

    memset(&res->entry, 0, sizeof(res->entry));
    strcpy(res->entry.name, name);
    res->entry.flags = SLOT_USED;

    int result = slot_store(&target, &res->entry);

    if (result != FS_OK) {
        return result;
    }

    res->found = 1;
    res->at = target;

    return FS_OK;
}

static int file_sync(const struct fs_file *file)
{
    int result = fat_flush();

    if (result != FS_OK) {
        return result;
    }

    result = block_read(file->dir_sector, dir_buf);

    if (result != FS_OK) {
        return result;
    }

    struct dir_entry *list = ((struct dir_entry *)(dir_buf));

    list[file->dir_slot].first = file->first;
    list[file->dir_slot].size = file->size;

    return block_write(file->dir_sector, dir_buf);
}

static int file_reach(struct fs_file *file, unsigned int index, int allocate)
{
    unsigned int cluster;
    unsigned int at;

    if (file->first == 0) {
        if (!allocate) {
            return FS_ERR_CORRUPT;
        }

        int result = cluster_alloc(&cluster);

        if (result != FS_OK) {
            return result;
        }

        file->first = cluster;
        file->cursor_cluster = cluster;
        file->cursor_index = 0;
    }

    if (file->cursor_cluster != 0 && file->cursor_index <= index) {
        cluster = file->cursor_cluster;
        at = file->cursor_index;
    } else {
        cluster = file->first;
        at = 0;
    }

    while (at < index) {
        unsigned int next;
        int result = chain_next(cluster, &next);

        if (result != FS_OK) {
            return result;
        }

        if (next == 0) {
            if (!allocate) {
                return FS_ERR_CORRUPT;
            }

            result = cluster_alloc(&next);

            if (result != FS_OK) {
                return result;
            }

            result = fat_set(cluster, next);

            if (result != FS_OK) {
                cluster_free(next);
                return result;
            }
        }

        cluster = next;
        at++;
    }

    file->cursor_cluster = cluster;
    file->cursor_index = at;

    return FS_OK;
}

static int superblock_check(const struct super_block *s)
{
    unsigned int sectors = s->cluster_sectors;

    if (sectors == 0 || sectors > MAX_CLUSTER_SECTORS || (sectors & (sectors - 1)) != 0) {
        return FS_ERR_CORRUPT;
    }

    if (s->total_sectors > disk_sectors - FS_LBA_START) {
        return FS_ERR_CORRUPT;
    }

    if (s->cluster_count == 0 || s->cluster_count > s->total_sectors / sectors) {
        return FS_ERR_CORRUPT;
    }

    if (s->fat_sectors != fat_sectors_for(s->cluster_count) || s->data_start != 1 + s->fat_sectors) {
        return FS_ERR_CORRUPT;
    }

    if (s->data_start + s->cluster_count * sectors > s->total_sectors) {
        return FS_ERR_CORRUPT;
    }

    if (s->root_cluster < 1 || s->root_cluster > s->cluster_count) {
        return FS_ERR_CORRUPT;
    }

    return FS_OK;
}

static unsigned int shift_for(unsigned int sectors)
{
    unsigned int shift = 0;

    while ((1u << shift) < sectors) {
        shift++;
    }

    return shift;
}

static int mount_now(void)
{
    fat_cache_sector = NO_SECTOR;
    fat_dirty = 0;
    next_free = 1;
    mounted = 0;

    int result = block_read(0, io_buf);

    if (result != FS_OK) {
        return result;
    }

    struct super_block candidate;
    memcpy(&candidate, io_buf, sizeof(candidate));

    if (candidate.magic != MAGIC_VALUE) {
        return FS_ERR_NOFS;
    }

    if (candidate.version != FORMAT_VERSION) {
        return FS_ERR_CORRUPT;
    }

    result = superblock_check(&candidate);

    if (result != FS_OK) {
        return result;
    }

    sb = candidate;
    cluster_shift = shift_for(sb.cluster_sectors);
    mounted = 1;

    return FS_OK;
}

static int format_now(void)
{
    if (disk_sectors < MIN_DISK_SECTORS) {
        return FS_ERR_TOOSMALL;
    }

    unsigned int total = disk_sectors - FS_LBA_START;
    unsigned int sectors = 1;

    while (total / sectors > TARGET_CLUSTERS && sectors < MAX_CLUSTER_SECTORS) {
        sectors <<= 1;
    }

    unsigned int clusters = (total - 1) / sectors;
    unsigned int fat = fat_sectors_for(clusters);

    while (clusters > 0 && clusters * sectors + fat > total - 1) {
        clusters--;
        fat = fat_sectors_for(clusters);
    }

    if (clusters < MIN_CLUSTERS) {
        return FS_ERR_TOOSMALL;
    }

    mounted = 0;
    fat_cache_sector = NO_SECTOR;
    fat_dirty = 0;
    next_free = 1;

    sb.magic = MAGIC_VALUE;
    sb.version = FORMAT_VERSION;
    sb.total_sectors = total;
    sb.cluster_sectors = sectors;
    sb.cluster_count = clusters;
    sb.fat_sectors = fat;
    sb.data_start = 1 + fat;
    sb.root_cluster = 1;
    cluster_shift = shift_for(sectors);

    memset(zero_block, 0, sizeof(zero_block));

    for (unsigned int s = 0; s < fat; s += ZERO_BATCH) {
        unsigned int count = fat - s;

        if (count > ZERO_BATCH) {
            count = ZERO_BATCH;
        }

        if (blocks_write(1 + s, count, zero_block) != FS_OK) {
            return FS_ERR_IO;
        }
    }

    mounted = 1;

    int result = fat_set(sb.root_cluster, CHAIN_END);

    if (result == FS_OK) {
        result = cluster_zero(sb.root_cluster);
    }

    if (result == FS_OK) {
        result = fat_flush();
    }

    if (result == FS_OK) {
        memset(io_buf, 0, sizeof(io_buf));
        memcpy(io_buf, &sb, sizeof(sb));
        result = block_write(0, io_buf);
    }

    if (result != FS_OK) {
        mounted = 0;
    }

    return result;
}

static unsigned char *old_image_load(void)
{
    unsigned char *image = ((unsigned char *)(kmalloc(OLD_IMAGE_SECTORS * SECTOR_SIZE)));

    if (blocks_read(0, OLD_IMAGE_SECTORS, image) != FS_OK) {
        kfree(image);
        return 0;
    }

    unsigned int count = load_u32(image + 4);

    if (load_u32(image) != OLD_MAGIC_VALUE || count > OLD_MAX_FILES) {
        kfree(image);
        return 0;
    }

    return image;
}

static void old_image_import(const unsigned char *image)
{
    unsigned int count = load_u32(image + 4);
    const unsigned char *record = image + 8;

    for (unsigned int i = 0; i < count; i++) {
        char name[OLD_NAME_SIZE + 1];
        unsigned int length = 0;

        for (unsigned int j = 0; j < OLD_NAME_SIZE; j++) {
            name[j] = ((char)(record[j]));
        }

        name[OLD_NAME_SIZE] = '\0';

        const char *data = ((const char *)(record + OLD_NAME_SIZE));

        while (length < OLD_DATA_SIZE && data[length] != '\0') {
            length++;
        }

        if (name_check(name) == FS_OK) {
            fs_write_all(name, data, length);
        }

        record += OLD_NAME_SIZE + OLD_DATA_SIZE;
    }
}

static int finish(int result)
{
    int flushed = fat_flush();

    if (result < 0 || flushed == FS_OK) {
        return result;
    }

    return flushed;
}

int fs_init(void)
{
    mounted = 0;
    fat_cache_sector = NO_SECTOR;
    fat_dirty = 0;
    disk_sectors = disk_total_sectors();

    if (disk_sectors == 0) {
        return FS_ERR_NODEV;
    }

    if (disk_sectors < MIN_DISK_SECTORS) {
        return FS_ERR_TOOSMALL;
    }

    int result = mount_now();

    if (result == FS_OK) {
        return FS_INIT_MOUNTED;
    }

    if (result != FS_ERR_NOFS) {
        return result;
    }

    unsigned char *old = old_image_load();

    result = format_now();

    if (result != FS_OK) {
        if (old != 0) {
            kfree(old);
        }

        return result;
    }

    if (old == 0) {
        return FS_INIT_FORMATTED;
    }

    old_image_import(old);
    kfree(old);

    return FS_INIT_MIGRATED;
}

int fs_format(void)
{
    if (disk_sectors == 0) {
        disk_sectors = disk_total_sectors();

        if (disk_sectors == 0) {
            return FS_ERR_NODEV;
        }
    }

    return format_now();
}

int fs_mounted(void)
{
    return mounted;
}

const char *fs_error_text(int code)
{
    switch (code) {
        case FS_OK: return "ok";
        case FS_ERR_IO: return "disk i/o error";
        case FS_ERR_NODEV: return "no disk found";
        case FS_ERR_NOFS: return "filesystem is not mounted";
        case FS_ERR_NOENT: return "file not found";
        case FS_ERR_EXIST: return "file already exists";
        case FS_ERR_NOSPC: return "no space left on disk";
        case FS_ERR_NAME: return "invalid file name";
        case FS_ERR_INVAL: return "invalid argument";
        case FS_ERR_CORRUPT: return "filesystem is corrupted";
        case FS_ERR_TOOSMALL: return "disk is too small";
        case FS_ERR_TOOBIG: return "file is too big for this operation";
        default: return "unknown error";
    }
}

static int create_impl(const char *name)
{
    if (!mounted) {
        return FS_ERR_NOFS;
    }

    int result = name_check(name);

    if (result != FS_OK) {
        return result;
    }

    struct lookup_result res;
    result = dir_lookup(name, &res);

    if (result != FS_OK) {
        return result;
    }

    if (res.found) {
        return FS_ERR_EXIST;
    }

    return entry_create(name, &res);
}

static int remove_impl(const char *name)
{
    if (!mounted) {
        return FS_ERR_NOFS;
    }

    int result = name_check(name);

    if (result != FS_OK) {
        return result;
    }

    struct lookup_result res;
    result = dir_lookup(name, &res);

    if (result != FS_OK) {
        return result;
    }

    if (!res.found) {
        return FS_ERR_NOENT;
    }

    unsigned int first = res.entry.first;

    memset(&res.entry, 0, sizeof(res.entry));
    result = slot_store(&res.at, &res.entry);

    if (result != FS_OK) {
        return result;
    }

    return chain_free(first);
}

int fs_rename(const char *from, const char *to)
{
    if (!mounted) {
        return FS_ERR_NOFS;
    }

    int result = name_check(from);

    if (result == FS_OK) {
        result = name_check(to);
    }

    if (result != FS_OK) {
        return result;
    }

    struct lookup_result target;
    result = dir_lookup(to, &target);

    if (result != FS_OK) {
        return result;
    }

    if (target.found) {
        return FS_ERR_EXIST;
    }

    struct lookup_result source;
    result = dir_lookup(from, &source);

    if (result != FS_OK) {
        return result;
    }

    if (!source.found) {
        return FS_ERR_NOENT;
    }

    memset(source.entry.name, 0, sizeof(source.entry.name));
    strcpy(source.entry.name, to);

    return slot_store(&source.at, &source.entry);
}

static int open_impl(const char *name, struct fs_file *file, int create)
{
    if (!mounted) {
        return FS_ERR_NOFS;
    }

    int result = name_check(name);

    if (result != FS_OK) {
        return result;
    }

    struct lookup_result res;
    result = dir_lookup(name, &res);

    if (result != FS_OK) {
        return result;
    }

    if (!res.found) {
        if (!create) {
            return FS_ERR_NOENT;
        }

        result = entry_create(name, &res);

        if (result != FS_OK) {
            return result;
        }
    }

    file->dir_sector = res.at.sector;
    file->dir_slot = res.at.slot;
    file->first = res.entry.first;
    file->size = res.entry.size;
    file->cursor_index = 0;
    file->cursor_cluster = res.entry.first;

    return FS_OK;
}

int fs_read(struct fs_file *file, unsigned int offset, void *buffer, unsigned int count)
{
    if (!mounted) {
        return FS_ERR_NOFS;
    }

    if (offset >= file->size || count == 0) {
        return 0;
    }

    if (count > MAX_IO) {
        count = MAX_IO;
    }

    if (count > file->size - offset) {
        count = file->size - offset;
    }

    unsigned char *out = ((unsigned char *)(buffer));
    const unsigned int cluster_mask = (SECTOR_SIZE << cluster_shift) - 1;
    unsigned int done = 0;

    while (done < count) {
        unsigned int position = offset + done;
        int result = file_reach(file, position >> (cluster_shift + SECTOR_SHIFT), 0);

        if (result != FS_OK) {
            return done > 0 ? ((int)(done)) : result;
        }

        unsigned int in_cluster = position & cluster_mask;
        unsigned int sector_index = in_cluster >> SECTOR_SHIFT;
        unsigned int in_sector = in_cluster & (SECTOR_SIZE - 1);
        unsigned int lba = cluster_sector(file->cursor_cluster) + sector_index;
        unsigned int remaining = count - done;

        if (in_sector == 0 && remaining >= SECTOR_SIZE) {
            unsigned int sectors = remaining >> SECTOR_SHIFT;
            unsigned int room = (1u << cluster_shift) - sector_index;

            if (sectors > room) {
                sectors = room;
            }

            result = blocks_read(lba, sectors, out + done);

            if (result != FS_OK) {
                return done > 0 ? ((int)(done)) : result;
            }

            done += sectors << SECTOR_SHIFT;
        } else {
            unsigned int chunk = SECTOR_SIZE - in_sector;

            if (chunk > remaining) {
                chunk = remaining;
            }

            result = block_read(lba, io_buf);

            if (result != FS_OK) {
                return done > 0 ? ((int)(done)) : result;
            }

            memcpy(out + done, io_buf + in_sector, chunk);
            done += chunk;
        }
    }

    return ((int)(done));
}

static int write_impl(struct fs_file *file, unsigned int offset, const void *buffer, unsigned int count)
{
    if (!mounted) {
        return FS_ERR_NOFS;
    }

    if (offset > file->size) {
        return FS_ERR_INVAL;
    }

    if (count > MAX_IO) {
        count = MAX_IO;
    }

    if (count > 0xFFFFFFFFu - offset) {
        count = 0xFFFFFFFFu - offset;
    }

    if (count == 0) {
        return 0;
    }

    const unsigned char *in = ((const unsigned char *)(buffer));
    const unsigned int cluster_mask = (SECTOR_SIZE << cluster_shift) - 1;
    unsigned int old_first = file->first;
    unsigned int old_size = file->size;
    unsigned int done = 0;
    int status = FS_OK;

    while (done < count) {
        unsigned int position = offset + done;
        int result = file_reach(file, position >> (cluster_shift + SECTOR_SHIFT), 1);

        if (result != FS_OK) {
            status = result;
            break;
        }

        unsigned int in_cluster = position & cluster_mask;
        unsigned int sector_index = in_cluster >> SECTOR_SHIFT;
        unsigned int in_sector = in_cluster & (SECTOR_SIZE - 1);
        unsigned int lba = cluster_sector(file->cursor_cluster) + sector_index;
        unsigned int remaining = count - done;

        if (in_sector == 0 && remaining >= SECTOR_SIZE) {
            unsigned int sectors = remaining >> SECTOR_SHIFT;
            unsigned int room = (1u << cluster_shift) - sector_index;

            if (sectors > room) {
                sectors = room;
            }

            result = blocks_write(lba, sectors, in + done);

            if (result != FS_OK) {
                status = result;
                break;
            }

            done += sectors << SECTOR_SHIFT;
        } else {
            unsigned int chunk = SECTOR_SIZE - in_sector;

            if (chunk > remaining) {
                chunk = remaining;
            }

            result = block_read(lba, io_buf);

            if (result == FS_OK) {
                memcpy(io_buf + in_sector, in + done, chunk);
                result = block_write(lba, io_buf);
            }

            if (result != FS_OK) {
                status = result;
                break;
            }

            done += chunk;
        }
    }

    if (done > 0 && offset + done > file->size) {
        file->size = offset + done;
    }

    if (file->first != old_first || file->size != old_size) {
        int result = file_sync(file);

        if (result != FS_OK) {
            return result;
        }
    }

    return done > 0 ? ((int)(done)) : status;
}

static int truncate_impl(struct fs_file *file, unsigned int size)
{
    if (!mounted) {
        return FS_ERR_NOFS;
    }

    if (size == file->size) {
        return FS_OK;
    }

    if (size > file->size) {
        return FS_ERR_INVAL;
    }

    const unsigned int cluster_bits = cluster_shift + SECTOR_SHIFT;
    const unsigned int cluster_mask = (1u << cluster_bits) - 1;
    unsigned int keep = (size >> cluster_bits) + ((size & cluster_mask) != 0 ? 1 : 0);
    unsigned int tail = 0;

    if (keep == 0) {
        tail = file->first;
        file->first = 0;
        file->size = 0;
        file->cursor_index = 0;
        file->cursor_cluster = 0;

        int result = file_sync(file);

        if (result != FS_OK) {
            return result;
        }

        return chain_free(tail);
    }

    int result = file_reach(file, keep - 1, 0);

    if (result != FS_OK) {
        return result;
    }

    unsigned int last = file->cursor_cluster;

    result = chain_next(last, &tail);

    if (result != FS_OK) {
        return result;
    }

    file->size = size;
    file->cursor_index = 0;
    file->cursor_cluster = file->first;

    result = file_sync(file);

    if (result != FS_OK) {
        return result;
    }

    if (tail == 0) {
        return FS_OK;
    }

    result = fat_set(last, CHAIN_END);

    if (result != FS_OK) {
        return result;
    }

    return chain_free(tail);
}

char *fs_read_all(const char *name, unsigned int *size, int *error)
{
    struct fs_file file;
    int result = fs_open(name, &file, 0);

    if (result != FS_OK) {
        *error = result;
        return 0;
    }

    if (file.size > FS_READ_ALL_MAX) {
        *error = FS_ERR_TOOBIG;
        return 0;
    }

    char *data = ((char *)(kmalloc(file.size + 1)));
    unsigned int got = 0;

    while (got < file.size) {
        result = fs_read(&file, got, data + got, file.size - got);

        if (result <= 0) {
            kfree(data);
            *error = result < 0 ? result : FS_ERR_IO;
            return 0;
        }

        got += ((unsigned int)(result));
    }

    data[file.size] = '\0';
    *size = file.size;
    *error = FS_OK;

    return data;
}

int fs_write_all(const char *name, const void *data, unsigned int size)
{
    struct fs_file file;
    int result = fs_open(name, &file, 1);

    if (result != FS_OK) {
        return result;
    }

    result = fs_truncate(&file, 0);

    if (result != FS_OK) {
        return result;
    }

    const unsigned char *bytes = ((const unsigned char *)(data));
    unsigned int written = 0;

    while (written < size) {
        result = fs_write(&file, written, bytes + written, size - written);

        if (result < 0) {
            return result;
        }

        if (result == 0) {
            return FS_ERR_IO;
        }

        written += ((unsigned int)(result));
    }

    return FS_OK;
}

void fs_iter_begin(struct fs_iter *it)
{
    it->cluster = mounted ? sb.root_cluster : 0;
    it->sector = 0;
    it->slot = 0;
    it->steps = 0;
}

int fs_iter_next(struct fs_iter *it, struct fs_info *info)
{
    if (!mounted) {
        return FS_ERR_NOFS;
    }

    while (it->cluster != 0) {
        int result = block_read(cluster_sector(it->cluster) + it->sector, dir_buf);

        if (result != FS_OK) {
            return result;
        }

        const struct dir_entry *list = ((const struct dir_entry *)(dir_buf));

        while (it->slot < SLOTS_PER_SECTOR) {
            const struct dir_entry *entry = &list[it->slot++];

            if (entry->flags & SLOT_USED) {
                memcpy(info->name, entry->name, sizeof(info->name));
                info->name[FS_NAME_MAX] = '\0';
                info->size = entry->size;

                return 1;
            }
        }

        it->slot = 0;
        it->sector++;

        if (it->sector >= (1u << cluster_shift)) {
            unsigned int next;

            it->sector = 0;
            result = chain_next(it->cluster, &next);

            if (result != FS_OK) {
                return result;
            }

            it->cluster = next;
            it->steps++;

            if (it->steps > sb.cluster_count) {
                return FS_ERR_CORRUPT;
            }
        }
    }

    return 0;
}

int fs_usage(struct fs_stats *stats)
{
    if (!mounted) {
        return FS_ERR_NOFS;
    }

    unsigned int free_count = 0;

    for (unsigned int s = 0; s < sb.fat_sectors; s++) {
        int result = fat_load(1 + s);

        if (result != FS_OK) {
            return result;
        }

        const unsigned int *entries = ((const unsigned int *)(fat_cache));

        for (unsigned int i = 0; i < FAT_PER_SECTOR; i++) {
            unsigned int cluster = s * FAT_PER_SECTOR + i;

            if (cluster >= 1 && cluster <= sb.cluster_count && entries[i] == 0) {
                free_count++;
            }
        }
    }

    stats->cluster_bytes = SECTOR_SIZE << cluster_shift;
    stats->total_clusters = sb.cluster_count;
    stats->free_clusters = free_count;

    return FS_OK;
}

int fs_create(const char *name)
{
    return finish(create_impl(name));
}

int fs_remove(const char *name)
{
    return finish(remove_impl(name));
}

int fs_open(const char *name, struct fs_file *file, int create)
{
    return finish(open_impl(name, file, create));
}

int fs_write(struct fs_file *file, unsigned int offset, const void *buffer, unsigned int count)
{
    return finish(write_impl(file, offset, buffer, count));
}

int fs_truncate(struct fs_file *file, unsigned int size)
{
    return finish(truncate_impl(file, size));
}
