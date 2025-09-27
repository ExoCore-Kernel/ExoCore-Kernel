#include "py/runtime.h"
#include "io.h"
#include <stdint.h>

#ifndef STATIC
#define STATIC static
#endif

STATIC mp_obj_t hwinfo_rdtsc(void) {
    return mp_obj_new_int_from_ull(io_rdtsc());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(hwinfo_rdtsc_obj, hwinfo_rdtsc);

STATIC mp_obj_t hwinfo_cpuid(mp_obj_t leaf_obj) {
    uint32_t leaf = (uint32_t)mp_obj_get_int(leaf_obj);
    uint32_t a, b, c, d;
    io_cpuid(leaf, &a, &b, &c, &d);
    mp_obj_t tuple[4] = {
        mp_obj_new_int_from_uint(a),
        mp_obj_new_int_from_uint(b),
        mp_obj_new_int_from_uint(c),
        mp_obj_new_int_from_uint(d),
    };
    return mp_obj_new_tuple(4, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(hwinfo_cpuid_obj, hwinfo_cpuid);

STATIC mp_obj_t hwinfo_inb(mp_obj_t port_obj) {
    uint16_t port = (uint16_t)mp_obj_get_int(port_obj);
    uint8_t value = io_inb(port);
    return mp_obj_new_int_from_uint(value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(hwinfo_inb_obj, hwinfo_inb);

STATIC mp_obj_t hwinfo_outb(mp_obj_t port_obj, mp_obj_t value_obj) {
    uint16_t port = (uint16_t)mp_obj_get_int(port_obj);
    uint8_t value = (uint8_t)mp_obj_get_int(value_obj);
    io_outb(port, value);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(hwinfo_outb_obj, hwinfo_outb);

STATIC const mp_rom_map_elem_t hwinfo_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_rdtsc), MP_ROM_PTR(&hwinfo_rdtsc_obj) },
    { MP_ROM_QSTR(MP_QSTR_cpuid), MP_ROM_PTR(&hwinfo_cpuid_obj) },
    { MP_ROM_QSTR(MP_QSTR_inb), MP_ROM_PTR(&hwinfo_inb_obj) },
    { MP_ROM_QSTR(MP_QSTR_outb), MP_ROM_PTR(&hwinfo_outb_obj) },
};
STATIC MP_DEFINE_CONST_DICT(hwinfo_module_globals, hwinfo_module_globals_table);

const mp_obj_module_t hwinfo_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&hwinfo_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_hwinfo_native, hwinfo_native_user_cmodule);
