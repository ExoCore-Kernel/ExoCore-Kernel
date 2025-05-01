#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include "multiboot.h"
#include "config.h"
#include "elf.h"
#include "mem.h"
#include "console.h"

/* I/O port access for serial */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* Serial console routines */
static void serial_init(void) {
    outb(0x3F8+1, 0x00); /* disable interrupts */
    outb(0x3F8+3, 0x80); /* enable DLAB */
    outb(0x3F8+0, 0x01); /* 115200 baud divisor low */
    outb(0x3F8+1, 0x00); /* divisor high */
    outb(0x3F8+3, 0x03); /* 8N1 */
    outb(0x3F8+2, 0xC7); /* FIFO */
    outb(0x3F8+4, 0x0B); /* IRQs */
}

static void serial_putc(char c) {
    while (!(inb(0x3F8+5) & 0x20));
    outb(0x3F8, c);
}

static void serial_write(const char *s) {
    for (; *s; ++s) serial_putc(*s);
}

static void serial_udec(uint32_t v) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (!v) buf[i--] = '0';
    while (v) {
        buf[i--] = '0' + (v % 10);
        v /= 10;
    }
    serial_write(&buf[i+1]);
}

/* Kernel panic: print on both consoles and halt */
static void panic(const char *msg) {
    serial_write("Panic: ");
    console_puts("Panic: ");
    serial_write(msg);
    console_puts(msg);
    serial_write("\n");
    console_puts("\n");
    for (;;) __asm__("hlt");
}

/* Entry point called by boot.S */
void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
    /* 1) Init consoles */
    serial_init();
    console_init();

    /* 2) Banner */
    serial_write("ExoCore booted\n");
    console_puts("ExoCore booted\n");

    /* 3) Check multiboot magic */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("Invalid multiboot magic");
    }

    /* 4) Initialize heap at 2MiB */
    mem_init(0x200000, 0x200000);
    serial_write("Heap @0x200000\n");
    console_puts("Heap @0x200000\n");

    /* 5) Ensure we have at least one module */
    if (mbi->mods_count == 0) {
        panic("No modules found");
    }

    /* 6) Load and execute modules */
    serial_write("Loading modules...\n");
    console_puts("Loading modules...\n");

#if FEATURE_RUN_DIR
    multiboot_module_t *mods = (void*)mbi->mods_addr;
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        console_puts(" Module ");
        console_udec(i);
        console_puts("\n");

        uint8_t *base = (uint8_t*)mods[i].mod_start;
        uint32_t mod_size = mods[i].mod_end - mods[i].mod_start;

        /* RAW-BIN fallback */
        if (*(uint32_t*)base != ELF_MAGIC) {
            console_puts("  Raw module\n");
            ((void(*)(void))base)();
            console_puts("  Raw returned\n");
            continue;
        }

        /* ELF32 path */
        console_puts("  ELF module\n");
        elf_header_t *eh = (elf_header_t*)base;
        elf_phdr_t *ph = (elf_phdr_t*)(base + eh->e_phoff);

        /* 6.1) Scan for min/max vaddr */
        uint32_t min_v = UINT32_MAX, max_v = 0;
        for (int p = 0; p < eh->e_phnum; p++, ph++) {
            if (ph->p_type != PT_LOAD) continue;
            if (ph->p_vaddr < min_v) min_v = ph->p_vaddr;
            uint32_t top = ph->p_vaddr + ph->p_memsz;
            if (top > max_v) max_v = top;
        }
        uint32_t total = max_v - min_v;

        /* 6.2) Allocate chunk */
        void *load_addr = mem_alloc(total);

        /* 6.3) Copy segments */
        ph = (elf_phdr_t*)(base + eh->e_phoff);
        for (int p = 0; p < eh->e_phnum; p++, ph++) {
            if (ph->p_type != PT_LOAD) continue;
            console_puts("   PT_LOAD segment\n");
            uint8_t *dst = (uint8_t*)load_addr + (ph->p_vaddr - min_v);
            uint8_t *src = base + ph->p_offset;
            /* file data */
            for (uint32_t b = 0; b < ph->p_filesz; b++) dst[b] = src[b];
            /* zero BSS */
            for (uint32_t b = ph->p_filesz; b < ph->p_memsz; b++) dst[b] = 0;
        }

        /* 6.4) Jump to relocated entry */
        console_puts("  Jump to ELF entry\n");
        uintptr_t entry = (uintptr_t)load_addr + (eh->e_entry - min_v);
        ((void(*)(void))entry)();
        console_puts("  ELF returned\n");
    }
#else
    console_puts("FEATURE_RUN_DIR disabled\n");
#endif

    console_puts("All done, halting\n");
    for (;;) __asm__("hlt");
}
