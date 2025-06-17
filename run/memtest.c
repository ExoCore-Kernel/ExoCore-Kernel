#include <stdint.h>
#include "console.h"
#include "mem.h"

void _start() {
    int id = mem_register_app(10); // high priority
    if (id < 0) {
        console_puts("mem_register_app failed\n");
        return;
    }
    void *a = mem_alloc_app(id, 4096);
    void *b = mem_alloc_app(id, 8192);
    (void)a; (void)b;
    console_puts("memtest used: ");
    console_udec(mem_app_used(id));
    console_puts(" bytes\n");
    /* Return control to allow remaining modules to execute */
    return;
}
