#include "proc.h"
#include "memctx.h"
#include "memutils.h"

#define PROC_MAX 16

typedef struct {
    proc_info_t info;
    int used;
    int fds[PROC_MAX_FDS];
} proc_entry_t;

static proc_entry_t procs[PROC_MAX];
static int next_pid = 1;
static int current_pid = 0;

static proc_entry_t *find_proc(int pid) {
    for (int i = 0; i < PROC_MAX; ++i) {
        if (procs[i].used && procs[i].info.pid == pid)
            return &procs[i];
    }
    return 0;
}

static void copy_text(char *dst, size_t dst_len, const char *src) {
    size_t i = 0;
    if (!dst || dst_len == 0)
        return;
    if (!src)
        src = "";
    while (i + 1 < dst_len && src[i]) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

void proc_init(void) {
    memset(procs, 0, sizeof(procs));
    next_pid = 1;
    current_pid = 0;
    memctx_init();
}

int proc_create(const char *name, int parent_pid) {
    if (parent_pid != 0 && !find_proc(parent_pid))
        return -1;
    for (int i = 0; i < PROC_MAX; ++i) {
        if (!procs[i].used) {
            memset(&procs[i], 0, sizeof(procs[i]));
            procs[i].used = 1;
            procs[i].info.pid = next_pid++;
            procs[i].info.parent_pid = parent_pid;
            procs[i].info.state = PROC_STATE_READY;
            procs[i].info.exit_status = 0;
            procs[i].info.memctx = memctx_create(procs[i].info.pid, 0);
            copy_text(procs[i].info.name, sizeof(procs[i].info.name), name);
            copy_text(procs[i].info.cwd, sizeof(procs[i].info.cwd), "/");
            for (int fd = 0; fd < PROC_MAX_FDS; ++fd)
                procs[i].fds[fd] = -1;
            if (current_pid == 0)
                current_pid = procs[i].info.pid;
            return procs[i].info.pid;
        }
    }
    return -1;
}

int proc_set_current(int pid) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || proc->info.state == PROC_STATE_EXITED)
        return -1;
    if (current_pid) {
        proc_entry_t *old = find_proc(current_pid);
        if (old && old->info.state == PROC_STATE_RUNNING)
            old->info.state = PROC_STATE_READY;
    }
    current_pid = pid;
    proc->info.state = PROC_STATE_RUNNING;
    return 0;
}

int proc_current_pid(void) {
    return current_pid;
}

int proc_exit(int pid, int status) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc)
        return -1;
    for (int fd = 0; fd < PROC_MAX_FDS; ++fd) {
        if (proc->fds[fd] >= 0) {
            vfs_close(proc->fds[fd]);
            proc->fds[fd] = -1;
        }
    }
    proc->info.exit_status = status;
    proc->info.state = PROC_STATE_EXITED;
    if (current_pid == pid)
        current_pid = 0;
    return 0;
}

int proc_wait(int parent_pid, int child_pid, int *status) {
    proc_entry_t *child = find_proc(child_pid);
    if (!child || child->info.parent_pid != parent_pid || child->info.state != PROC_STATE_EXITED)
        return -1;
    if (status)
        *status = child->info.exit_status;
    memctx_destroy(child->info.memctx);
    memset(child, 0, sizeof(*child));
    return child_pid;
}

int proc_info(int pid, proc_info_t *info) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || !info)
        return -1;
    *info = proc->info;
    return 0;
}

void *proc_alloc(int pid, size_t size) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc)
        return 0;
    return memctx_alloc(proc->info.memctx, size);
}

int proc_free(int pid, void *ptr) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc)
        return -1;
    return memctx_free(proc->info.memctx, ptr);
}

static int apply_proc_cwd(proc_entry_t *proc) {
    if (!proc || proc->info.state == PROC_STATE_EXITED)
        return -1;
    return vfs_chdir(proc->info.cwd);
}

int proc_open(int pid, const char *path, int flags) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || proc->info.state == PROC_STATE_EXITED || apply_proc_cwd(proc) != 0)
        return -1;
    int real_fd = vfs_open(path, flags);
    if (real_fd < 0)
        return -1;
    for (int fd = 0; fd < PROC_MAX_FDS; ++fd) {
        if (proc->fds[fd] < 0) {
            proc->fds[fd] = real_fd;
            return fd;
        }
    }
    vfs_close(real_fd);
    return -1;
}

static int real_fd(proc_entry_t *proc, int fd) {
    if (!proc || fd < 0 || fd >= PROC_MAX_FDS)
        return -1;
    return proc->fds[fd];
}

long proc_read(int pid, int fd, void *buf, size_t len) {
    proc_entry_t *proc = find_proc(pid);
    return vfs_read(real_fd(proc, fd), buf, len);
}

long proc_write(int pid, int fd, const void *buf, size_t len) {
    proc_entry_t *proc = find_proc(pid);
    return vfs_write(real_fd(proc, fd), buf, len);
}

long proc_lseek(int pid, int fd, long offset, int whence) {
    proc_entry_t *proc = find_proc(pid);
    return vfs_lseek(real_fd(proc, fd), offset, whence);
}

int proc_close(int pid, int fd) {
    proc_entry_t *proc = find_proc(pid);
    int backing = real_fd(proc, fd);
    if (backing < 0)
        return -1;
    proc->fds[fd] = -1;
    return vfs_close(backing);
}


int proc_stat(int pid, const char *path, vfs_stat_t *st) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || proc->info.state == PROC_STATE_EXITED || apply_proc_cwd(proc) != 0)
        return -1;
    return vfs_stat(path, st);
}

int proc_fstat(int pid, int fd, vfs_stat_t *st) {
    proc_entry_t *proc = find_proc(pid);
    return vfs_fstat(real_fd(proc, fd), st);
}

long proc_getdents(int pid, int fd, vfs_dirent_t *ents, size_t max_ents) {
    proc_entry_t *proc = find_proc(pid);
    return vfs_getdents(real_fd(proc, fd), ents, max_ents);
}

int proc_chdir(int pid, const char *path) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || proc->info.state == PROC_STATE_EXITED || apply_proc_cwd(proc) != 0)
        return -1;
    if (vfs_chdir(path) != 0)
        return -1;
    return vfs_getcwd(proc->info.cwd, sizeof(proc->info.cwd));
}

int proc_getcwd(int pid, char *buf, size_t len) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || proc->info.state == PROC_STATE_EXITED || !buf || len == 0)
        return -1;
    size_t n = strlen(proc->info.cwd);
    if (n + 1 > len)
        return -1;
    memcpy(buf, proc->info.cwd, n + 1);
    return 0;
}


int proc_mkdir(int pid, const char *path) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || proc->info.state == PROC_STATE_EXITED || apply_proc_cwd(proc) != 0)
        return -1;
    return vfs_mkdir(path);
}

int proc_unlink(int pid, const char *path) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || proc->info.state == PROC_STATE_EXITED || apply_proc_cwd(proc) != 0)
        return -1;
    return vfs_unlink(path);
}

int proc_rename(int pid, const char *old_path, const char *new_path) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || proc->info.state == PROC_STATE_EXITED || apply_proc_cwd(proc) != 0)
        return -1;
    return vfs_rename(old_path, new_path);
}
