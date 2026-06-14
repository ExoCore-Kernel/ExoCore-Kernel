#include "syscall.h"
#include "idt.h"
#include "console.h"
#include "fs.h"
#include "mem.h"
#include "proc.h"
#include "vfs.h"
#include "debuglog.h"
#include "io.h"
#include "memutils.h"
#include "micropython.h"
#include <stdint.h>

extern void *isr_stub_table[];
extern uint8_t end;

static int user_ptr_valid(const void *ptr, size_t len) {
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t k_end = (uintptr_t)&end;
    uintptr_t e = p + len;
    if (e < p)
        return 0;
    if (p >= k_end)
        return 1;
    proc_info_t info;
    if (proc_info(proc_current_pid(), &info) != 0)
        return 0;
    if (info.stack_pointer >= 4096) {
        uintptr_t stack_base = info.stack_pointer - 4096;
        if (p >= stack_base && e <= info.stack_pointer)
            return 1;
    }
    if (info.image_base && p >= info.image_base && e <= info.image_base + info.image_size)
        return 1;
    return 0;
}

static uint64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    switch (num) {
    case SYS_WRITE: {
        if (!user_ptr_valid((const void*)a1, a2)) return (uint64_t)-1;
        const char *s = (const char*)a1;
        for (size_t i = 0; i < a2; i++)
            console_putc(s[i]);
        return (uint64_t)a2;
    }
    case SYS_WRITE_FD: {
        int fd = (int)a1;
        if (!user_ptr_valid((const void*)a2, a3)) return (uint64_t)-1;
        const char *s = (const char*)a2;
        if (fd == 1 || fd == 2) {
            for (size_t i = 0; i < a3; i++)
                console_putc(s[i]);
            return (uint64_t)a3;
        }
        if (fd >= 3)
            return (uint64_t)proc_write(proc_current_pid(), fd, s, (size_t)a3);
        return (uint64_t)-1;
    }
    case SYS_READ:
        if (!user_ptr_valid((void*)a2, a3)) return (uint64_t)-1;
        if ((int)a1 == 0) {
            char *b = (char*)a2;
            for (size_t i = 0; i < a3; ++i) b[i] = console_getc();
            return a3;
        }
        return (uint64_t)proc_read(proc_current_pid(), (int)a1, (void*)a2, (size_t)a3);
    case SYS_EXIT:
        proc_exit(proc_current_pid(), (int)a1);
        return 0;
    case SYS_MEM_ALLOC:
        return (uint64_t)(uintptr_t)mem_alloc((size_t)a1);
    case SYS_MEM_FREE:
        if (!user_ptr_valid((const void*)a1, a2)) return (uint64_t)-1;
        mem_free((void*)a1, (size_t)a2);
        return 0;
    case SYS_FS_READ:
        if (!user_ptr_valid((void*)a2, a3)) return (uint64_t)-1;
        return (uint64_t)fs_read((size_t)a1, (void*)a2, (size_t)a3);
    case SYS_FS_WRITE:
        if (!user_ptr_valid((const void*)a2, a3)) return (uint64_t)-1;
        return (uint64_t)fs_write((size_t)a1, (const void*)a2, (size_t)a3);
    case SYS_PROC_SPAWN:
        if (!user_ptr_valid((const void*)a1, 1)) return (uint64_t)-1;
        return (uint64_t)proc_spawn_exec(proc_current_pid(), (const char*)a1);

    case SYS_GETPID:
        return (uint64_t)proc_current_pid();
    case SYS_GETPPID: {
        proc_info_t info;
        return proc_info(proc_current_pid(), &info) == 0 ? (uint64_t)info.parent_pid : (uint64_t)-1;
    }
    case SYS_PROC_INFO:
        if (!user_ptr_valid((void*)a2, sizeof(proc_info_t))) return (uint64_t)-1;
        return (uint64_t)proc_info((int)a1, (proc_info_t*)a2);
    case SYS_PROC_LIST:
        if (!user_ptr_valid((void*)a1, a2 * sizeof(proc_info_t))) return (uint64_t)-1;
        return (uint64_t)proc_list((proc_info_t*)a1, (size_t)a2);
    case SYS_PROC_WAIT:
        if (a2 && !user_ptr_valid((void*)a2, sizeof(int))) return (uint64_t)-1;
        return (uint64_t)proc_wait(proc_current_pid(), (int)a1, (int*)a2);
    case SYS_PROC_KILL:
        return (uint64_t)proc_kill((int)a1, (int)a2);
    case SYS_DUP:
        return (uint64_t)proc_dup(proc_current_pid(), (int)a1);
    case SYS_DUP2:
        return (uint64_t)proc_dup2(proc_current_pid(), (int)a1, (int)a2);
    case SYS_UPTIME_MS:
        return (uint64_t)(io_rdtsc() / 1000000ULL);
    case SYS_SLEEP_MS: {
        uint64_t end = io_rdtsc() + a1 * 1000000ULL;
        while (io_rdtsc() < end) {}
        return 0;
    }
    case SYS_PIPE:
        return (uint64_t)-1;
    case SYS_EXECVE:
    case SYS_PROC_SPAWN_EX:
        if (!user_ptr_valid((const void*)a1, 1)) return (uint64_t)-1;
        return (uint64_t)proc_spawn_exec(proc_current_pid(), (const char*)a1);
    case SYS_DMESG_READ: {
        if (!user_ptr_valid((void*)a1, a2)) return (uint64_t)-1;
        size_t len = debuglog_length();
        if (a3 >= len) return 0;
        size_t n = len - a3;
        if (n > a2) n = a2;
        memcpy((void*)a1, debuglog_buffer() + a3, n);
        return (uint64_t)n;
    }
    case SYS_MEM_INFO:
        if (!user_ptr_valid((void*)a1, sizeof(mem_info_t))) return (uint64_t)-1;
        mem_get_info((mem_info_t*)a1);
        return 0;
    case SYS_SYNC:
        debuglog_flush();
        return 0;
    case SYS_IOCTL:
        return 0;
    case SYS_FS_OPEN:
        if (!user_ptr_valid((const void*)a1, 1)) return (uint64_t)-1;
        return (uint64_t)fs_open((const char*)a1, (int)a2);
    case SYS_FS_READ_FD:
        if (!user_ptr_valid((void*)a2, a3)) return (uint64_t)-1;
        return (uint64_t)fs_read_fd((int)a1, (void*)a2, (size_t)a3);
    case SYS_FS_WRITE_FD:
        if (!user_ptr_valid((const void*)a2, a3)) return (uint64_t)-1;
        return (uint64_t)fs_write_fd((int)a1, (const void*)a2, (size_t)a3);
    case SYS_FS_LSEEK_FD:
        return (uint64_t)fs_lseek_fd((int)a1, (long)a2, (int)a3);
    case SYS_FS_CLOSE:
        return (uint64_t)fs_close((int)a1);
    case SYS_FS_FILE_SIZE:
        return (uint64_t)fs_file_size((int)a1);
    case SYS_VFS_OPEN:
        if (!user_ptr_valid((const void*)a1, 1)) return (uint64_t)-1;
        return (uint64_t)vfs_open((const char*)a1, (int)a2);
    case SYS_VFS_READ:
        if (!user_ptr_valid((void*)a2, a3)) return (uint64_t)-1;
        return (uint64_t)vfs_read((int)a1, (void*)a2, (size_t)a3);
    case SYS_VFS_WRITE:
        if (!user_ptr_valid((const void*)a2, a3)) return (uint64_t)-1;
        return (uint64_t)vfs_write((int)a1, (const void*)a2, (size_t)a3);
    case SYS_VFS_LSEEK:
        return (uint64_t)vfs_lseek((int)a1, (long)a2, (int)a3);
    case SYS_VFS_CLOSE:
        return (uint64_t)vfs_close((int)a1);
    case SYS_VFS_MKDIR:
        if (!user_ptr_valid((const void*)a1, 1)) return (uint64_t)-1;
        return (uint64_t)vfs_mkdir((const char*)a1);
    case SYS_VFS_UNLINK:
        if (!user_ptr_valid((const void*)a1, 1)) return (uint64_t)-1;
        return (uint64_t)vfs_unlink((const char*)a1);
    case SYS_VFS_RMDIR:
        if (!user_ptr_valid((const void*)a1, 1)) return (uint64_t)-1;
        return (uint64_t)vfs_rmdir((const char*)a1);
    case SYS_VFS_ACCESS:
        if (!user_ptr_valid((const void*)a1, 1)) return (uint64_t)-1;
        return (uint64_t)vfs_access((const char*)a1, (int)a2);
    case SYS_VFS_RENAME:
        if (!user_ptr_valid((const void*)a1, 1) || !user_ptr_valid((const void*)a2, 1)) return (uint64_t)-1;
        return (uint64_t)vfs_rename((const char*)a1, (const char*)a2);
    case SYS_VFS_CHDIR:
        if (!user_ptr_valid((const void*)a1, 1)) return (uint64_t)-1;
        return (uint64_t)vfs_chdir((const char*)a1);
    case SYS_VFS_GETCWD:
        if (!user_ptr_valid((void*)a1, a2)) return (uint64_t)-1;
        return (uint64_t)vfs_getcwd((char*)a1, (size_t)a2);
    case SYS_VFS_STAT:
        if (!user_ptr_valid((const void*)a1, 1) || !user_ptr_valid((void*)a2, sizeof(vfs_stat_t))) return (uint64_t)-1;
        return (uint64_t)vfs_stat((const char*)a1, (vfs_stat_t*)a2);
    case SYS_VFS_FSTAT:
        if (!user_ptr_valid((void*)a2, sizeof(vfs_stat_t))) return (uint64_t)-1;
        return (uint64_t)vfs_fstat((int)a1, (vfs_stat_t*)a2);
    case SYS_VFS_GETDENTS:
        if (!user_ptr_valid((void*)a2, a3 * sizeof(vfs_dirent_t))) return (uint64_t)-1;
        return (uint64_t)vfs_getdents((int)a1, (vfs_dirent_t*)a2, (size_t)a3);
    case SYS_MOUNT_INFO:
    case SYS_DISK_LIST:
    case SYS_DISK_INFO:
        return 0;
    case SYS_MPY_EXEC_FILE: {
        if (!user_ptr_valid((const void*)a1, 1)) return (uint64_t)-1;
        const char *path = (const char*)a1;
        vfs_stat_t st;
        if (vfs_stat(path, &st) != 0 || st.type != VFS_TYPE_FILE || st.size == 0) return (uint64_t)-1;
        char *buf = (char*)mem_alloc(st.size + 1);
        if (!buf) return (uint64_t)-1;
        int fd = vfs_open(path, VFS_O_RDONLY);
        if (fd < 0) { mem_free(buf, st.size + 1); return (uint64_t)-1; }
        long got = vfs_read(fd, buf, st.size);
        vfs_close(fd);
        if (got != (long)st.size) { mem_free(buf, st.size + 1); return (uint64_t)-1; }
        buf[st.size] = 0;
        mp_runtime_exec(buf, st.size, path);
        mem_free(buf, st.size + 1);
        return 0;
    }
    case SYS_REBOOT:
    case SYS_POWEROFF:
        return 0;
    default:
        return (uint64_t)-1;
    }
}

static void syscall_irq_handler(uint32_t num, uint32_t err, uint64_t rsp) {
    (void)num; (void)err;
    uint64_t *regs = (uint64_t*)rsp;
    uint64_t sysno = regs[0];
    uint64_t a1 = regs[6];
    uint64_t a2 = regs[5];
    uint64_t a3 = regs[3];
    regs[0] = syscall_dispatch(sysno, a1, a2, a3);
}

void syscall_init(void) {
    idt_set_user_gate(0x80, isr_stub_table[0x80]);
    register_irq_handler(0x80, syscall_irq_handler);
}
