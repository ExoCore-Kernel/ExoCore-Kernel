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
            if (c == '\\' || c == '"' || c == '\n') esc_len++; /* escape */
        }
        size_t total = name_len * 2 + m->source_len + esc_len + 128;
        char *buf = mem_alloc(total);
        if (!buf)
            continue;
        char *p = buf;
        memcpy(p, "import env\n", sizeof("import env\n") - 1); p += sizeof("import env\n") - 1;
        memcpy(p, "env._mpymod_data['", sizeof("env._mpymod_data['") - 1); p += sizeof("env._mpymod_data['") - 1;
        memcpy(p, m->name, name_len); p += name_len;
        memcpy(p, "'] = \"#mpyexo\\n\"", sizeof("'] = \"#mpyexo\\n\"") - 1); p += sizeof("'] = \"#mpyexo\\n\"") - 1;
        for (size_t j = 0; j < m->source_len; ++j) {
            char c = m->source[j];
            if (c == '\\' || c == '"' || c == '\n') {
                *p++ = '\\';
                if (c == '\n') {
                    *p++ = 'n';
                    continue;
                }
            }
            *p++ = c;
        }
        memcpy(p, "\"\nenv.mpyrun('", sizeof("\"\nenv.mpyrun('") - 1); p += sizeof("\"\nenv.mpyrun('") - 1;
        memcpy(p, m->name, name_len); p += name_len;
        memcpy(p, "')\n", sizeof("')\n") - 1); p += sizeof("')\n") - 1;
        *p = '\0';
        mp_runtime_exec(buf, p - buf);
        mem_free(buf, total);
    }
}
