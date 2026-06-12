#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VFS_MAX_NAME 31
#define VFS_MAX_PATH 128
#define VFS_MAX_DIRENTS 16

#define VFS_O_RDONLY 0x01
#define VFS_O_WRONLY 0x02
#define VFS_O_RDWR   0x03
#define VFS_O_CREAT  0x10
#define VFS_O_TRUNC  0x20
#define VFS_O_APPEND 0x40

#define VFS_SEEK_SET 0
#define VFS_SEEK_CUR 1
#define VFS_SEEK_END 2

#define VFS_TYPE_FILE 1
#define VFS_TYPE_DIR  2

typedef struct {
    uint32_t type;
    size_t size;
    uint32_t inode;
    uint32_t children;
} vfs_stat_t;

typedef struct {
    uint32_t type;
    uint32_t inode;
    size_t size;
    char name[VFS_MAX_NAME + 1];
} vfs_dirent_t;

int vfs_init(void);
int vfs_mkdir(const char *path);
int vfs_unlink(const char *path);
int vfs_rename(const char *old_path, const char *new_path);
int vfs_chdir(const char *path);
int vfs_getcwd(char *buf, size_t len);
int vfs_open(const char *path, int flags);
long vfs_read(int fd, void *buf, size_t len);
long vfs_write(int fd, const void *buf, size_t len);
long vfs_lseek(int fd, long offset, int whence);
int vfs_close(int fd);
int vfs_stat(const char *path, vfs_stat_t *st);
int vfs_fstat(int fd, vfs_stat_t *st);
long vfs_getdents(int fd, vfs_dirent_t *ents, size_t max_ents);
int vfs_is_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* VFS_H */
