#include <stdint.h>
#include "console.h"
#include "serial.h"
#include "mem.h"

void _start() {
    int id = mem_register_app(10); // high priority
    if (id < 0) {
        console_puts("mem_register_app failed\n");
        serial_write("mem_register_app failed\n");
        for (;;) {}
    }
    void *a = mem_alloc_app(id, 4096);
    void *b = mem_alloc_app(id, 8192);
    (void)a; (void)b;
    console_puts("memtest used: ");
    serial_write("memtest used: ");
    console_udec(mem_app_used(id));
    {
        char buf[12];
        uint32_t u=mem_app_used(id); int i=10; buf[11]='\0'; if(u==0) buf[i--]='0'; while(u){buf[i--]='0'+(u%10); u/=10;} serial_write(&buf[i+1]);
    }
    console_puts(" bytes\n");
    serial_write(" bytes\n");
    for (;;) { __asm__("hlt"); }
}
