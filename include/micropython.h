#ifndef MICROPYTHON_H
#define MICROPYTHON_H
#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define EXO_FORCE_ALIGN_ARG_POINTER __attribute__((force_align_arg_pointer))
#else
#define EXO_FORCE_ALIGN_ARG_POINTER
#endif

void mp_runtime_init(void);
void mp_runtime_deinit(void);
void mp_runtime_exec(const char *code, size_t size, const char *filename);
void mp_runtime_exec_mpy(const uint8_t *buf, size_t size);

// legacy single-shot helpers
void run_micropython(const char *code, size_t size, const char *filename)
    EXO_FORCE_ALIGN_ARG_POINTER;
void run_micropython_mpy(const uint8_t *buf, size_t size)
    EXO_FORCE_ALIGN_ARG_POINTER;

#endif
