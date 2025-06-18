// kernel/main.c

#include <stdint.h>
#include "memutils.h"
#include "multiboot.h"
#include "config.h"
#include "elf.h"
#include "console.h"
#include "mem.h"
#include "panic.h"
#include "idt.h"
#include "serial.h"
#include "runstate.h"
#include "script.h"
#include "minipy.h"
#include "micropy.h"

static int debug_mode = 0;
static int userland_mode = 0;
volatile const char *current_program = "kernel";
volatile int current_user_app = 0;

static void parse_cmdline(const char *cmd) {
    if (!cmd) return;
    for (const char *p = cmd; *p; p++) {
        if (!debug_mode && !strncmp(p, "debug", 5))
            debug_mode = 1;
        if (!userland_mode && !strncmp(p, "userland", 8))
            userland_mode = 1;
    }
}



/* Entry point, called by boot.S (magic in RDI, mbi ptr in RSI) */
void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
    /* 1) Init consoles */
    serial_init();
    console_init();
    extern uint8_t end;
    mem_init((uintptr_t)&end, 128 * 1024);
    idt_init();

    if (mbi->flags & (1 << 2)) { // cmdline present
        parse_cmdline((const char*)(uintptr_t)mbi->cmdline);
    }

    if (debug_mode && userland_mode) {
        serial_write("Userland mode enabled\n");
    }

    /* 2) Banner */
    serial_write("ExoCore booted\n");
    console_puts("ExoCore booted\n");
    if (debug_mode) {
        serial_write("Debug mode enabled\n");
        console_puts("Debug mode enabled\n");
    }

    /* 3) Multiboot check */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("Invalid multiboot magic");
    }

    /* 4) Module count */
    serial_write("mods_count="); console_puts("mods_count=");
    // we want decimal printing, reuse console_udec
    console_udec(mbi->mods_count);
    console_putc('\n');
    serial_write("\n");
    if (mbi->mods_count == 0) {
        panic("No modules found");
    }

#if FEATURE_RUN_DIR
    /* 5) Load & execute modules. Modules are linked to run at
       * MODULE_BASE_ADDR, but GRUB may load them at arbitrary addresses.
       * Copy each module to the expected location before jumping. */
    multiboot_module_t *mods = (multiboot_module_t*)(uintptr_t)mbi->mods_addr;
    uint8_t *const load_addr = (uint8_t*)MODULE_BASE_ADDR;
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        const char *mstr = (const char*)(uintptr_t)mods[i].string;
        int is_user = (mstr && !strncmp(mstr, "userland", 8));
        if (userland_mode ? !is_user : is_user) {
            continue;
        }

        /* Copy module to fixed address */
        uint8_t *src  = (uint8_t*)(uintptr_t)mods[i].mod_start;
        uint32_t size = mods[i].mod_end - mods[i].mod_start;
        memcpy(load_addr, src, size);
        uint8_t *base = load_addr;

        /* check for TinyScript or MiniPy */
        int is_ts = 0, is_py = 0, is_mp = 0;
        if (mstr) {
            const char *p = mstr;
            while (*p) p++;
            if (p - mstr >= 3 && p[-3] == '.' && p[-2] == 't' && p[-1] == 's')
                is_ts = 1;
            if (p - mstr >= 3 && p[-3] == '.' && p[-2] == 'p' && p[-1] == 'y')
                is_py = 1;
            if (p - mstr >= 4 && p[-4] == "." && p[-3] == "m" && p[-2] == "p" && p[-1] == "y")
                is_mp = 1;
        }

        if (is_ts) {
            console_puts("  TinyScript module\n");
            if (debug_mode) serial_write("  TinyScript module\n");
            run_script((const char*)src, size);
            continue;
        }

        if (is_py) {
            console_puts("  MiniPy module\n");
            if (debug_mode) serial_write("  MiniPy module\n");
            run_minipy((const char*)src, size);
            continue;
        }
        if (is_mp) {
            console_puts("  MicroPython module\n");
            if (debug_mode) serial_write("  MicroPython module\n");
            run_micropy((const char*)src, size);
            continue;
        }

        /* Debug header */
        console_puts("Module "); console_udec(i); console_puts("\n");
        if (debug_mode) { serial_write("Module "); serial_udec(i); serial_write("\n"); }

        /* Skip non-ELF modules to avoid jumping into invalid code */
        if (*(uint32_t*)base != ELF_MAGIC) {
            console_puts("  Skipping non-ELF module\n");
            if (debug_mode) serial_write("  Skipping non-ELF module\n");
            continue;
        }

        console_puts("  ELF-module\n");
        if (debug_mode) serial_write("  ELF-module\n");

        elf_header_t *eh = (elf_header_t*)base;
        elf_phdr_t   *ph = (elf_phdr_t*)(base + eh->e_phoff);

        /* Determine base virtual address */
        uint32_t first_vaddr = 0;
        for (int p = 0; p < eh->e_phnum; p++) {
            if (ph[p].p_type == PT_LOAD) { first_vaddr = ph[p].p_vaddr; break; }
        }

        /* Load each PT_LOAD segment and zero BSS */
        uint32_t max_size = 0;
        for (int p = 0; p < eh->e_phnum; p++) {
            if (ph[p].p_type != PT_LOAD) continue;
            uint32_t off = ph[p].p_vaddr - first_vaddr;
            memcpy(base + off, src + ph[p].p_offset, ph[p].p_filesz);
            if (ph[p].p_memsz > ph[p].p_filesz)
                memset(base + off + ph[p].p_filesz, 0, ph[p].p_memsz - ph[p].p_filesz);
            if (off + ph[p].p_memsz > max_size)
                max_size = off + ph[p].p_memsz;
        }
        if (max_size > size)
            memset(base + size, 0, max_size - size);

        uintptr_t entry = (uintptr_t)base + (eh->e_entry - first_vaddr);

        console_puts("  Jumping to entry 0x");
        console_uhex((uint64_t)entry);
        console_puts("\n");
        if (debug_mode) { serial_write("  Jumping to entry 0x"); serial_uhex((uint64_t)entry); serial_write("\n"); }

        current_program = mstr ? mstr : "module";
        current_user_app = is_user;
        ((void(*)(void))entry)();
        current_program = "kernel";
        current_user_app = 0;
        console_puts("  ELF-module returned\n");
        if (debug_mode) serial_write("  ELF-module returned\n");
    }
#else
    console_puts("FEATURE_RUN_DIR disabled\n");
#endif

    console_puts("All done, halting\n");
    if (debug_mode) serial_write("All done, halting\n");
    for (;;) __asm__("hlt");
}
