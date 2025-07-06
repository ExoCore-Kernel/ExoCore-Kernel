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
#include "debuglog.h"
#include "io.h"
#include "runstate.h"
#include "script.h"
#include "micropython.h"
#include "mpy_loader.h"

int debug_mode = 0;
static int userland_mode = 0;
int mp_vga_output = 1;
volatile const char *current_program = "kernel";
volatile int current_user_app = 0;

/* Symbol defined by the linker marking the end of the kernel image */
extern uint8_t end;

static inline void dbg_puts(const char *s) { if (debug_mode) console_puts(s); }
static inline void dbg_putc(char c) { if (debug_mode) console_putc(c); }
static inline void dbg_udec(uint32_t v) { if (debug_mode) console_udec(v); }
static inline void dbg_uhex(uint64_t v) { if (debug_mode) console_uhex(v); }

static void parse_cmdline(const char *cmd) {
    if (!cmd) return;
    for (const char *p = cmd; *p; p++) {
        if (!debug_mode && !strncmp(p, "debug", 5))
            debug_mode = 1;
        if (!userland_mode && !strncmp(p, "userland", 8))
            userland_mode = 1;
        if (mp_vga_output && !strncmp(p, "nompvga", 7))
            mp_vga_output = 0;
    }
}



/* Entry point, called by boot.S (magic in RDI, mbi ptr in RSI) */
void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
    debuglog_print_timestamp();
    dbg_puts("kernel_main start\n");
    /* 1) Init consoles */
    serial_init();
    debuglog_print_timestamp();
    dbg_puts("serial_init done\n");
    console_init();
    debuglog_print_timestamp();
    dbg_puts("console_init done\n");
    debuglog_print_timestamp();
    dbg_puts("GRUB handed control to kernel\n");
    if (debug_mode) {
        debuglog_print_timestamp();
        uint64_t rsp;
        __asm__("mov %%rsp, %0" : "=r"(rsp));
        dbg_puts("stack_ptr=0x");
        dbg_uhex(rsp);
        dbg_putc('\n');
        dbg_puts("mem_lower=");
        dbg_udec(mbi->mem_lower);
        dbg_puts(" mem_upper=");
        dbg_udec(mbi->mem_upper);
        dbg_putc('\n');
        uint32_t eax, ebx, ecx, edx;
        io_cpuid(0, &eax, &ebx, &ecx, &edx);
        char vendor[13];
        *(uint32_t*)&vendor[0] = ebx;
        *(uint32_t*)&vendor[4] = edx;
        *(uint32_t*)&vendor[8] = ecx;
        vendor[12] = 0;
        dbg_puts("cpu_vendor=");
        dbg_puts(vendor);
        dbg_putc('\n');
        io_cpuid(1, &eax, &ebx, &ecx, &edx);
        dbg_puts("cpuid_feat edx=0x");
        dbg_uhex(edx);
        dbg_puts(" ecx=0x");
        dbg_uhex(ecx);
        dbg_putc('\n');
        dbg_puts("Initializing memory manager at 0x");
        dbg_uhex((uint64_t)(uintptr_t)&end);
        dbg_puts("\n");
        debuglog_print_timestamp();
        mem_init((uintptr_t)&end, 128 * 1024);
        debuglog_memdump(&end, 64);
        dbg_puts("heap_end=0x");
        dbg_uhex((uint64_t)(uintptr_t)&end + 128*1024);
        dbg_putc('\n');
        idt_init();
        debuglog_print_timestamp();
        dbg_puts("IDT initialized\n");
        debuglog_memdump((void*)idt_init, 64);
        debuglog_init();
        debuglog_print_timestamp();
        dbg_puts("Debug log ready\n");
        debuglog_memdump(debuglog_buffer(), 64);
    } else {
        mem_init((uintptr_t)&end, 128 * 1024);
        idt_init();
        debuglog_init();
    }

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

    mp_runtime_init();
    mpymod_load_all();

