#ifndef MEM_H
#define MEM_H

#include <stdint.h>

/**
 * Initialize the heap.
 *
 * @param heap_start  Physical start address of the heap region.
 * @param heap_size   Size in bytes of the heap region.
 */
void mem_init(uint32_t heap_start, uint32_t heap_size);

/**
 * Allocate a chunk of memory from the heap.
 *
 * @param size  Number of bytes to allocate.
 * @return      Pointer to the start of the allocated block.
 */
void *mem_alloc(uint32_t size);

#endif /* MEM_H */
