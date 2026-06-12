#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FS_MAX_PATH 128
#define FS_MAX_OPEN 16

/* Mount a storage backing. FAT32 volumes are detected automatically;
 * unformatted buffers remain available through the raw byte interface.
 */
void fs_mount(void *storage, size_t size);

/* Read up to 'len' bytes from 'offset'. Returns bytes read. On a FAT32
 * mount this addresses bytes in the mounted volume image.
 */
size_t fs_read(size_t offset, void *buf, size_t len);

/* Write up to 'len' bytes to 'offset'. Returns bytes written. On a FAT32
 * mount this addresses bytes in the mounted volume image.
 */
size_t fs_write(size_t offset, const void *data, size_t len);

/* Total capacity of the backing store in bytes (0 if unmounted). */
size_t fs_capacity(void);

/* Non-zero if a backing store is currently mounted. */
int fs_is_mounted(void);

/* Non-zero when the mounted backing store was recognized as FAT32. */
int fs_is_fat32(void);

/* FAT32 root-directory file calls using 8.3 names. These are intentionally
 * separate from VFS calls, which keep the vfs_ prefix and SYS_VFS_* numbers.
 */
int fs_open(const char *path, int flags);
long fs_read_fd(int fd, void *buf, size_t len);
long fs_write_fd(int fd, const void *buf, size_t len);
long fs_lseek_fd(int fd, long offset, int whence);
int fs_close(int fd);
long fs_file_size(int fd);

#ifdef __cplusplus
}
#endif

#endif /* FS_H */
