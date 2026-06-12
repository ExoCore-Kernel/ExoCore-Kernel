#include "proc.h"
#include "memctx.h"
#include "memutils.h"
#include "console.h"

#define PROC_MAX 16
#define PROC_STACK_SIZE 4096

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

static const char *basename_from_path(const char *path) {
    const char *base = path ? path : "";
    for (const char *p = base; *p; ++p) {
        if (*p == '/')
            base = p + 1;
    }
    return *base ? base : path;
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
    if (parent_pid == 0 && next_pid != 1)
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

int proc_attach_image(int pid, const char *path, const elf_image_t *image) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || !image || !image->load_base || !image->entry)
        return -1;
    proc->info.entry_point = (uintptr_t)image->entry;
    proc->info.image_base = (uintptr_t)image->load_base;
    proc->info.image_size = image->load_size;
    proc->info.requested_base = image->requested_base;
    proc->info.requested_end = image->requested_end;
    copy_text(proc->info.exe_path, sizeof(proc->info.exe_path), path);
    void *stack = proc_alloc(pid, PROC_STACK_SIZE);
    if (!stack)
        return -1;
    proc->info.stack_pointer = (uintptr_t)stack + PROC_STACK_SIZE;
    return 0;
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

int proc_start_flat(int pid) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || !proc->info.entry_point || proc->info.state == PROC_STATE_EXITED)
        return -1;
    int previous = current_pid;
    if (proc_set_current(pid) != 0)
        return -1;
    void (*entry)(void) = (void (*)(void))proc->info.entry_point;
    entry();
    proc = find_proc(pid);
    if (proc && proc->info.state == PROC_STATE_RUNNING)
        proc->info.state = PROC_STATE_READY;
    if (previous && find_proc(previous) && previous != pid)
        proc_set_current(previous);
    else if (current_pid == pid)
        current_pid = 0;
    return 0;
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
    if (pid != 1) {
        for (int i = 0; i < PROC_MAX; ++i) {
            if (procs[i].used && procs[i].info.parent_pid == pid)
                procs[i].info.parent_pid = 1;
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

static int read_vfs_file(const char *path, void *buf, size_t size) {
    int fd = vfs_open(path, VFS_O_RDONLY);
    if (fd < 0)
        return -1;
    long got = vfs_read(fd, buf, size);
    vfs_close(fd);
    return got == (long)size ? 0 : -1;
}

int proc_spawn_exec(int parent_pid, const char *path) {
    if (!path || !find_proc(parent_pid))
        return -1;
    vfs_stat_t st;
    if (vfs_stat(path, &st) != 0 || st.type != VFS_TYPE_FILE || st.size == 0)
        return -1;
    int pid = proc_create(basename_from_path(path), parent_pid);
    if (pid < 0)
        return -1;
    void *file = proc_alloc(pid, st.size);
    if (!file)
        return -1;
    if (read_vfs_file(path, file, st.size) != 0)
        return -1;
    elf_image_t image;
    if (elf_load_process_image(pid, file, st.size, &image) != 0)
        return -1;
    if (proc_attach_image(pid, path, &image) != 0)
        return -1;
    return proc_start_flat(pid) == 0 ? pid : -1;
}

int proc_open(int pid, const char *path, int flags) {
    proc_entry_t *proc = find_proc(pid);
    if (!proc || proc->info.state == PROC_STATE_EXITED)
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
