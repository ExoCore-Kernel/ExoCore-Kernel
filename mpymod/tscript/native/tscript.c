#include "py/runtime.h"
#include "script.h"

#ifndef STATIC
#define STATIC static
#endif

STATIC mp_obj_t tscript_run(mp_obj_t code_obj) {
    mp_uint_t len;
    const char *code = mp_obj_str_get_data(code_obj, &len);
    run_script(code, len);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(tscript_run_obj, tscript_run);

STATIC const mp_rom_map_elem_t tscript_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&tscript_run_obj) },
};
STATIC MP_DEFINE_CONST_DICT(tscript_module_globals, tscript_module_globals_table);

const mp_obj_module_t tscript_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&tscript_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_tscript_native, tscript_native_user_cmodule);
