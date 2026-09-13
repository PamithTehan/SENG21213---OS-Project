#include "fs.h"

static fs_node_t file_table[FS_MAX_FILES];


static int fs_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static size_t fs_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static void fs_strcpy(char *dest, const char *src, size_t max) {
    size_t i = 0;
    while (src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}


int fs_create(const char *name, const char *content) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!file_table[i].used) {
            fs_strcpy(file_table[i].name, name, FS_MAX_FILENAME);
            
            size_t len = fs_strlen(content);
            if (len >= FS_MAX_FILESIZE) len = FS_MAX_FILESIZE - 1;
            
            for (size_t j = 0; j < len; j++) {
                file_table[i].data[j] = (uint8_t)content[j];
            }
            file_table[i].data[len] = '\0';
            file_table[i].size = len;
            file_table[i].used = 1;
            return i;
        }
    }
    return -1; 
}

int fs_open(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (file_table[i].used && fs_strcmp(file_table[i].name, name) == 0) {
            return i;
        }
    }
    return -1; 
}

int fs_read(int fd, char *buf, size_t max_len) {
    if (fd < 0 || fd >= FS_MAX_FILES || !file_table[fd].used) {
        return -1;
    }

    size_t to_read = file_table[fd].size;
    if (to_read >= max_len) to_read = max_len - 1;

    for (size_t i = 0; i < to_read; i++) {
        buf[i] = (char)file_table[fd].data[i];
    }
    buf[to_read] = '\0';
    return to_read;
}

int fs_get_count(void) {
    return FS_MAX_FILES;
}

const fs_node_t *fs_get_node(int idx) {
    if (idx < 0 || idx >= FS_MAX_FILES) return 0;
    return &file_table[idx];
}

void fs_init(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        file_table[i].used = 0;
    }

    fs_create("README.TXT", "Welcome to SENG21213-OS RAM Disk.\nStage 4 File System initialized successfully.\n");
    fs_create("SYSINFO.LOG", "Kernel: i686 Protected Mode\nPMM Pool: 16 MB\nScheduler: Round-Robin\n");
    fs_create("AUTHORS.TXT", "Developed for SENG21213: Computer Architecture & OS.\n");
}