#include "micropython.h"
#include "config.h"
#include "console.h"
#include "mpy_loader.h"
#include "port/micropython_embed.h"
#include "modexec.h"
#include "panic.h"
#include "serial.h"
#include <string.h>
#include "py/compile.h"
#include "py/lexer.h"
#include "py/parse.h"
#include "py/stackctrl.h"
#include "py/objmodule.h"
#include "py/objstr.h"
#include "py/obj.h"
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
            "def _ensure_env_dict():\n"
            "    try:\n"
            "        data = env.env\n"
            "    except AttributeError:\n"
            "        data = {}\n"
            "        try:\n"
            "            env.env = data\n"
            "        except AttributeError:\n"
            "            pass\n"
            "    if not isinstance(data, dict):\n"
            "        try:\n"
            "            data = dict(data)\n"
            "        except Exception:\n"
            "            data = {}\n"
            "        try:\n"
            "            env.env = data\n"
            "        except AttributeError:\n"
            "            pass\n"
            "    return data\n"
            "_ensure_env_dict()\n"
            "_sys_modules = getattr(sys, 'modules', None)\n"
            "if _sys_modules is None:\n"
            "    _sys_modules = {}\n"
            "    try:\n"
            "        sys.modules = _sys_modules\n"
            "    except AttributeError:\n"
            "        pass\n"
            "    else:\n"
            "        _sys_modules = sys.modules\n"
            "env._sys_modules = _sys_modules\n"
            "_mpymod_cache = {}\n"
            "def _mpymod_exec(name, src, args=None):\n"
            "    env_dict = _ensure_env_dict()\n"
            "    ns = {'env': env_dict, 'mpyrun': mpyrun, 'c': c, '__name__': name}\n"
            "    if args:\n"
            "        ns['__args__'] = args\n"
            "    exec(src, ns)\n"
            "    mod = type('obj', (), ns)\n"
            "    _sys_modules[name] = mod\n"
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
            "_default_import = getattr(builtins, '__import__', None)\n"
            "if _default_import is not None:\n"
            "    try:\n"
            "        def __exo_import__(name, globals=None, locals=None, fromlist=(), level=0):\n"
            "            if level:\n"
            "                return _default_import(name, globals, locals, fromlist, level)\n"
            "            try:\n"
            "                return _default_import(name, globals, locals, fromlist, level)\n"
            "            except ImportError:\n"
            "                mod = _mpymod_load(name)\n"
            "                if mod is None:\n"
            "                    raise\n"
            "                if fromlist:\n"
            "                    return mod\n"
            "                return mod\n"
            "        builtins.__import__ = __exo_import__\n"
            "    except AttributeError:\n"
            "        pass\n"
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

static void mp_exo_print_strn(void *env, const char *str, size_t len) {
    (void)env;
    for (size_t i = 0; i < len; ++i) {
        char c = str[i];
        console_putc(c);
        serial_putc(c);
    }
}

static void mp_report_exception(mp_obj_t exc, const char *fallback_name) {
    static const mp_print_t exo_print = { .data = NULL, .print_strn = mp_exo_print_strn };
    mp_obj_print_exception(&exo_print, exc);

    size_t n = 0;
    size_t *values = NULL;
    mp_obj_exception_get_traceback(exc, &n, &values);

    const char *filename = fallback_name ? fallback_name : "<stdin>";
    size_t line = 0;

    if (n >= 3) {
        filename = qstr_str(values[n - 3]);
        line = values[n - 2];
    }

    console_puts("MicroPython error location: ");
    serial_write("MicroPython error location: ");
    console_puts(filename);
    serial_write(filename);
    console_puts(":");
    serial_write(":");
    console_udec(line);
    serial_udec(line);
    console_putc('\n');
    serial_write("\n");

    panic("MicroPython exception");
}

void mp_runtime_exec(const char *code, size_t size, const char *filename) {
    mp_runtime_init();
    qstr source_name = filename ? qstr_from_str(filename) : MP_QSTR__lt_stdin_gt_;
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(source_name, code, size, 0);
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        mp_report_exception((mp_obj_t)nlr.ret_val, filename);
    }
}

void mp_runtime_exec_mpy(const uint8_t *buf, size_t size) {
    mp_runtime_init();
    mp_embed_exec_mpy(buf, size);
}

__attribute__((force_align_arg_pointer))
void run_micropython(const char *code, size_t size, const char *filename) {
    mp_runtime_init();
    mp_runtime_exec(code, size, filename);
    mp_runtime_deinit();
}

__attribute__((force_align_arg_pointer))
void run_micropython_mpy(const uint8_t *buf, size_t size) {
    mp_runtime_init();
    mp_runtime_exec_mpy(buf, size);
    mp_runtime_deinit();
}
