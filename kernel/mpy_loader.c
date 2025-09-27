#include "mpy_loader.h"
#include "micropython.h"
#include "mem.h"
#include "serial.h"
#include <string.h>

void mpymod_load_all(void) {
    for (size_t i = 0; i < mpymod_table_count; ++i) {
        const mpymod_entry_t *m = &mpymod_table[i];
        size_t name_len = strlen(m->name);
        /* escape special characters for Python string */
        size_t esc_len = 0;
        for (size_t j = 0; j < m->source_len; ++j) {
            char c = m->source[j];
            if (c == '\\' || c == '"' || c == '\n' || c == '\r')
                esc_len++; /* extra backslash */
        }
        size_t total = name_len * 2 + m->source_len + esc_len + 160;
        char *buf = mem_alloc(total);
        if (!buf)
            continue;
        char *p = buf;
        memcpy(p, "import env\n", 11); p += 11;
        memcpy(p, "env._mpymod_exec('", 18); p += 18;
        memcpy(p, m->name, name_len); p += name_len;
        memcpy(p, "', \"", 4); p += 4;
        for (size_t j = 0; j < m->source_len; ++j) {
            char c = m->source[j];
            if (c == '\n') {
                *p++ = '\\';
                *p++ = 'n';
            } else if (c == '\r') {
                *p++ = '\\';
                *p++ = 'r';
            } else {
                if (c == '\\' || c == '"') *p++ = '\\';
                *p++ = c;
            }
        }
        memcpy(p, "\")\n", 3); p += 3;
        *p = '\0';
        serial_write("mpy:\n");
        serial_write(buf);
        serial_write("\n");
        mp_runtime_exec(buf, p - buf);
        mem_free(buf, total);
    }
}
