#include "mem.h"

#define MAX_APPS 16

typedef struct {
    int      id;
    uint8_t  priority;
    uint32_t base;
    uint32_t used;
} app_mem_t;

static app_mem_t apps[MAX_APPS];
static uint32_t heap_ptr;
static uint32_t heap_end;
static int next_id = 1;

void mem_init(uint32_t heap_start, uint32_t heap_size) {
    heap_ptr = heap_start;
    heap_end = heap_start + heap_size;
    for (int i = 0; i < MAX_APPS; i++) {
        apps[i].id = 0;
        apps[i].priority = 0;
        apps[i].base = 0;
        apps[i].used = 0;
    }
}

void *mem_alloc(uint32_t size) {
    size = (size + 7) & ~7;
    if (heap_ptr + size > heap_end)
        return NULL;
    void *addr = (void*)heap_ptr;
    heap_ptr += size;
    return addr;
}

int mem_register_app(uint8_t priority) {
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i].id == 0) {
            apps[i].id = next_id++;
            apps[i].priority = priority;
            apps[i].base = 0;
            apps[i].used = 0;
            return apps[i].id;
        }
    }
    return -1;
}

void *mem_alloc_app(int app_id, uint32_t size) {
    size = (size + 7) & ~7;
    if (heap_ptr + size > heap_end)
        return NULL;
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i].id == app_id) {
            if (apps[i].base == 0)
                apps[i].base = heap_ptr;
            void *addr = (void*)heap_ptr;
            heap_ptr += size;
            apps[i].used += size;
            return addr;
        }
    }
    return NULL;
}

uint32_t mem_app_used(int app_id) {
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i].id == app_id)
            return apps[i].used;
    }
    return 0;
}
