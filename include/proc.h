#ifndef PROC_H
#define PROC_H

#include <stddef.h>
#include <stdint.h>
#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PROC_NAME_MAX 31
#define PROC_MAX_FDS 16

#define PROC_STATE_UNUSED 0
#define PROC_STATE_READY  1
#define PROC_STATE_RUNNING 2
#define PROC_STATE_EXITED 3

typedef struct {
    int pid;
    int parent_pid;
    int state;
    int exit_status;
    int memctx;
    char name[PROC_NAME_MAX + 1];
    char cwd[VFS_MAX_PATH];
} proc_info_t;

void proc_init(void);
int proc_create(const char *name, int parent_pid);
int proc_set_current(int pid);
int proc_current_pid(void);
int proc_exit(int pid, int status);
int proc_wait(int parent_pid, int child_pid, int *status);
int proc_info(int pid, proc_info_t *info);
void *proc_alloc(int pid, size_t size);
int proc_free(int pid, void *ptr);
int proc_open(int pid, const char *path, int flags);
long proc_read(int pid, int fd, void *buf, size_t len);
long proc_write(int pid, int fd, const void *buf, size_t len);
long proc_lseek(int pid, int fd, long offset, int whence);
int proc_close(int pid, int fd);
int proc_stat(int pid, const char *path, vfs_stat_t *st);
int proc_fstat(int pid, int fd, vfs_stat_t *st);
long proc_getdents(int pid, int fd, vfs_dirent_t *ents, size_t max_ents);
int proc_chdir(int pid, const char *path);
int proc_getcwd(int pid, char *buf, size_t len);
int proc_mkdir(int pid, const char *path);
int proc_unlink(int pid, const char *path);
int proc_rename(int pid, const char *old_path, const char *new_path);

#ifdef __cplusplus
}
#endif

#endif /* PROC_H */
