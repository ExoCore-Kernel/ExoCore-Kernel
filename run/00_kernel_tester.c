#include <stdint.h>
#include "console.h"
#include "serial.h"

/* Manual declaration from linkdep */
extern uint32_t example_compute(uint32_t x, uint32_t y);

void _start() {
    console_puts("[test] Kernel tester running\n");
    serial_write("[test] Kernel tester running\n");

    /* Verify library linkage */
    uint32_t val = example_compute(2, 3);
    console_puts("[test] example_compute result: ");
    console_udec(val);
    console_putc('\n');
    serial_write("[test] example_compute result: ");
    serial_udec(val);
    serial_putc('\n');

    /* Check long mode */
    if (sizeof(void*) == 8) {
        console_set_attr(VGA_LIGHT_GREEN, VGA_BLACK);
        console_puts("[test] 64-bit mode OK\n");
        console_set_attr(VGA_WHITE, VGA_BLACK);
        serial_write("[test] 64-bit mode OK\n");
    } else {
        console_set_attr(VGA_LIGHT_RED, VGA_BLACK);
        console_puts("[test] 64-bit mode FAILED\n");
        console_set_attr(VGA_WHITE, VGA_BLACK);
        serial_write("[test] 64-bit mode FAILED\n");
    }

    /* Return to the kernel so the next module can run */
    return;
}
