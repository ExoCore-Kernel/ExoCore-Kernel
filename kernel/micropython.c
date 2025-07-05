#include "micropython.h"
#include "console.h"
#include "mem.h"
#include "port/micropython_embed.h"
#include <string.h>

static char mp_heap[64 * 1024];
static int mp_active = 0;
static int mp_stack_top;

void mp_runtime_init(void) {
    if (!mp_active) {
        mp_embed_init(mp_heap, sizeof(mp_heap), &mp_stack_top);
        mp_active = 1;
    }
}

void mp_runtime_deinit(void) {
    if (mp_active) {
        mp_embed_deinit();
        mp_active = 0;
    }
}

void mp_runtime_exec(const char *code, size_t size) {
    mp_runtime_init();
    char *buf = mem_alloc(size + 1);
    if (!buf)
        return;
    memcpy(buf, code, size);
    buf[size] = '\0';
    mp_embed_exec_str(buf);
}

void mp_runtime_exec_mpy(const uint8_t *buf, size_t size) {
    mp_runtime_init();
    mp_embed_exec_mpy(buf, size);
}

__attribute__((force_align_arg_pointer))
void run_micropython(const char *code, size_t size) {
    mp_runtime_init();
    mp_runtime_exec(code, size);
    mp_runtime_deinit();
}

__attribute__((force_align_arg_pointer))
void run_micropython_mpy(const uint8_t *buf, size_t size) {
    mp_runtime_init();
    mp_runtime_exec_mpy(buf, size);
    mp_runtime_deinit();
}
