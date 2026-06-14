#ifndef PROC_H
#define PROC_H

#include <stddef.h>
#include <stdint.h>
#include "vfs.h"
#include "elf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PROC_NAME_MAX 31
#define PROC_MAX_FDS 16
#define PROC_EXEC_PATH_MAX VFS_MAX_PATH

#define PROC_STATE_UNUSED 0
#define PROC_STATE_READY  1
#define PROC_STATE_RUNNING 2
#define PROC_STATE_EXITED 3
#define PROC_STATE_ZOMBIE 4
#define PROC_STATE_DEAD 5

#define PROC_ERR_NOT_FOUND -3
#define PROC_ERR_PROTECTED -13
#define PROC_ERR_NOMEM -12
#define PROC_ERR_INVALID_STATE -22

typedef struct {
    int pid;
    int parent_pid;
    int state;
    int exit_status;
    int memctx;
    char name[PROC_NAME_MAX + 1];
    char cwd[VFS_MAX_PATH];
    char exe_path[PROC_EXEC_PATH_MAX];
    uintptr_t entry_point;
    uintptr_t image_base;
    size_t image_size;
    uintptr_t requested_base;
    uintptr_t requested_end;
    uintptr_t stack_pointer;
} proc_info_t;

void proc_init(void);
int proc_create(const char *name, int parent_pid);
int proc_attach_image(int pid, const char *path, const elf_image_t *image);
int proc_start_flat(int pid);
int proc_spawn_exec(int parent_pid, const char *path);
int proc_set_current(int pid);
int proc_current_pid(void);
int proc_current_valid(void);
int proc_exit(int pid, int status);
int proc_wait(int parent_pid, int child_pid, int *status);
int proc_kill(int pid, int status);
long proc_list(proc_info_t *infos, size_t max_infos);
int proc_dup(int pid, int oldfd);
int proc_dup2(int pid, int oldfd, int newfd);
int proc_info(int pid, proc_info_t *info);
void *proc_alloc(int pid, size_t size);
int proc_free(int pid, void *ptr);
int proc_open(int pid, const char *path, int flags);
long proc_read(int pid, int fd, void *buf, size_t len);
long proc_write(int pid, int fd, const void *buf, size_t len);
long proc_lseek(int pid, int fd, long offset, int whence);
int proc_close(int pid, int fd);

#ifdef __cplusplus
}
#endif

#endif /* PROC_H */
