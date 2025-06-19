#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Mount a contiguous memory region as the storage backing. */
void fs_mount(void *storage, size_t size);

/* Read up to 'len' bytes from 'offset'. Returns bytes read. */
size_t fs_read(size_t offset, void *buf, size_t len);

/* Write up to 'len' bytes to 'offset'. Returns bytes written. */
size_t fs_write(size_t offset, const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* FS_H */
