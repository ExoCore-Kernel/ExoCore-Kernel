#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

/* magic value passed by GRUB */
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
} multiboot_module_t;

typedef struct {
    uint32_t flags;
    uint32_t mem_lower, mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    /* remaining fields omitted */
} multiboot_info_t;

#endif /* MULTIBOOT_H */

