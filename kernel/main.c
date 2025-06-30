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

int debug_mode = 0;
static int userland_mode = 0;
int mp_vga_output = 1;
volatile const char *current_program = "kernel";
volatile int current_user_app = 0;

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
    console_puts("kernel_main start\n");
    /* 1) Init consoles */
    serial_init();
    debuglog_print_timestamp();
    console_puts("serial_init done\n");
    console_init();
    debuglog_print_timestamp();
    console_puts("console_init done\n");
    debuglog_print_timestamp();
    console_puts("GRUB handed control to kernel\n");
    debuglog_print_timestamp();
    uint64_t rsp;
    __asm__("mov %%rsp, %0" : "=r"(rsp));
    console_puts("stack_ptr=0x");
    console_uhex(rsp);
    console_putc('\n');
    console_puts("mem_lower=");
    console_udec(mbi->mem_lower);
    console_puts(" mem_upper=");
    console_udec(mbi->mem_upper);
    console_putc('\n');
    uint32_t eax, ebx, ecx, edx;
    io_cpuid(0, &eax, &ebx, &ecx, &edx);
    char vendor[13];
    *(uint32_t*)&vendor[0] = ebx;
    *(uint32_t*)&vendor[4] = edx;
    *(uint32_t*)&vendor[8] = ecx;
    vendor[12] = 0;
    console_puts("cpu_vendor=");
    console_puts(vendor);
    console_putc('\n');
    io_cpuid(1, &eax, &ebx, &ecx, &edx);
    console_puts("cpuid_feat edx=0x");
    console_uhex(edx);
    console_puts(" ecx=0x");
    console_uhex(ecx);
    console_putc('\n');
    extern uint8_t end;
    console_puts("Initializing memory manager at 0x");
    console_uhex((uint64_t)(uintptr_t)&end);
    console_puts("\n");
    debuglog_print_timestamp();
    mem_init((uintptr_t)&end, 128 * 1024);
    debuglog_memdump(&end, 64);
    console_puts("heap_end=0x");
    console_uhex((uint64_t)(uintptr_t)&end + 128*1024);
    console_putc('\n');
    idt_init();
    debuglog_print_timestamp();
    console_puts("IDT initialized\n");
    debuglog_memdump((void*)idt_init, 64);
    debuglog_init();
    debuglog_print_timestamp();
    console_puts("Debug log ready\n");
    debuglog_memdump(debuglog_buffer(), 64);

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
            console_puts("  TinyScript module\n");
            if (debug_mode) serial_write("  TinyScript module\n");
            run_script((const char*)src, size);
            continue;
        }

        if (is_py) {
            console_puts("  MicroPython script\n");
            if (debug_mode) serial_write("  MicroPython script\n");
            run_micropython((const char*)src, size);
            continue;
        }

        if (is_mpy) {
            console_puts("  MicroPython bytecode\n");
            if (debug_mode) serial_write("  MicroPython bytecode\n");
            run_micropython_mpy(src, size);
            continue;
        }

        /* Debug header */
        console_puts("Module "); console_udec(i); console_puts("\n");
        if (debug_mode) { serial_write("Module "); serial_udec(i); serial_write("\n"); }
        console_puts("  src=0x"); console_uhex((uint64_t)(uintptr_t)src);
        console_puts(" size="); console_udec(size); console_puts("\n");
        debuglog_hexdump(src, size > 64 ? 64 : size);
        debuglog_memdump(base, size > 128 ? 128 : size);

        /* Skip unknown non-ELF modules */
        if (*(uint32_t*)base != ELF_MAGIC) {
            console_puts("  Non-ELF module skipped\n");
            if (debug_mode) serial_write("  Non-ELF module skipped\n");
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
            console_puts("   segment off=0x"); console_uhex(ph[p].p_offset);
            console_puts(" filesz="); console_udec(ph[p].p_filesz);
            console_puts(" memsz="); console_udec(ph[p].p_memsz); console_puts("\n");
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
        console_puts(" heap_free=");
        console_udec(mem_heap_free());
        console_putc('\n');
        if (debug_mode) serial_write("  ELF-module returned\n");
    }
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
    for (;;) __asm__("hlt");
}
