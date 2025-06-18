#include "micropy.h"
#include "console.h"
#include <stddef.h>
#ifdef WITH_MICROPY
#include "py/runtime.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/stackctrl.h"

static char mp_heap[16 * 1024];

void run_micropy(const char *code, size_t size) {
    mp_stack_ctrl_init();
    mp_stack_set_limit(sizeof(mp_heap) - 1024);
    gc_init(mp_heap, mp_heap + sizeof(mp_heap));
    mp_init();

    mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, code, size, 0);
    qstr source = MP_QSTR__lt_stdin_gt_;
    mp_parse_tree_t parse = mp_parse(lex, MP_PARSE_FILE_INPUT);
    mp_obj_t mod = mp_compile(&parse, source, MP_EMIT_OPT_NONE, true);
    mp_call_function_0(mod);
    mp_deinit();
}
#else
void run_micropy(const char *code, size_t size) {
    console_puts("MicroPython not built\n");
}
#endif
