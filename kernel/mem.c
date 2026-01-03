#include "mem.h"
#include "memutils.h"
#include "console.h"
#include "debuglog.h"
#include "panic.h"
#include "runstate.h"
#include "vga_draw.h"

#define MAX_APPS 16

typedef struct {
    int      id;
    uint8_t  priority;
    uint8_t *base;
    uint32_t used;
    struct {
        void   *addr;
        size_t  size;
    } blocks[16];
    int block_count;
} app_mem_t;

static app_mem_t apps[MAX_APPS];
static uint8_t *heap_start;
static uint8_t *heap_ptr;
static uint8_t *heap_end;
static int next_id = 1;
static uint8_t swap_space[64 * 1024];
static uint8_t *swap_ptr;

typedef struct mem_block {
    size_t total_size;   /* bytes including header */
    size_t user_size;    /* bytes available to caller */
    uint32_t guard;
    struct mem_block *next;
    int from_swap;
} mem_block_t;

typedef struct {
    const char *owner;
    uint32_t locks;
} vram_lock_t;

static mem_block_t *free_list;
static uint8_t *vram_base = (uint8_t *)0xB8000;
static size_t vram_size = VGA_DRAW_COLS * VGA_DRAW_ROWS * 2;
static vram_lock_t vram_locks[4];

#define MEM_GUARD_ACTIVE 0xC0DEFACEU
#define MEM_GUARD_FREED  0xDEADFA11U

static void log_mem_event(const char *msg) {
    console_puts(msg);
}

static int ptr_in_region(uint8_t *ptr, uint8_t *start, uint8_t *end) {
    return ptr >= start && ptr < end;
}

static void ensure_block_integrity(mem_block_t *blk, const char *context) {
    if (!blk)
        panic("mem: null block");
    if (blk->guard != MEM_GUARD_ACTIVE) {
        console_puts("mem guard failure in ");
        console_puts(context);
        console_putc('\n');
        panic("mem: use-after-free or corrupt block");
    }
}

static void mem_vram_init(void) {
    vram_base = (uint8_t *)0xB8000;
    vram_size = VGA_DRAW_COLS * VGA_DRAW_ROWS * 2;
    for (int i = 0; i < 4; ++i) {
        vram_locks[i].owner = NULL;
        vram_locks[i].locks = 0;
    }
}

static mem_block_t *split_block(mem_block_t *blk, size_t total_needed, mem_block_t **prev) {
    size_t available = blk->total_size;
    if (available >= total_needed + sizeof(mem_block_t) + 8) {
        mem_block_t *remainder = (mem_block_t *)((uint8_t *)blk + total_needed);
        remainder->total_size = available - total_needed;
        remainder->user_size = remainder->total_size - sizeof(mem_block_t);
        remainder->guard = MEM_GUARD_FREED;
        remainder->from_swap = blk->from_swap;
        remainder->next = blk->next;
        blk->total_size = total_needed;
        blk->user_size = total_needed - sizeof(mem_block_t);
        blk->next = NULL;
        *prev = remainder;
    } else {
        *prev = blk->next;
    }
    blk->guard = MEM_GUARD_ACTIVE;
    blk->next = NULL;
    return blk;
}

static mem_block_t *acquire_from_free_list(size_t total) {
    mem_block_t **prev = &free_list;
    mem_block_t *blk = free_list;
    while (blk) {
        if (blk->total_size >= total) {
            return split_block(blk, total, prev);
        }
        prev = &blk->next;
        blk = blk->next;
    }
    return NULL;
}

static mem_block_t *fresh_block(uint8_t **cursor, uint8_t *limit, size_t total) {
    if (*cursor + total > limit)
        return NULL;
    mem_block_t *blk = (mem_block_t *)*cursor;
    *cursor += total;
    blk->total_size = total;
    blk->user_size = total - sizeof(mem_block_t);
    blk->guard = MEM_GUARD_ACTIVE;
    blk->next = NULL;
    blk->from_swap = (limit == swap_space + sizeof(swap_space));
    return blk;
}

