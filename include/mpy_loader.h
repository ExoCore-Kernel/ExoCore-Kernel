#ifndef MPY_LOADER_H
#define MPY_LOADER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;        // module import name
    const char *source;      // python source code
    size_t source_len;       // length of source
} mpymod_entry_t;

extern const mpymod_entry_t mpymod_table[];
extern const size_t mpymod_table_count;

void mpymod_load_all(void);

#ifdef __cplusplus
}
#endif

#endif /* MPY_LOADER_H */
