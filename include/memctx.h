#ifndef MEMCTX_H
#define MEMCTX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t quota;
    size_t used;
    size_t peak;
    uint32_t allocations;
} memctx_stats_t;

void memctx_init(void);
int memctx_create(int owner_pid, size_t quota);
void *memctx_alloc(int ctx_id, size_t size);
int memctx_free(int ctx_id, void *ptr);
int memctx_stats(int ctx_id, memctx_stats_t *stats);
int memctx_destroy(int ctx_id);

#ifdef __cplusplus
}
#endif

#endif /* MEMCTX_H */
