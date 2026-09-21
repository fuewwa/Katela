#ifndef FS_H
#define FS_H

#define FS_LBA_START 100
#define FS_NAME_MAX 115
#define FS_READ_ALL_MAX (1024u * 1024u)

#define FS_OK 0
#define FS_ERR_IO -1
#define FS_ERR_NODEV -2
#define FS_ERR_NOFS -3
#define FS_ERR_NOENT -4
#define FS_ERR_EXIST -5
#define FS_ERR_NOSPC -6
#define FS_ERR_NAME -7
#define FS_ERR_INVAL -8
#define FS_ERR_CORRUPT -9
#define FS_ERR_TOOSMALL -10
#define FS_ERR_TOOBIG -11

#define FS_INIT_MOUNTED 0
#define FS_INIT_FORMATTED 1
#define FS_INIT_MIGRATED 2

struct fs_file {
    unsigned int dir_sector;
    unsigned int dir_slot;
    unsigned int first;
    unsigned int size;
    unsigned int cursor_index;
    unsigned int cursor_cluster;
};

struct fs_info {
    char name[FS_NAME_MAX + 1];
    unsigned int size;
};

struct fs_iter {
    unsigned int cluster;
    unsigned int sector;
    unsigned int slot;
    unsigned int steps;
};

struct fs_stats {
    unsigned int cluster_bytes;
    unsigned int total_clusters;
    unsigned int free_clusters;
};

int fs_init(void);
int fs_format(void);
int fs_mounted(void);
const char *fs_error_text(int code);

int fs_create(const char *name);
int fs_remove(const char *name);
int fs_rename(const char *from, const char *to);

int fs_open(const char *name, struct fs_file *file, int create);
int fs_read(struct fs_file *file, unsigned int offset, void *buffer, unsigned int count);
int fs_write(struct fs_file *file, unsigned int offset, const void *buffer, unsigned int count);
int fs_truncate(struct fs_file *file, unsigned int size);

char *fs_read_all(const char *name, unsigned int *size, int *error);
int fs_write_all(const char *name, const void *data, unsigned int size);

void fs_iter_begin(struct fs_iter *it);
int fs_iter_next(struct fs_iter *it, struct fs_info *info);

int fs_usage(struct fs_stats *stats);

#endif
