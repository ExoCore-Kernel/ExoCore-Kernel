#include "fs.h"

static fs_device_t *fs_dev = NULL;

void fs_mount(fs_device_t *dev) {
    fs_dev = dev;
}

size_t fs_read(size_t offset, void *buf, size_t len) {
    if (!fs_dev || !fs_dev->read)
        return 0;
    if (offset >= fs_dev->size)
        return 0;
    if (offset + len > fs_dev->size)
        len = fs_dev->size - offset;
    return fs_dev->read(offset, buf, len);
}

size_t fs_write(size_t offset, const void *data, size_t len) {
    if (!fs_dev || !fs_dev->write)
        return 0;
    if (offset >= fs_dev->size)
        return 0;
    if (offset + len > fs_dev->size)
        len = fs_dev->size - offset;
    return fs_dev->write(offset, data, len);
}
