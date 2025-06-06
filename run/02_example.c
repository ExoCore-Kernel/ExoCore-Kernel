// run/example.c
// Example module calling example_compute from linkdep without using a header

#include <stdint.h>
#include "console.h"
#include "serial.h"

// Manual declaration of the library function
extern uint32_t example_compute(uint32_t x, uint32_t y);

void _start() {
    console_puts("=== Minimal Example Module ===\n");
    serial_write("=== Minimal Example Module ===\n");

    // Call into our manual example_lib function
    uint32_t sum = example_compute(3, 7);
    console_puts("Returned from example_compute: ");
    serial_write("Returned from example_compute: ");
    console_udec(sum);
    {
        char buf[12];
        int i=10; buf[11]='\0'; if(sum==0) buf[i--]='0'; uint32_t t=sum; while(t){buf[i--]='0'+(t%10); t/=10;} serial_write(&buf[i+1]);
    }
    console_puts("\n");
    serial_write("\n");

    console_puts("Example done, halting.\n");
    serial_write("Example done, halting.\n");
    for (;;) { __asm__("hlt"); }
}
