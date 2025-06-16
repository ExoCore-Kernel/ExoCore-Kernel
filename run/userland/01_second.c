#include <stdint.h>
#include "console.h"

void _start() {
    console_puts("[user] Second userland app\n");
    for (;;) __asm__("hlt");
}
