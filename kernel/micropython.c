#include "micropython.h"
#include "console.h"
#include "mem.h"
#include "port/micropython_embed.h"
#include <string.h>

static char mp_heap[64 * 1024];

__attribute__((force_align_arg_pointer))
void run_micropython(const char *code, size_t size) {
    int stack_top;
    mp_embed_init(mp_heap, sizeof(mp_heap), &stack_top);
    char *buf = mem_alloc(size + 1);
    if (!buf) {
        mp_embed_deinit();
        return;
    }
    memcpy(buf, code, size);
    buf[size] = '\0';
    mp_embed_exec_str(buf);
    mp_embed_deinit();
}
__attribute__((force_align_arg_pointer))

void run_micropython_mpy(const uint8_t *buf, size_t size) {
    int stack_top;
    mp_embed_init(mp_heap, sizeof(mp_heap), &stack_top);
    mp_embed_exec_mpy(buf, size);
    mp_embed_deinit();
}
