#include "micropython.h"
#include "console.h"
#include "mem.h"
#include "port/micropython_embed.h"
#include <string.h>
#include "py/stackctrl.h"
#include "py/objmodule.h"
#include "py/runtime.h"

static char mp_heap[64 * 1024];
static int mp_active = 0;


void mp_runtime_init(void) {
    if (!mp_active) {
        int stack_dummy;
        mp_stack_ctrl_init();
        mp_embed_init(mp_heap, sizeof(mp_heap), &stack_dummy);
        mp_stack_set_limit(16 * 1024);
        /* create real 'env' module for MicroPython */
        mp_obj_dict_t *env_globals = mp_obj_new_dict(0);
        mp_obj_module_t *env_mod = m_new_obj(mp_obj_module_t);
        env_mod->base.type = &mp_type_module;
        env_mod->globals = env_globals;
        mp_obj_dict_store(MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_loaded_modules_dict)),
            mp_obj_new_str("env", 3), MP_OBJ_FROM_PTR(env_mod));

        /* shared environment dictionary */
        mp_obj_t env_dict = mp_obj_new_dict(0);
        mp_obj_dict_store(env_globals, mp_obj_new_str("env", 3), env_dict);

        /* expose helper functions via Python stub */
        mp_embed_exec_str(
            "import sys, env\n"
            "_mpymod_data = {}\n"
            "class _C:\n"
            "    def exec(self, *args):\n"
            "        print('c.exec', args)\n"
            "c = _C()\n"
            "def mpyrun(name, *args):\n"
            "    src = _mpymod_data.get(name)\n"
            "    if src is None:\n"
            "        print('module not found', name)\n"
            "        return\n"
            "    ns = {'env': env.env, 'mpyrun': mpyrun, 'c': c, '__name__': name}\n"
            "    exec(src, ns)\n"
            "    sys.modules[name] = type('obj', (), ns)\n"
            "env.mpyrun = mpyrun\n"
            "env.c = c\n"
            "env._mpymod_data = _mpymod_data\n"
        );
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
    mem_free(buf, size + 1);
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
