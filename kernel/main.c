#include <stdint.h>
#include "multiboot.h"
#include "config.h"

static volatile char *video = (char*)0xB8000;

/* Entry called by boot.S: magic in EAX, mbi pointer in EBX */
void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
    /* Verify multiboot */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        video[0] = '!';
        for (;;) __asm__("hlt");
    }

    /* Mark kernel started */
    video[0] = 'E';

#if FEATURE_RUN_DIR
    /* Iterate modules in run/ (loaded by GRUB) */
    multiboot_module_t *mods = (multiboot_module_t*)mbi->mods_addr;
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        /* Treat module start address as function pointer */
        void (*mod_entry)(void) = (void(*)(void))(mods[i].mod_start);
        mod_entry();
    }
#endif

    /* Halt */
    for (;;) __asm__("hlt");
}

