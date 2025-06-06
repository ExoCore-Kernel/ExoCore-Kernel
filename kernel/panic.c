#include <stdint.h>
#include "console.h"
#include "serial.h"
#include "panic.h"

void panic(const char *msg) {
    serial_init();
    console_puts("Panic: ");
    serial_write("Panic: ");
    console_puts(msg);
    serial_write(msg);
    console_putc('\n');
    serial_write("\n");
    for (;;) __asm__("hlt");
}
