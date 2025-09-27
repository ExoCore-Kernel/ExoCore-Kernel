#include "py/runtime.h"
#include "debuglog.h"

#ifndef STATIC
#define STATIC static
#endif

STATIC mp_obj_t debugview_flush(void) {
    debuglog_flush();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(debugview_flush_obj, debugview_flush);

STATIC mp_obj_t debugview_dump_console(void) {
    debuglog_dump_console();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(debugview_dump_console_obj, debugview_dump_console);

STATIC mp_obj_t debugview_save_file(void) {
    debuglog_save_file();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(debugview_save_file_obj, debugview_save_file);

STATIC mp_obj_t debugview_buffer(void) {
    const char *buf = debuglog_buffer();
    size_t len = debuglog_length();
    if (!buf || len == 0) {
        return mp_const_empty_bytes;
    }
    return mp_obj_new_bytes((const unsigned char *)buf, len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(debugview_buffer_obj, debugview_buffer);

STATIC mp_obj_t debugview_is_available(void) {
    return mp_obj_new_bool(debuglog_buffer() != NULL);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(debugview_is_available_obj, debugview_is_available);

STATIC const mp_rom_map_elem_t debugview_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&debugview_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_dump_console), MP_ROM_PTR(&debugview_dump_console_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_file), MP_ROM_PTR(&debugview_save_file_obj) },
    { MP_ROM_QSTR(MP_QSTR_buffer), MP_ROM_PTR(&debugview_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_available), MP_ROM_PTR(&debugview_is_available_obj) },
};
STATIC MP_DEFINE_CONST_DICT(debugview_module_globals, debugview_module_globals_table);

const mp_obj_module_t debugview_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&debugview_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_debugview_native, debugview_native_user_cmodule);
