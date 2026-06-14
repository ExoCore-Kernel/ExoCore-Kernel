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
    SYS_READ = 13,
    SYS_GETPID = 14,
    SYS_GETPPID = 15,
    SYS_PROC_INFO = 16,
    SYS_PROC_LIST = 17,
    SYS_PROC_WAIT = 18,
    SYS_PROC_KILL = 19,
    SYS_WRITE_FD = 20,
    SYS_DUP = 21,
    SYS_DUP2 = 22,
    SYS_UPTIME_MS = 23,
    SYS_SLEEP_MS = 24,
    SYS_PIPE = 25,
    SYS_EXECVE = 26,
    SYS_PROC_SPAWN_EX = 27,
    SYS_DMESG_READ = 28,
    SYS_MEM_INFO = 29,
    SYS_SYNC = 30,
    SYS_IOCTL = 31,

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
    SYS_VFS_GETDENTS = 44,
    SYS_VFS_RMDIR = 45,
    SYS_VFS_ACCESS = 46,
    SYS_MOUNT_INFO = 47,
    SYS_DISK_LIST = 48,
    SYS_DISK_INFO = 49,
    SYS_REBOOT = 50,
    SYS_POWEROFF = 51,
    SYS_MPY_EXEC_FILE = 52,
    SYS_FB_INFO = 53,
    SYS_DISPLAY_MODE = 54,
    SYS_FB_CLEAR = 55,
    SYS_FB_DRAW_PIXEL = 56
};

typedef struct { uint32_t width; uint32_t height; uint32_t pitch; uint32_t bpp; uint32_t theme; uint32_t logs_visible; } syscall_fb_info_t;

#define SYS_DISPLAY_ENABLE_LOGS 1u
#define SYS_DISPLAY_DISABLE_LOGS 2u
#define SYS_DISPLAY_DARK_MODE 3u
#define SYS_DISPLAY_WHITE_MODE 4u

void syscall_init(void);
void enter_user_mode(void (*entry)(void), void *user_stack);

#ifdef __cplusplus
}
#endif

#endif /* SYSCALL_H */
