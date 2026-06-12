#include "syscall.h"
#include "idt.h"
#include "console.h"
#include "fs.h"
#include "mem.h"
#include "modexec.h"
#include "vfs.h"
#include <stdint.h>

extern void *isr_stub_table[];
extern uint8_t end;

static int user_ptr_valid(const void *ptr, size_t len) {
    uintptr_t p = (uintptr_t)ptr;
    uintptr_t k_end = (uintptr_t)&end;
    return p >= k_end && p + len >= p;
}

static uint64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    switch (num) {
    case SYS_WRITE: {
        if (!user_ptr_valid((const void*)a1, a2)) return (uint64_t)-1;
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
        return (uint64_t)modexec_run((const char*)a1);
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

