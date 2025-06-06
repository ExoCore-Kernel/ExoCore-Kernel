// kernel/main.c

#include <stdint.h>
#include "multiboot.h"
#include "config.h"
#include "elf.h"
#include "console.h"
#include "serial.h"
#include "mem.h"
#include "panic.h"
#include "idt.h"

/* Entry point, called by boot.S (magic in RDI, mbi ptr in RSI) */
void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
    /* 1) Init consoles */
    serial_init();
    console_init();
    extern uint8_t end;
    mem_init((uint32_t)&end, 128 * 1024);
    idt_init();

    /* 2) Banner */
    serial_write("ExoCore booted\n");
    console_puts ("ExoCore booted\n");

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
    /* 5) Load & execute modules in-place */
    multiboot_module_t *mods = (void*)mbi->mods_addr;
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        uint8_t *base = (uint8_t*)mods[i].mod_start;

        /* Debug header */
        console_puts("Module "); console_udec(i); console_puts("\n");

        /* If not ELF, call raw */
        if (*(uint32_t*)base != ELF_MAGIC) {
            console_puts("  RAW-module -> jumping at 0x");
            console_uhex((uint64_t)(uintptr_t)base);
            console_puts("\n");
            ((void(*)(void))base)();
            console_puts("  RAW-module returned\n");
            continue;
        }

        /* ELF in-place */
        console_puts("  ELF-module\n");
        elf_header_t *eh = (elf_header_t*)base;
        elf_phdr_t   *ph = (elf_phdr_t*)(base + eh->e_phoff);

        /* Find first loadable segment’s p_vaddr */
        uint32_t first_vaddr = 0;
        for (int p = 0; p < eh->e_phnum; p++, ph++) {
            if (ph->p_type == PT_LOAD) {
                first_vaddr = ph->p_vaddr;
                break;
            }
        }

        /* Compute actual entry = base + (e_entry − first_vaddr) */
        uintptr_t entry = (uintptr_t)base + (eh->e_entry - first_vaddr);

        console_puts("  Jumping to entry 0x");
        console_uhex((uint64_t)entry);
        console_puts("\n");

        ((void(*)(void))entry)();
        console_puts("  ELF-module returned\n");
    }
#else
    console_puts("FEATURE_RUN_DIR disabled\n");
#endif

    console_puts("All done, halting\n");
    for (;;) __asm__("hlt");
}
