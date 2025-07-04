#include "py/runtime.h"
extern int mp_vga_output;

static mp_obj_t vga_enable(mp_obj_t enable) {
    mp_vga_output = mp_obj_is_true(enable);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(vga_enable_obj, vga_enable);

static const mp_rom_map_elem_t vga_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_enable), MP_ROM_PTR(&vga_enable_obj) },
};
static MP_DEFINE_CONST_DICT(vga_module_globals, vga_globals_table);

const mp_obj_module_t mp_module_vga = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&vga_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_vga, mp_module_vga);