#if FEATURE_RUN_DIR
    /* 5) Load & execute modules. Modules are linked to run at
       * MODULE_BASE_ADDR, but GRUB may load them at arbitrary addresses.
       * Copy each module to the expected location before jumping. */
    multiboot_module_t *mods = (multiboot_module_t*)(uintptr_t)mbi->mods_addr;
    uint8_t *const load_addr = (uint8_t*)MODULE_BASE_ADDR;
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        debuglog_print_timestamp();
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

        /* check for TinyScript or MicroPython */
        int is_ts = 0;
        int is_py = 0;
        int is_mpy = 0;
        if (mstr) {
            /* the module string may contain arguments like
               "userland /boot/foo.mpy". Use only the last
               space-separated token when checking the file
               extension so MicroPython modules are detected
               correctly. */
            const char *name = mstr;
            for (const char *p = mstr; *p; p++)
                if (*p == ' ') name = p + 1;
            const char *p = name; while (*p) p++;
            int len = p - name;
            #define LOWER(c) ((c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c))
            if (len >= 3) {
                if (LOWER(name[len-3]) == '.' && LOWER(name[len-2]) == 't' && LOWER(name[len-1]) == 's')
                    is_ts = 1;
                if (LOWER(name[len-3]) == '.' && LOWER(name[len-2]) == 'p' && LOWER(name[len-1]) == 'y')
                    is_py = 1;
            }
            if (len >= 4 &&
                LOWER(name[len-4]) == '.' &&
                LOWER(name[len-3]) == 'm' &&
                LOWER(name[len-2]) == 'p' &&
                LOWER(name[len-1]) == 'y')
                is_mpy = 1;
            #undef LOWER
        }

        if (is_ts) {
#if MODULE_MODE != MODULE_MODE_MICROPYTHON
            dbg_puts("  TinyScript module\n");
            if (debug_mode) serial_write("  TinyScript module\n");
            run_script((const char*)src, size);
            continue;
#else
            dbg_puts("  TinyScript skipped (micropython-only mode)\n");
            if (debug_mode) serial_write("  TinyScript skipped (micropython-only mode)\n");
            continue;
#endif
        }

        if (is_py) {
#if MODULE_MODE != MODULE_MODE_ELF
            dbg_puts("  MicroPython script\n");
            if (debug_mode) serial_write("  MicroPython script\n");
            mp_runtime_exec((const char*)src, size);
            continue;
#else
            dbg_puts("  MicroPython script skipped (ELF-only mode)\n");
            if (debug_mode) serial_write("  MicroPython script skipped (ELF-only mode)\n");
            continue;
#endif
        }

        if (is_mpy) {
#if MODULE_MODE != MODULE_MODE_ELF
            dbg_puts("  MicroPython bytecode\n");
            if (debug_mode) serial_write("  MicroPython bytecode\n");
            mp_runtime_exec_mpy(src, size);
            continue;
#else
            dbg_puts("  MicroPython bytecode skipped (ELF-only mode)\n");
            if (debug_mode) serial_write("  MicroPython bytecode skipped (ELF-only mode)\n");
            continue;
#endif
        }

        /* Debug header */
        dbg_puts("Module "); if (debug_mode) serial_write("Module ");
        dbg_udec(i); if (debug_mode) serial_udec(i); dbg_putc('\n'); if (debug_mode) serial_write("\n");
        dbg_puts("  src=0x"); dbg_uhex((uint64_t)(uintptr_t)src);
        dbg_puts(" size="); dbg_udec(size); dbg_putc('\n');
        debuglog_hexdump(src, size > 64 ? 64 : size);
        debuglog_memdump(base, size > 128 ? 128 : size);

        /* Skip unknown non-ELF modules or run as MicroPython */
        if (*(uint32_t*)base != ELF_MAGIC) {
#if MODULE_MODE != MODULE_MODE_ELF
            if (size >= 7 && memcmp(src, "#mpyexo", 7) == 0) {
                console_puts("  MicroPython script\n");
                if (debug_mode) serial_write("  MicroPython script\n");
                const uint8_t *p = src + 7;
                const uint8_t *end = src + size;
                while (p < end && *p != '\n' && *p != '\r') p++;
                if (p < end && *p == '\r') p++;
                if (p < end && *p == '\n') p++;
                mp_runtime_exec((const char*)p, end - p);
                continue;
            }
#endif
            dbg_puts("  Non-ELF module skipped\n");
            if (debug_mode) serial_write("  Non-ELF module skipped\n");
            continue;
        }

        dbg_puts("  ELF-module\n");
        if (debug_mode) serial_write("  ELF-module\n");
#if MODULE_MODE == MODULE_MODE_MICROPYTHON
        dbg_puts("  ELF loading disabled in micropython-only mode\n");
        if (debug_mode) serial_write("  ELF loading disabled in micropython-only mode\n");
        continue;
#else

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
            dbg_puts("   segment off=0x"); dbg_uhex(ph[p].p_offset);
            dbg_puts(" filesz="); dbg_udec(ph[p].p_filesz);
            dbg_puts(" memsz="); dbg_udec(ph[p].p_memsz); dbg_putc('\n');
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

        dbg_puts("  Jumping to entry 0x");
        dbg_uhex((uint64_t)entry);
        dbg_putc('\n');
        if (debug_mode) { serial_write("  Jumping to entry 0x"); serial_uhex((uint64_t)entry); serial_write("\n"); }

        current_program = mstr ? mstr : "module";
        current_user_app = is_user;
        ((void(*)(void))entry)();
        current_program = "kernel";
        current_user_app = 0;
        dbg_puts("  ELF-module returned\n");
        dbg_puts(" heap_free=");
        dbg_udec(mem_heap_free());
        dbg_putc('\n');
        if (debug_mode) serial_write("  ELF-module returned\n");
#endif /* MODULE_MODE == MODULE_MODE_MICROPYTHON */
    }
    mp_runtime_deinit();
#else
    console_puts("FEATURE_RUN_DIR disabled\n");
#endif

    console_puts("All done, halting\n");
    console_puts("final_heap_free=");
    console_udec(mem_heap_free());
    console_putc('\n');
    if (debug_mode) serial_write("All done, halting\n");
    debuglog_save_file();
    debuglog_flush();
    __asm__ volatile("cli");
    for (;;) __asm__("hlt");
}
