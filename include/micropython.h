#ifndef MICROPYTHON_H
#define MICROPYTHON_H
#include <stddef.h>
#include <stdint.h>
void run_micropython(const char *code, size_t size);
void run_micropython_mpy(const uint8_t *buf, size_t size);
#endif