void mem_init(uintptr_t heap_start_addr, size_t heap_size) {
    console_puts("mem_init heap_start=0x");
    console_uhex((uint64_t)heap_start_addr);
    console_puts(" size=");
    console_udec(heap_size);
    console_putc('\n');
    heap_start = (uint8_t*)heap_start_addr;
    heap_ptr = heap_start;
    heap_end = heap_ptr + heap_size;
    swap_ptr = swap_space;
    free_list = NULL;
    mem_vram_init();
    for (int i = 0; i < MAX_APPS; i++) {
        apps[i].id = 0;
        apps[i].priority = 0;
        apps[i].base = NULL;
        apps[i].used = 0;
        apps[i].block_count = 0;
        for (int j = 0; j < 16; j++) {
            apps[i].blocks[j].addr = NULL;
            apps[i].blocks[j].size = 0;
        }
    }
}

void *mem_alloc(size_t size) {
    size = (size + 7) & ~7;
    size_t total = sizeof(mem_block_t) + size;
    total = (total + 7) & ~7;

    log_mem_event("mem_alloc size=");
    console_udec(size);
    console_puts(" heap_ptr=0x");
    console_uhex((uint64_t)(uintptr_t)heap_ptr);
    console_putc('\n');

    mem_block_t *blk = acquire_from_free_list(total);
    if (!blk) {
        blk = fresh_block(&heap_ptr, heap_end, total);
    }
    if (!blk) {
        blk = fresh_block(&swap_ptr, swap_space + sizeof(swap_space), total);
    }
    if (!blk) {
        panic("mem: out of memory");
    }

    console_puts(blk->from_swap ? " swap alloc addr=0x" : " alloc addr=0x");
    console_uhex((uint64_t)(uintptr_t)(blk + 1));
    console_putc('\n');
    return (void *)(blk + 1);
}

int mem_register_app(uint8_t priority) {
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i].id == 0) {
            apps[i].id = next_id++;
            apps[i].priority = priority;
            apps[i].base = NULL;
            apps[i].used = 0;
            apps[i].block_count = 0;
            for (int j = 0; j < 16; j++) {
                apps[i].blocks[j].addr = NULL;
                apps[i].blocks[j].size = 0;
            }
            console_puts("mem_register_app id=");
            console_udec(apps[i].id);
            console_putc('\n');
            return apps[i].id;
        }
    }
    return -1;
}

void *mem_alloc_app(int app_id, size_t size) {
    size = (size + 7) & ~7;
    console_puts("mem_alloc_app id=");
    console_udec(app_id);
    console_puts(" size=");
    console_udec(size);
    console_putc('\n');
    void *addr = mem_alloc(size);
    if (!addr)
        return NULL;
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i].id == app_id) {
            if (apps[i].base == NULL)
                apps[i].base = addr;
            apps[i].used += size;
            console_puts(" app alloc addr=0x");
            console_uhex((uint64_t)(uintptr_t)addr);
            console_putc('\n');
            return addr;
        }
    }
    mem_free(addr, size);
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
    size_t free_bytes = (size_t)(heap_end - heap_ptr);
    free_bytes += (size_t)(sizeof(swap_space) - (swap_ptr - swap_space));
    mem_block_t *blk = free_list;
    while (blk) {
        free_bytes += blk->total_size;
        blk = blk->next;
    }
    return free_bytes;
}

int mem_save_app(int app_id, const void *data, size_t size) {
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i].id == app_id) {
            if (apps[i].block_count >= 16)
                return -1;
            void *dst = mem_alloc_app(app_id, size);
            if (!dst)
                return -1;
            memcpy(dst, data, size);
            int idx = apps[i].block_count;
            apps[i].blocks[idx].addr = dst;
            apps[i].blocks[idx].size = size;
            apps[i].block_count++;
            console_puts("mem_save_app id=");
            console_udec(app_id);
            console_puts(" handle=");
            console_udec(idx);
            console_puts(" size=");
            console_udec(size);
            console_putc('\n');
            return idx;
        }
    }
    return -1;
}

