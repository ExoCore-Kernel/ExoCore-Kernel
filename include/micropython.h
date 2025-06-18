#ifndef MICROPYTHON_H
#define MICROPYTHON_H
#include <stddef.h>
#include <stdint.h>
// Ensure these functions are entered with a 16-byte aligned stack
void run_micropython(const char *code, size_t size)
    __attribute__((force_align_arg_pointer));
void run_micropython_mpy(const uint8_t *buf, size_t size)
    __attribute__((force_align_arg_pointer));
#endif
