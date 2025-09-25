#include "fs.h"
#include "memutils.h"
#include "console.h"
#include "debuglog.h"

static unsigned char *fs_data = NULL;
static size_t fs_size = 0;

void fs_mount(void *storage, size_t size) {
    fs_data = (unsigned char *)storage;
    fs_size = size;
    console_puts("fs_mount size=");
    console_udec(size);
    console_puts(" data=0x");
    console_uhex((uint64_t)(uintptr_t)storage);
    console_putc('\n');
}

size_t fs_read(size_t offset, void *buf, size_t len) {
    if (!fs_data || offset >= fs_size)
        return 0;
    if (offset + len > fs_size)
        len = fs_size - offset;
    memcpy(buf, fs_data + offset, len);
    console_puts("fs_read offset=");
    console_udec(offset);
    console_puts(" len=");
    console_udec(len);
    console_putc('\n');
    debuglog_hexdump(buf, len > 64 ? 64 : len);
    return len;
}

size_t fs_write(size_t offset, const void *data, size_t len) {
    if (!fs_data || offset >= fs_size)
        return 0;
    if (offset + len > fs_size)
        len = fs_size - offset;
    memcpy(fs_data + offset, data, len);
    console_puts("fs_write offset=");
    console_udec(offset);
    console_puts(" len=");
    console_udec(len);
    console_putc('\n');
    debuglog_hexdump(data, len > 64 ? 64 : len);
    return len;
}

int fs_is_mounted(void) {
    return fs_data != NULL;
}

size_t fs_capacity(void) {
    return fs_size;
}
