#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SYS_WRITE = 0,
    SYS_EXIT = 1,
    SYS_MEM_ALLOC = 2,
    SYS_MEM_FREE = 3,
    SYS_FS_READ = 4,
    SYS_FS_WRITE = 5,
    SYS_PROC_SPAWN = 6,
    SYS_FS_OPEN = 7,
    SYS_FS_READ_FD = 8,
    SYS_FS_WRITE_FD = 9,
    SYS_FS_LSEEK_FD = 10,
    SYS_FS_CLOSE = 11,
    SYS_FS_FILE_SIZE = 12,

    /* VFS syscalls intentionally use separate names/numbers so the FAT32
     * FS syscall surface remains distinct from the in-kernel VFS layer.
     */
    SYS_VFS_OPEN = 32,
    SYS_VFS_READ = 33,
    SYS_VFS_WRITE = 34,
    SYS_VFS_LSEEK = 35,
    SYS_VFS_CLOSE = 36,
    SYS_VFS_MKDIR = 37,
    SYS_VFS_UNLINK = 38,
    SYS_VFS_RENAME = 39,
    SYS_VFS_CHDIR = 40,
    SYS_VFS_GETCWD = 41,
    SYS_VFS_STAT = 42,
    SYS_VFS_FSTAT = 43,
    SYS_VFS_GETDENTS = 44
};

void syscall_init(void);
void enter_user_mode(void (*entry)(void), void *user_stack);

#ifdef __cplusplus
}
#endif

#endif /* SYSCALL_H */
