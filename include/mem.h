#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>
void mem_init(uintptr_t heap_start, size_t heap_size);
void *mem_alloc(size_t size);

/* Per-application ballooned memory management */
int  mem_register_app(uint8_t priority);
void *mem_alloc_app(int app_id, size_t size);
size_t mem_app_used(int app_id);

#endif /* MEM_H */
