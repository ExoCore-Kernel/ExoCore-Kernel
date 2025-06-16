#include <stdint.h>
#include "console.h"

void _start() {
    console_puts("[user] Hello from userland 00\n");
    for (;;) __asm__("hlt");
}
