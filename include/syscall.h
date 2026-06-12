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
    SYS_OPEN = 7,
    SYS_READ = 8,
    SYS_WRITE_FD = 9,
    SYS_CLOSE = 10,
    SYS_LSEEK = 11,
    SYS_STAT = 12,
    SYS_FSTAT = 13,
    SYS_GETDENTS = 14,
    SYS_MKDIR = 15,
    SYS_UNLINK = 16,
    SYS_RENAME = 17,
    SYS_CHDIR = 18,
    SYS_GETCWD = 19,
    SYS_WAITPID = 20,
    SYS_EXIT_STATUS = 21
};

void syscall_init(void);
uint64_t syscall_test_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3);
void enter_user_mode(void (*entry)(void), void *user_stack);

#ifdef __cplusplus
}
#endif

#endif /* SYSCALL_H */
