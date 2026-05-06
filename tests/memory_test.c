#include <stdio.h>
#include "../include/mem.h"

int main() {
    static unsigned char heap[0x10000];
    mem_init((uintptr_t)heap, sizeof(heap));
    int id = mem_register_app(5);
    if (id < 0) return 1;
    void *a = mem_alloc_app(id, 1024);
    void *b = mem_alloc_app(id, 2048);
    if (!a || !b) return 1;
    if (mem_app_used(id) != 3072) return 1;
    int idx = mem_save_app(id, "hi", 3);
    if (idx < 0) return 1;
    size_t sz; char *p = mem_retrieve_app(id, idx, &sz);
    if (!p || sz != 3) return 1;
    if (p[0] != 'h' || p[1] != 'i') return 1;

    /* Verify fragmentation recovery for large data blocks. */
    void *large_a = mem_alloc(20000);
    void *large_b = mem_alloc(20000);
    if (!large_a || !large_b) return 1;
    mem_free(large_a, 20000);
    mem_free(large_b, 20000);
    void *large_merged = mem_alloc(38000);
    if (!large_merged) return 1;

    printf("memory manager ok\n");
    return 0;
}
