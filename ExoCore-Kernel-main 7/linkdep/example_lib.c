// linkdep/example_lib.c
// Minimal example library for ExoCore Kernel

#include "console.h"
#include <stdint.h>

// Example function: print a greeting and return a value
uint32_t example_compute(uint32_t x, uint32_t y) {
    console_puts("[lib] Computing sum...\n");
    uint32_t result = x + y;
    console_puts("[lib] Result: ");
    console_udec(result);
    console_puts("\n");
    return result;
}
