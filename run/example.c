// run/example.c
// Example module calling example_compute from linkdep without using a header

#include <stdint.h>
#include "console.h"

// Manual declaration of the library function
extern uint32_t example_compute(uint32_t x, uint32_t y);

void _start() {
    console_puts("=== Minimal Example Module ===\n");

    // Call into our manual example_lib function
    uint32_t sum = example_compute(3, 7);
    console_puts("Returned from example_compute: ");
    console_udec(sum);
    console_puts("\n");

    console_puts("Example done.\n");
    /* Return control so additional modules may run */
    return;
}
