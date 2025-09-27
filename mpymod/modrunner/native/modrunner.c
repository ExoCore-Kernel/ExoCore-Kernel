#include "py/runtime.h"
#include "modexec.h"

#ifndef STATIC
#define STATIC static
#endif

STATIC mp_obj_t modrunner_run(mp_obj_t name_obj) {
    mp_uint_t len;
    const char *name = mp_obj_str_get_data(name_obj, &len);
    int rc = modexec_run(name);
    return mp_obj_new_int(rc);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modrunner_run_obj, modrunner_run);

STATIC const mp_rom_map_elem_t modrunner_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&modrunner_run_obj) },
};
STATIC MP_DEFINE_CONST_DICT(modrunner_module_globals, modrunner_module_globals_table);

const mp_obj_module_t modrunner_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&modrunner_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_modrunner_native, modrunner_native_user_cmodule);
