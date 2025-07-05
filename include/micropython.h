#ifndef MICROPYTHON_H
#define MICROPYTHON_H
#include <stddef.h>
#include <stdint.h>

void mp_runtime_init(void);
void mp_runtime_deinit(void);
void mp_runtime_exec(const char *code, size_t size);
void mp_runtime_exec_mpy(const uint8_t *buf, size_t size);

// legacy single-shot helpers
void run_micropython(const char *code, size_t size)
    __attribute__((force_align_arg_pointer));
void run_micropython_mpy(const uint8_t *buf, size_t size)
    __attribute__((force_align_arg_pointer));

#endif
