#include "mpy_loader.h"
#include "micropython.h"
#include "mem.h"
#include <string.h>

void mpymod_load_all(void) {
    for (size_t i = 0; i < mpymod_table_count; ++i) {
        const mpymod_entry_t *m = &mpymod_table[i];
        size_t name_len = strlen(m->name);
        /* escape special characters for Python string */
        size_t esc_len = 0;
        for (size_t j = 0; j < m->source_len; ++j) {
            char c = m->source[j];
            if (c == '\\' || c == '"') esc_len++; /* extra backslash */
        }
        size_t total = name_len * 2 + m->source_len + esc_len + 128;
        char *buf = mem_alloc(total);
        if (!buf)
            continue;
        char *p = buf;
        memcpy(p, "import builtins\n", 16); p += 16;
        memcpy(p, "builtins._mpymod_data['", 23); p += 23;
        memcpy(p, m->name, name_len); p += name_len;
        memcpy(p, "'] = \"", 6); p += 6;
        for (size_t j = 0; j < m->source_len; ++j) {
            char c = m->source[j];
            if (c == '\\' || c == '"') *p++ = '\\';
            *p++ = c;
        }
        memcpy(p, "\"\nbuiltins.mpyrun('", 20); p += 20;
        memcpy(p, m->name, name_len); p += name_len;
        memcpy(p, "')\n", 3); p += 3;
        *p = '\0';
        mp_runtime_exec(buf, p - buf);
        mem_free(buf, total);
    }
}
