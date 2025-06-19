#include "fs.h"
#include "memutils.h"

static fs_device_t *fs_dev = NULL;

static unsigned char *mem_store = NULL;
static size_t mem_size = 0;
static size_t mem_read(size_t offset, void *buf, size_t len) {
    if (offset >= mem_size)
        return 0;
    if (offset + len > mem_size)
        len = mem_size - offset;
    memcpy(buf, mem_store + offset, len);
    return len;
}

static size_t mem_write(size_t offset, const void *data, size_t len) {
    if (offset >= mem_size)
        return 0;
    if (offset + len > mem_size)
        len = mem_size - offset;
    memcpy(mem_store + offset, data, len);
    return len;
}
static fs_device_t mem_dev;

void fs_mount(void *storage, size_t size) {
    mem_store = (unsigned char *)storage;
    mem_size = size;
    mem_dev.size = size;
    mem_dev.read = mem_read;
    mem_dev.write = mem_write;
    fs_mount_device(&mem_dev);
}

void fs_mount_device(fs_device_t *dev) {
    fs_dev = dev;
}

size_t fs_read(size_t offset, void *buf, size_t len) {
    if (!fs_dev || !fs_dev->read || offset >= fs_dev->size)
        return 0;
    if (offset + len > fs_dev->size)
        len = fs_dev->size - offset;
    return fs_dev->read(offset, buf, len);
}

size_t fs_write(size_t offset, const void *data, size_t len) {
    if (!fs_dev || !fs_dev->write || offset >= fs_dev->size)
        return 0;
    if (offset + len > fs_dev->size)
        len = fs_dev->size - offset;
    return fs_dev->write(offset, data, len);
}
