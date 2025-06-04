#include <stdio.h>
#include "../include/mem.h"

int main() {
    mem_init(0x100000, 0x10000); // simple heap
    int id = mem_register_app(5);
    if (id < 0) return 1;
    void *a = mem_alloc_app(id, 1024);
    void *b = mem_alloc_app(id, 2048);
    if (!a || !b) return 1;
    if (mem_app_used(id) != 3072) return 1;
    printf("memory manager ok\n");
    return 0;
}
