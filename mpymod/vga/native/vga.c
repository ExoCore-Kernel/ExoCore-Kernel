#include "py/runtime.h"
#include "runstate.h"

#ifndef STATIC
#define STATIC static
#endif

STATIC mp_obj_t vga_enable(mp_obj_t flag) {
    mp_vga_output = mp_obj_is_true(flag);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(vga_enable_obj, vga_enable);

STATIC const mp_rom_map_elem_t vga_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_enable), MP_ROM_PTR(&vga_enable_obj) },
};
STATIC MP_DEFINE_CONST_DICT(vga_module_globals, vga_module_globals_table);

// MicroPython module definition
const mp_obj_module_t vga_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&vga_module_globals,
};

// Register the module so MicroPython can automatically include it
MP_REGISTER_MODULE(MP_QSTR_vga, vga_user_cmodule);
