#include "syscall.h"
#include "idt.h"
#include "console.h"
#include "fs.h"
#include "mem.h"
#include "modexec.h"
#include "proc.h"
#include "vfs.h"
#include <stdint.h>

extern void *isr_stub_table[];
extern uint8_t end;

static int user_ptr_valid(const void *ptr, size_t len) {
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t k_end = (uintptr_t)&end;
    return p >= k_end && p + len >= p;
}

static int syscall_ptr_valid(const void *ptr, size_t len, int from_user) {
    return from_user ? user_ptr_valid(ptr, len) : (ptr != 0 || len == 0);
}

static int syscall_str_valid(const char *s, int from_user) {
    if (!syscall_ptr_valid(s, 1, from_user))
        return 0;
    for (size_t i = 0; i < VFS_MAX_PATH; ++i) {
        if (!syscall_ptr_valid(s + i, 1, from_user))
            return 0;
        if (s[i] == '\0')
            return 1;
    }
    return 0;
}

static int current_pid_or_none(void) {
    int pid = proc_current_pid();
    proc_info_t info;
    if (pid > 0 && proc_info(pid, &info) == 0 && info.state != PROC_STATE_EXITED)
        return pid;
    return 0;
}

static uint64_t syscall_dispatch_impl(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, int from_user) {
    switch (num) {
    case SYS_WRITE: {
        if (!syscall_ptr_valid((const void*)a1, a2, from_user)) return (uint64_t)-1;
        const char *s = (const char*)a1;
        for (size_t i = 0; i < a2; i++)
            console_putc(s[i]);
        return 0;
    }
    case SYS_EXIT:
        console_puts("User process exited\n");
        __asm__ volatile("cli; hlt");
        return 0;
    case SYS_MEM_ALLOC:
        return (uint64_t)(uintptr_t)mem_alloc((size_t)a1);
    case SYS_MEM_FREE:
        if (!syscall_ptr_valid((const void*)a1, a2, from_user)) return (uint64_t)-1;
        mem_free((void*)a1, (size_t)a2);
        return 0;
    case SYS_FS_READ:
        if (!syscall_ptr_valid((void*)a2, a3, from_user)) return (uint64_t)-1;
        return (uint64_t)fs_read((size_t)a1, (void*)a2, (size_t)a3);
    case SYS_FS_WRITE:
        if (!syscall_ptr_valid((const void*)a2, a3, from_user)) return (uint64_t)-1;
        return (uint64_t)fs_write((size_t)a1, (const void*)a2, (size_t)a3);
    case SYS_PROC_SPAWN:
        if (!syscall_str_valid((const char*)a1, from_user)) return (uint64_t)-1;
        return (uint64_t)modexec_run((const char*)a1);
    case SYS_OPEN: {
        if (!syscall_str_valid((const char*)a1, from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_open(pid, (const char*)a1, (int)a2)
                              : vfs_open((const char*)a1, (int)a2));
    }
    case SYS_READ: {
        if (a3 && !syscall_ptr_valid((void*)a2, (size_t)a3, from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_read(pid, (int)a1, (void*)a2, (size_t)a3)
                              : vfs_read((int)a1, (void*)a2, (size_t)a3));
    }
    case SYS_WRITE_FD: {
        if (a3 && !syscall_ptr_valid((const void*)a2, (size_t)a3, from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_write(pid, (int)a1, (const void*)a2, (size_t)a3)
                              : vfs_write((int)a1, (const void*)a2, (size_t)a3));
    }
    case SYS_CLOSE: {
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_close(pid, (int)a1) : vfs_close((int)a1));
    }
    case SYS_LSEEK: {
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_lseek(pid, (int)a1, (long)a2, (int)a3)
                              : vfs_lseek((int)a1, (long)a2, (int)a3));
    }
    case SYS_STAT: {
        if (!syscall_str_valid((const char*)a1, from_user) || !syscall_ptr_valid((void*)a2, sizeof(vfs_stat_t), from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_stat(pid, (const char*)a1, (vfs_stat_t*)a2)
                              : vfs_stat((const char*)a1, (vfs_stat_t*)a2));
    }
    case SYS_FSTAT: {
        if (!syscall_ptr_valid((void*)a2, sizeof(vfs_stat_t), from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_fstat(pid, (int)a1, (vfs_stat_t*)a2)
                              : vfs_fstat((int)a1, (vfs_stat_t*)a2));
    }
    case SYS_GETDENTS: {
        if (a3 && !syscall_ptr_valid((void*)a2, sizeof(vfs_dirent_t) * (size_t)a3, from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_getdents(pid, (int)a1, (vfs_dirent_t*)a2, (size_t)a3)
                              : vfs_getdents((int)a1, (vfs_dirent_t*)a2, (size_t)a3));
    }
    case SYS_MKDIR: {
        if (!syscall_str_valid((const char*)a1, from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_mkdir(pid, (const char*)a1) : vfs_mkdir((const char*)a1));
    }
    case SYS_UNLINK: {
        if (!syscall_str_valid((const char*)a1, from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_unlink(pid, (const char*)a1) : vfs_unlink((const char*)a1));
    }
    case SYS_RENAME: {
        if (!syscall_str_valid((const char*)a1, from_user) || !syscall_str_valid((const char*)a2, from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_rename(pid, (const char*)a1, (const char*)a2) : vfs_rename((const char*)a1, (const char*)a2));
    }
    case SYS_CHDIR: {
        if (!syscall_str_valid((const char*)a1, from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_chdir(pid, (const char*)a1) : vfs_chdir((const char*)a1));
    }
    case SYS_GETCWD: {
        if (!syscall_ptr_valid((void*)a1, (size_t)a2, from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        return (uint64_t)(pid ? proc_getcwd(pid, (char*)a1, (size_t)a2) : vfs_getcwd((char*)a1, (size_t)a2));
    }
    case SYS_WAITPID: {
        if (a2 && !syscall_ptr_valid((void*)a2, sizeof(int), from_user)) return (uint64_t)-1;
        int pid = current_pid_or_none();
        if (!pid) return (uint64_t)-1;
        return (uint64_t)proc_wait(pid, (int)a1, (int*)a2);
    }
    case SYS_EXIT_STATUS: {
        int pid = current_pid_or_none();
        if (!pid) return (uint64_t)-1;
        return (uint64_t)proc_exit(pid, (int)a1);
    }
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
    regs[0] = syscall_dispatch_impl(sysno, a1, a2, a3, 1);
}

uint64_t syscall_test_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    return syscall_dispatch_impl(num, a1, a2, a3, 0);
}

void syscall_init(void) {
    idt_set_user_gate(0x80, isr_stub_table[0x80]);
    register_irq_handler(0x80, syscall_irq_handler);
}
