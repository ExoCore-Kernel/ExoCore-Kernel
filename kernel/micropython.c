#include "micropython.h"
// Stubs for removed MicroPython integration
__attribute__((force_align_arg_pointer))
void run_micropython(const char *code, size_t size) {
    (void)code;
    (void)size;
}

__attribute__((force_align_arg_pointer))
void run_micropython_mpy(const uint8_t *buf, size_t size) {
    (void)buf;
    (void)size;
}
