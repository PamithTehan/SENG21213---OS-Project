#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stddef.h>

#define FS_MAX_FILES     16
#define FS_MAX_FILENAME  32
#define FS_MAX_FILESIZE  1024

typedef struct {
    char     name[FS_MAX_FILENAME];
    uint32_t size;
    uint8_t  data[FS_MAX_FILESIZE];
    uint8_t  used;
} fs_node_t;

void fs_init(void);
int  fs_create(const char *name, const char *content);
int  fs_open(const char *name);
int  fs_read(int fd, char *buf, size_t max_len);
int  fs_get_count(void);

const fs_node_t *fs_get_node(int idx);

#endif