void *mem_retrieve_app(int app_id, int handle, size_t *size) {
    for (int i = 0; i < MAX_APPS; i++) {
        if (apps[i].id == app_id) {
            if (handle < 0 || handle >= apps[i].block_count)
                return NULL;
            if (size)
                *size = apps[i].blocks[handle].size;
            console_puts("mem_retrieve_app id=");
            console_udec(app_id);
            console_puts(" handle=");
            console_udec(handle);
            console_putc('\n');
            return apps[i].blocks[handle].addr;
        }
    }
    return NULL;
}

void mem_free(void *addr, size_t size) {
    if (!addr)
        return;
    mem_block_t *blk = ((mem_block_t *)addr) - 1;
    if (!ptr_in_region((uint8_t *)blk, heap_start, heap_end) &&
        !ptr_in_region((uint8_t *)blk, swap_space, swap_space + sizeof(swap_space))) {
        panic("mem: free outside managed region");
    }
    ensure_block_integrity(blk, "mem_free");

    if (size && size > blk->user_size) {
        panic("mem: free size larger than allocation");
    }

    if (debug_mode) {
#if UINTPTR_MAX == 0xffffffff
        uint32_t *p = addr;
        size_t n = blk->user_size / sizeof(uint32_t);
        for (size_t i = 0; i < n; i++)
            p[i] = 0xDEADBEEF;
        size_t off = n * sizeof(uint32_t);
#else
        uint64_t *p = addr;
        size_t n = blk->user_size / sizeof(uint64_t);
        for (size_t i = 0; i < n; i++)
            p[i] = 0xDEADBEEFDEADBEEFULL;
        size_t off = n * sizeof(uint64_t);
#endif
        uint8_t *b = (uint8_t *)addr;
        for (size_t i = off; i < blk->user_size; i++)
            b[i] = 0xEF;
    }

    blk->guard = MEM_GUARD_FREED;
    blk->next = free_list;
    free_list = blk;
}

void *mem_vram_base(void) {
    return vram_base;
}

size_t mem_vram_size(void) {
    return vram_size;
}

static vram_lock_t *find_vram_slot(const char *owner) {
    for (int i = 0; i < 4; ++i) {
        if (vram_locks[i].owner == owner && owner != NULL)
            return &vram_locks[i];
        if (vram_locks[i].owner && owner && strcmp(vram_locks[i].owner, owner) == 0)
            return &vram_locks[i];
    }
    return NULL;
}

void *mem_vram_lock(const char *owner) {
    const char *tag = owner ? owner : "unknown";
    vram_lock_t *slot = find_vram_slot(tag);
    if (!slot) {
        for (int i = 0; i < 4; ++i) {
            if (vram_locks[i].owner == NULL || vram_locks[i].locks == 0) {
                vram_locks[i].owner = tag;
                vram_locks[i].locks = 1;
                return vram_base;
            }
        }
        panic("mem: no VRAM locks available");
    }
    slot->locks++;
    return vram_base;
}

void mem_vram_unlock(const char *owner) {
    vram_lock_t *slot = find_vram_slot(owner ? owner : "unknown");
    if (!slot)
        panic("mem: unlocking unowned VRAM");
    if (slot->locks == 0)
        panic("mem: VRAM lock underflow");
    slot->locks--;
    if (slot->locks == 0)
        slot->owner = NULL;
}

void *mem_vram_window(const char *owner, size_t offset, size_t length) {
    if (offset + length > vram_size)
        panic("mem: VRAM window exceeds bounds");
    mem_vram_lock(owner);
    return vram_base + offset;
}
