#include "py/runtime.h"
#include "serial.h"

STATIC mp_obj_t serialctl_write(mp_obj_t text_obj) {
    mp_uint_t len;
    const char *data = mp_obj_str_get_data(text_obj, &len);
    for (mp_uint_t i = 0; i < len; ++i) {
        serial_putc(data[i]);
    }
    return mp_obj_new_int(len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(serialctl_write_obj, serialctl_write);

STATIC mp_obj_t serialctl_write_hex(mp_obj_t value_obj) {
    uint64_t value = mp_obj_get_int_truncated(value_obj);
    serial_uhex(value);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(serialctl_write_hex_obj, serialctl_write_hex);

STATIC mp_obj_t serialctl_write_dec(mp_obj_t value_obj) {
    uint32_t value = (uint32_t)mp_obj_get_int(value_obj);
    serial_udec(value);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(serialctl_write_dec_obj, serialctl_write_dec);

STATIC mp_obj_t serialctl_raw_write(mp_obj_t data_obj) {
    mp_uint_t len;
    const char *data = mp_obj_str_get_data(data_obj, &len);
    for (mp_uint_t i = 0; i < len; ++i) {
        serial_raw_putc(data[i]);
    }
    return mp_obj_new_int(len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(serialctl_raw_write_obj, serialctl_raw_write);

STATIC const mp_rom_map_elem_t serialctl_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&serialctl_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_hex), MP_ROM_PTR(&serialctl_write_hex_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_dec), MP_ROM_PTR(&serialctl_write_dec_obj) },
    { MP_ROM_QSTR(MP_QSTR_raw_write), MP_ROM_PTR(&serialctl_raw_write_obj) },
};
STATIC MP_DEFINE_CONST_DICT(serialctl_module_globals, serialctl_module_globals_table);

const mp_obj_module_t serialctl_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&serialctl_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_serialctl_native, serialctl_native_user_cmodule);
