// run/test_vga.c
#include "console.h"

void _start(void) {
    console_puts(">>> Module VGA Test Begin\n");
    console_puts("Counting: ");
    for (int i = 1; i <= 5; i++) {
        console_udec(i);
        console_putc(' ');
    }
    console_putc('\n');
    console_puts(">>> Module VGA Test End\n");
    for (;;) __asm__("hlt");
}
