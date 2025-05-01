#include "mem.h"

static uint32_t heap_ptr = 0;

void mem_init(uint32_t heap_start, uint32_t heap_size) {
    (void)heap_size;      // we’re not checking overflow right now
    heap_ptr = heap_start;
}

void *mem_alloc(uint32_t size) {
    /* align size to 8 bytes */
    size = (size + 7) & ~7;
    void *addr = (void *)heap_ptr;
    heap_ptr += size;
    return addr;
}
