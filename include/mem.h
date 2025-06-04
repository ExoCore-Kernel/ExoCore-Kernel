#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>
void mem_init(uint32_t heap_start, uint32_t heap_size);
void *mem_alloc(uint32_t size);

/* Per-application ballooned memory management */
int  mem_register_app(uint8_t priority);
void *mem_alloc_app(int app_id, uint32_t size);
uint32_t mem_app_used(int app_id);

#endif /* MEM_H */
