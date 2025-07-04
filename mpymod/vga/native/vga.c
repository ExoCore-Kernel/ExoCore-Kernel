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

const mp_obj_module_t mp_module_vga = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&vga_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_vga, mp_module_vga);
