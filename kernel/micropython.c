#include "micropython.h"
#include "config.h"
#include "console.h"
#include "mem.h"
#include "mpy_loader.h"
#include "port/micropython_embed.h"
#include "modexec.h"
#include <string.h>
#include "py/stackctrl.h"
#include "py/objmodule.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/qstr.h"

#ifndef STATIC
#define STATIC static
#endif

static char mp_heap[EXOCORE_MICROPY_HEAP_SIZE];
static int mp_active = 0;

STATIC mp_obj_t mp_c_execo(mp_obj_t path_obj) {
    const char *path = mp_obj_str_get_str(path_obj);
    modexec_run(path);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_c_execo_obj, mp_c_execo);

STATIC mp_obj_t mp_c_getmod(mp_obj_t name_obj) {
    const char *name = mp_obj_str_get_str(name_obj);
    for (size_t i = 0; i < mpymod_table_count; ++i) {
        const mpymod_entry_t *entry = &mpymod_table[i];
        if (strcmp(entry->name, name) == 0) {
            return mp_obj_new_str(entry->source, entry->source_len);
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_c_getmod_obj, mp_c_getmod);


void mp_runtime_init(void) {
    if (!mp_active) {
        int stack_dummy;
        mp_stack_ctrl_init();
        mp_embed_init(mp_heap, sizeof(mp_heap), &stack_dummy);
        mp_stack_set_limit(16 * 1024);
        /* create real 'env' module for MicroPython */
        qstr env_qstr = qstr_from_str("env");
        mp_obj_t env_mod = mp_obj_new_module(env_qstr);
        mp_obj_dict_t *env_globals = mp_obj_module_get_globals(env_mod);
        mp_obj_t env_dict = mp_obj_new_dict(0);
        mp_obj_dict_store(MP_OBJ_FROM_PTR(env_globals), MP_OBJ_NEW_QSTR(env_qstr), env_dict);

        /* create built-in 'c' module with execo() */
        qstr c_qstr = qstr_from_str("c");
        mp_obj_t c_mod = mp_obj_new_module(c_qstr);
        mp_obj_dict_t *c_globals = mp_obj_module_get_globals(c_mod);
        mp_obj_dict_store(MP_OBJ_FROM_PTR(c_globals), MP_OBJ_NEW_QSTR(qstr_from_str("execo")), MP_OBJ_FROM_PTR(&mp_c_execo_obj));
        mp_obj_dict_store(MP_OBJ_FROM_PTR(c_globals), MP_OBJ_NEW_QSTR(qstr_from_str("getmod")), MP_OBJ_FROM_PTR(&mp_c_getmod_obj));

        /* expose helper functions via Python stub */
        mp_embed_exec_str(
            "import sys, env, c, builtins\n"
            "if not hasattr(env, 'env'):\n"
            "    env.env = {}\n"
            "_mpymod_cache = {}\n"
            "def _mpymod_exec(name, src, args=None):\n"
            "    ns = {'env': env.env, 'mpyrun': mpyrun, 'c': c, '__name__': name}\n"
            "    if args:\n"
            "        ns['__args__'] = args\n"
            "    exec(src, ns)\n"
            "    mod = type('obj', (), ns)\n"
            "    sys.modules[name] = mod\n"
            "    return mod\n"
            "def _mpymod_load(name, args=None):\n"
            "    src = _mpymod_cache.get(name)\n"
            "    if src is None:\n"
            "        src = c.getmod(name)\n"
            "        if src is None:\n"
            "            return None\n"
            "        _mpymod_cache[name] = src\n"
            "    return _mpymod_exec(name, src, args)\n"
            "def mpyrun(name, *args):\n"
            "    mod = _mpymod_load(name, args)\n"
            "    if mod is None:\n"
            "        print('module not found', name)\n"
            "    return mod\n"
            "_default_import = builtins.__import__\n"
            "def __exo_import__(name, globals=None, locals=None, fromlist=(), level=0):\n"
            "    if level:\n"
            "        return _default_import(name, globals, locals, fromlist, level)\n"
            "    try:\n"
            "        return _default_import(name, globals, locals, fromlist, level)\n"
            "    except ImportError:\n"
            "        mod = _mpymod_load(name)\n"
            "        if mod is None:\n"
            "            raise\n"
            "        if fromlist:\n"
            "            return mod\n"
            "        return mod\n"
            "builtins.__import__ = __exo_import__\n"
            "env.mpyrun = mpyrun\n"
            "env.c = c\n"
            "env._mpymod_cache = _mpymod_cache\n"
            "env._mpymod_data = _mpymod_cache\n"
            "env._mpymod_exec = _mpymod_exec\n"
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
