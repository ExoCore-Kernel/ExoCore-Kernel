// run/test_print.c
#include "console.h"   /* for console_puts, console_putc, console_udec */

void _start(void) {
    console_puts(">>> Module test begin\n");

    console_puts("Counting: ");
    for (int i = 1; i <= 5; i++) {
        console_udec(i);
        console_putc(' ');
    }
    console_putc('\n');

    console_puts(">>> Module test end\n");

    /* hang so we don’t fall off into random memory */
    for (;;) __asm__("hlt");
}
