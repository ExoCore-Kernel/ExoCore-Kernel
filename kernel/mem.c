#include "mem.h"

static uint32_t heap_ptr;

void mem_init(uint32_t heap_start, uint32_t heap_size) {
    (void)heap_size;
    heap_ptr = heap_start;
}

void *mem_alloc(uint32_t size) {
    size = (size + 7) & ~7;
    void *addr = (void*)heap_ptr;
    heap_ptr += size;
    return addr;
}
