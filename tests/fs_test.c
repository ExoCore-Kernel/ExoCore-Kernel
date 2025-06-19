#include <stdio.h>
#include "../include/fs.h"
#include "../include/memutils.h"

#define STORAGE_SIZE 128
static unsigned char storage[STORAGE_SIZE];

static size_t mem_read(size_t offset, void *buf, size_t len) {
    memcpy(buf, storage + offset, len);
    return len;
}

static size_t mem_write(size_t offset, const void *data, size_t len) {
    memcpy(storage + offset, data, len);
    return len;
}

int main() {
    fs_device_t dev = {STORAGE_SIZE, mem_read, mem_write};
    fs_mount_device(&dev);
    const char msg[] = "hello";
    if (fs_write(0, msg, sizeof(msg)) != sizeof(msg)) return 1;
    char tmp[16];
    if (fs_read(0, tmp, sizeof(msg)) != sizeof(msg)) return 1;
    if (memcmp(msg, tmp, sizeof(msg)) != 0) return 1;
    printf("fs driver ok\n");
    return 0;
}
