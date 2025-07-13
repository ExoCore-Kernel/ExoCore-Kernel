#include "modexec.h"
#include "mem.h"
#include "console.h"
#include "config.h"
#include "multiboot.h"
#include <string.h>

static multiboot_info_t *g_mbi = NULL;

void modexec_set_mbi(multiboot_info_t *mbi) {
    g_mbi = mbi;
}

int modexec_run(const char *name) {
    if (!g_mbi) return -1;
    multiboot_module_t *mods = (multiboot_module_t*)(uintptr_t)g_mbi->mods_addr;
    for (uint32_t i = 0; i < g_mbi->mods_count; ++i) {
        const char *mstr = (const char*)(uintptr_t)mods[i].string;
        if (mstr && strcmp(mstr, name) == 0) {
            uint8_t *src  = (uint8_t*)(uintptr_t)mods[i].mod_start;
            uint32_t size = mods[i].mod_end - mods[i].mod_start;
            uint8_t *base = (uint8_t*)MODULE_BASE_ADDR;
            memcpy(base, src, size);
            console_puts("exec.o ");
            console_puts(name);
            console_putc('\n');
            void (*entry)(void) = (void(*)(void))base;
            entry();
            return 0;
        }
    }
    console_puts("exec.o failed: not found\n");
    return -1;
}
