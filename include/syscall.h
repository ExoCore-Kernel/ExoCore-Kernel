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
    SYS_PROC_SPAWN = 6
};

void syscall_init(void);
void enter_user_mode(void (*entry)(void), void *user_stack);

#ifdef __cplusplus
}
#endif

#endif /* SYSCALL_H */
