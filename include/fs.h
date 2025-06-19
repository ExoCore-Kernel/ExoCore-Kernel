#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t size; /* total bytes available */
    size_t (*read)(size_t offset, void *buf, size_t len);
    size_t (*write)(size_t offset, const void *data, size_t len);
} fs_device_t;

/* Mount a generic storage device */
void fs_mount(fs_device_t *dev);

/* Read up to 'len' bytes from 'offset'. Returns bytes read. */
size_t fs_read(size_t offset, void *buf, size_t len);

/* Write up to 'len' bytes to 'offset'. Returns bytes written. */
size_t fs_write(size_t offset, const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* FS_H */
