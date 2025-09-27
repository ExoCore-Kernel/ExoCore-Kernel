#include "py/runtime.h"
#include "py/objstr.h"
#include "fs.h"
#include <string.h>

#ifndef STATIC
#define STATIC static
#endif

STATIC mp_obj_t fsbridge_read(mp_obj_t offset_obj, mp_obj_t length_obj) {
    size_t offset = (size_t)mp_obj_get_int(offset_obj);
    size_t length = (size_t)mp_obj_get_int(length_obj);
    if (length == 0) {
        return mp_const_empty_bytes;
    }
    vstr_t vstr;
    vstr_init_len(&vstr, length);
    size_t read = fs_read(offset, vstr.buf, length);
    if (read < length) {
        vstr.len = read;
    }
    mp_obj_t result = mp_obj_new_bytes((const unsigned char *)vstr.buf, vstr.len);
    vstr_clear(&vstr);
    return result;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(fsbridge_read_obj, fsbridge_read);

STATIC mp_obj_t fsbridge_write(mp_obj_t offset_obj, mp_obj_t data_obj) {
    size_t offset = (size_t)mp_obj_get_int(offset_obj);
    mp_uint_t len;
    const char *data = mp_obj_str_get_data(data_obj, &len);
    size_t written = fs_write(offset, data, len);
    return mp_obj_new_int_from_uint((mp_uint_t)written);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(fsbridge_write_obj, fsbridge_write);

STATIC mp_obj_t fsbridge_capacity(void) {
    return mp_obj_new_int_from_ull((unsigned long long)fs_capacity());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(fsbridge_capacity_obj, fsbridge_capacity);

STATIC mp_obj_t fsbridge_is_mounted(void) {
    return mp_obj_new_bool(fs_is_mounted());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(fsbridge_is_mounted_obj, fsbridge_is_mounted);

STATIC const mp_rom_map_elem_t fsbridge_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&fsbridge_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&fsbridge_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_capacity), MP_ROM_PTR(&fsbridge_capacity_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_mounted), MP_ROM_PTR(&fsbridge_is_mounted_obj) },
};
STATIC MP_DEFINE_CONST_DICT(fsbridge_module_globals, fsbridge_module_globals_table);

const mp_obj_module_t fsbridge_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&fsbridge_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_fsbridge_native, fsbridge_native_user_cmodule);
