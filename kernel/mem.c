#include "mem.h"

#define MAX_APPS 16

typedef struct {
    int      id;
    uint8_t  priority;
    uint8_t *base;
    uint32_t used;
} app_mem_t;

static app_mem_t apps[MAX_APPS];
static uint8_t *heap_ptr;
static uint8_t *heap_end;
static int next_id = 1;
static uint8_t swap_space[64 * 1024];
static uint8_t *swap_ptr;

void mem_init(uintptr_t heap_start, size_t heap_size) {
    heap_ptr = (uint8_t*)heap_start;
    heap_end = heap_ptr + heap_size;
    swap_ptr = swap_space;
    for (int i = 0; i < MAX_APPS; i++) {
        apps[i].id = 0;
        apps[i].priority = 0;
        apps[i].base = NULL;
        apps[i].used = 0;
    }
}

void *mem_alloc(size_t size) {
    size = (size + 7) & ~7;
    if (heap_ptr + size > heap_end) {
        if (swap_ptr + size > swap_space + sizeof(swap_space))
            return NULL;
        void *addr = swap_ptr;
        swap_ptr += size;
        return addr;
    }
    void *addr = heap_ptr;
    heap_ptr += size;
    return addr;
}

int mem_register_app(uint8_t priority) {
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i].id == 0) {
            apps[i].id = next_id++;
            apps[i].priority = priority;
            apps[i].base = NULL;
            apps[i].used = 0;
            return apps[i].id;
        }
    }
    return -1;
}

void *mem_alloc_app(int app_id, size_t size) {
    size = (size + 7) & ~7;
    if (heap_ptr + size > heap_end)
        return NULL;
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i].id == app_id) {
            if (apps[i].base == NULL)
                apps[i].base = heap_ptr;
            void *addr = heap_ptr;
            heap_ptr += size;
            apps[i].used += size;
            return addr;
        }
    }
    return NULL;
}

size_t mem_app_used(int app_id) {
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i].id == app_id)
            return apps[i].used;
    }
    return 0;
}

size_t mem_heap_free(void) {
    return (heap_end - heap_ptr) + (sizeof(swap_space) - (swap_ptr - swap_space));
}
