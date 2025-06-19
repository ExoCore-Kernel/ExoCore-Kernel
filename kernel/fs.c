#include "fs.h"
#include "memutils.h"

static unsigned char *fs_data = NULL;
static size_t fs_size = 0;

void fs_mount(void *storage, size_t size) {
    fs_data = (unsigned char *)storage;
    fs_size = size;
}

size_t fs_read(size_t offset, void *buf, size_t len) {
    if (!fs_data || offset >= fs_size)
        return 0;
    if (offset + len > fs_size)
        len = fs_size - offset;
    memcpy(buf, fs_data + offset, len);
    return len;
}

size_t fs_write(size_t offset, const void *data, size_t len) {
    if (!fs_data || offset >= fs_size)
        return 0;
    if (offset + len > fs_size)
        len = fs_size - offset;
    memcpy(fs_data + offset, data, len);
    return len;
}
