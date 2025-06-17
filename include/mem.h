#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void mem_init(uintptr_t heap_start, size_t heap_size);
void *mem_alloc(size_t size);

/* Per-application ballooned memory management */
int  mem_register_app(uint8_t priority);
void *mem_alloc_app(int app_id, size_t size);
size_t mem_app_used(int app_id);
size_t mem_heap_free(void);

#ifdef __cplusplus
}
#endif

#endif /* MEM_H */
