#include "py/runtime.h"
#include "runstate.h"
#include <string.h>

#ifndef STATIC
#define STATIC static
#endif

STATIC char current_program_buf[64];

STATIC mp_obj_t runstatectl_current_program(void) {
    const char *name = current_program ? current_program : "";
    return mp_obj_new_str(name, strlen(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(runstatectl_current_program_obj, runstatectl_current_program);

STATIC mp_obj_t runstatectl_set_current_program(mp_obj_t name_obj) {
    mp_uint_t len;
    const char *src = mp_obj_str_get_data(name_obj, &len);
    if (len >= sizeof(current_program_buf)) {
        len = sizeof(current_program_buf) - 1;
    }
    memcpy(current_program_buf, src, len);
    current_program_buf[len] = '\0';
    current_program = current_program_buf;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(runstatectl_set_current_program_obj, runstatectl_set_current_program);

STATIC mp_obj_t runstatectl_current_user_app(void) {
    return mp_obj_new_int(current_user_app);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(runstatectl_current_user_app_obj, runstatectl_current_user_app);

STATIC mp_obj_t runstatectl_set_current_user_app(mp_obj_t value_obj) {
    current_user_app = mp_obj_get_int(value_obj);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(runstatectl_set_current_user_app_obj, runstatectl_set_current_user_app);

STATIC mp_obj_t runstatectl_is_debug_mode(void) {
    return mp_obj_new_bool(debug_mode != 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(runstatectl_is_debug_mode_obj, runstatectl_is_debug_mode);

STATIC mp_obj_t runstatectl_set_debug_mode(mp_obj_t flag_obj) {
    debug_mode = mp_obj_is_true(flag_obj);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(runstatectl_set_debug_mode_obj, runstatectl_set_debug_mode);

STATIC mp_obj_t runstatectl_is_vga_output(void) {
    return mp_obj_new_bool(mp_vga_output != 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(runstatectl_is_vga_output_obj, runstatectl_is_vga_output);

STATIC mp_obj_t runstatectl_set_vga_output(mp_obj_t flag_obj) {
    mp_vga_output = mp_obj_is_true(flag_obj);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(runstatectl_set_vga_output_obj, runstatectl_set_vga_output);

STATIC const mp_rom_map_elem_t runstatectl_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_current_program), MP_ROM_PTR(&runstatectl_current_program_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_current_program), MP_ROM_PTR(&runstatectl_set_current_program_obj) },
    { MP_ROM_QSTR(MP_QSTR_current_user_app), MP_ROM_PTR(&runstatectl_current_user_app_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_current_user_app), MP_ROM_PTR(&runstatectl_set_current_user_app_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_debug_mode), MP_ROM_PTR(&runstatectl_is_debug_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_debug_mode), MP_ROM_PTR(&runstatectl_set_debug_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_vga_output), MP_ROM_PTR(&runstatectl_is_vga_output_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_vga_output), MP_ROM_PTR(&runstatectl_set_vga_output_obj) },
};
STATIC MP_DEFINE_CONST_DICT(runstatectl_module_globals, runstatectl_module_globals_table);

const mp_obj_module_t runstatectl_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&runstatectl_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_runstatectl_native, runstatectl_native_user_cmodule);
