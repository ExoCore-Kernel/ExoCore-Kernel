// linkdep/example_lib.c
// Minimal example library for ExoCore Kernel

#include "console.h"
#include "serial.h"
#include <stdint.h>

// Example function: print a greeting and return a value
uint32_t example_compute(uint32_t x, uint32_t y) {
    console_puts("[lib] Computing sum...\n");
    serial_write("[lib] Computing sum...\n");
    uint32_t result = x + y;
    console_puts("[lib] Result: ");
    serial_write("[lib] Result: ");
    console_udec(result);
    {
        char buf[12];
        uint32_t r = result;
        int i = 10; buf[11]='\0'; if(r==0) buf[i--]='0'; while(r){buf[i--]='0'+(r%10); r/=10;}
        serial_write(&buf[i+1]);
    }
    console_puts("\n");
    return result;
}
