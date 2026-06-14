#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t heap_used;
    size_t heap_free;
    size_t heap_committed;
    size_t heap_max;
    size_t grow_count;
    size_t alloc_fail_count;
} mem_info_t;

void mem_init(uintptr_t heap_start, size_t heap_size);
void *mem_alloc(size_t size);
void *mem_alloc_or_panic(size_t size);
void mem_free(void *addr, size_t size);

/* VRAM management */
void *mem_vram_base(void);
size_t mem_vram_size(void);
void *mem_vram_lock(const char *owner);
void mem_vram_unlock(const char *owner);
void *mem_vram_window(const char *owner, size_t offset, size_t length);

/* Per-application ballooned memory management */
int  mem_register_app(uint8_t priority);
void *mem_alloc_app(int app_id, size_t size);
size_t mem_app_used(int app_id);
size_t mem_heap_free(void);
void mem_get_info(mem_info_t *info);

/* Store arbitrary data for an application */
int   mem_save_app(int app_id, const void *data, size_t size);
void *mem_retrieve_app(int app_id, int handle, size_t *size);

#ifdef __cplusplus
}
#endif

#endif /* MEM_H */
