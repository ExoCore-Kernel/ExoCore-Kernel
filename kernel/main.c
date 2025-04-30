#include <stdint.h>
#include "multiboot.h"
#include "config.h"

static volatile char *video = (char*)0xB8000;

/* Entry called by boot.S: magic in EAX, mbi ptr in EBX */
void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
    /* Verify multiboot */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        video[0] = '!';
        for (;;) __asm__("hlt");
    }

    /* Show kernel started */
    video[0] = 'E';

#if FEATURE_RUN_DIR
    /* Debug: show how many modules GRUB passed us */
    uint32_t count = mbi->mods_count;
    video[1] = (count < 10) ? ('0' + count) : '?';

    /* Iterate modules, mark an 'M' for each */
    multiboot_module_t *mods = (multiboot_module_t*)mbi->mods_addr;
    for (uint32_t i = 0; i < count && i < 77; i++) {
        video[2 + i] = 'M';
        /* You could also check mods[i].mod_start, mods[i].mod_end here */
    }
#endif

    /* Halt */
    for (;;) __asm__("hlt");
}
