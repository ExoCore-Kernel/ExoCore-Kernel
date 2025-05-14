#ifndef MEM_H
#define MEM_H

#include <stdint.h>
void mem_init(uint32_t heap_start, uint32_t heap_size);
void *mem_alloc(uint32_t size);

#endif /* MEM_H */
