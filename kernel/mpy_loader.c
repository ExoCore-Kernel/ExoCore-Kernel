#include "mpy_loader.h"
#include "micropython.h"
#include "mem.h"
#include <string.h>

void mpymod_load_all(void) {
    for (size_t i = 0; i < mpymod_table_count; ++i) {
        const mpymod_entry_t *m = &mpymod_table[i];
        const char prefix1[] = "import sys, types\nmod = types.ModuleType('";
        const char prefix2[] = "')\nexec(\"\"\"\n";
        const char prefix3[] = "\"\", mod.__dict__)\nsys.modules['";
        const char prefix4[] = "'] = mod\n";
        size_t name_len = strlen(m->name);
        size_t total = sizeof(prefix1) - 1 + name_len +
                       sizeof(prefix2) - 1 + m->source_len + 1 +
                       sizeof(prefix3) - 1 + name_len + sizeof(prefix4) - 1;
        char *buf = mem_alloc(total + 1);
        if (!buf)
            continue;
        char *p = buf;
        memcpy(p, prefix1, sizeof(prefix1) - 1); p += sizeof(prefix1) - 1;
        memcpy(p, m->name, name_len); p += name_len;
        memcpy(p, prefix2, sizeof(prefix2) - 1); p += sizeof(prefix2) - 1;
        memcpy(p, m->source, m->source_len); p += m->source_len;
        *p++ = '\n';
        memcpy(p, prefix3, sizeof(prefix3) - 1); p += sizeof(prefix3) - 1;
        memcpy(p, m->name, name_len); p += name_len;
        memcpy(p, prefix4, sizeof(prefix4) - 1); p += sizeof(prefix4) - 1;
        *p = '\0';
        mp_runtime_exec(buf, p - buf);
    }
}
