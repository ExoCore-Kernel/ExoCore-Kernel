#include "py/runtime.h"
#include "console.h"

#ifndef STATIC
#define STATIC static
#endif

STATIC mp_obj_t keyinput_read_code(void) {
    int value = (unsigned char)console_getc();
    return mp_obj_new_int(value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(keyinput_read_code_obj, keyinput_read_code);

STATIC mp_obj_t keyinput_read_char(void) {
    int value = (unsigned char)console_getc();
    if (value >= 32 || value == '\n' || value == '\r' || value == '\t') {
        char ch = (char)value;
        return mp_obj_new_str(&ch, 1);
    }
    if (value == '\b') {
        char ch = '\b';
        return mp_obj_new_str(&ch, 1);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(keyinput_read_char_obj, keyinput_read_char);

STATIC const mp_rom_map_elem_t keyinput_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_char), MP_ROM_PTR(&keyinput_read_char_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_code), MP_ROM_PTR(&keyinput_read_code_obj) },
};
STATIC MP_DEFINE_CONST_DICT(keyinput_module_globals, keyinput_module_globals_table);

const mp_obj_module_t keyinput_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&keyinput_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_keyinput_native, keyinput_native_user_cmodule);
