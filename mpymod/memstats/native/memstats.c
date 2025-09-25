#include "py/runtime.h"
#include "mem.h"

STATIC mp_obj_t memstats_heap_free(void) {
    return mp_obj_new_int_from_ull((unsigned long long)mem_heap_free());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(memstats_heap_free_obj, memstats_heap_free);

STATIC mp_obj_t memstats_register_app(mp_obj_t priority_obj) {
    int priority = mp_obj_get_int(priority_obj);
    int id = mem_register_app((uint8_t)priority);
    return mp_obj_new_int(id);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(memstats_register_app_obj, memstats_register_app);

STATIC mp_obj_t memstats_app_used(mp_obj_t app_id_obj) {
    int app_id = mp_obj_get_int(app_id_obj);
    size_t used = mem_app_used(app_id);
    return mp_obj_new_int_from_ull((unsigned long long)used);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(memstats_app_used_obj, memstats_app_used);

STATIC mp_obj_t memstats_save_block(mp_obj_t app_id_obj, mp_obj_t data_obj) {
    int app_id = mp_obj_get_int(app_id_obj);
    mp_uint_t len;
    const char *data = mp_obj_str_get_data(data_obj, &len);
    int handle = mem_save_app(app_id, data, len);
    return mp_obj_new_int(handle);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(memstats_save_block_obj, memstats_save_block);

STATIC mp_obj_t memstats_load_block(mp_obj_t app_id_obj, mp_obj_t handle_obj) {
    int app_id = mp_obj_get_int(app_id_obj);
    int handle = mp_obj_get_int(handle_obj);
    size_t size = 0;
    void *ptr = mem_retrieve_app(app_id, handle, &size);
    if (!ptr || size == 0) {
        return mp_const_none;
    }
    return mp_obj_new_bytes((const unsigned char *)ptr, size);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(memstats_load_block_obj, memstats_load_block);

STATIC const mp_rom_map_elem_t memstats_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_heap_free), MP_ROM_PTR(&memstats_heap_free_obj) },
    { MP_ROM_QSTR(MP_QSTR_register_app), MP_ROM_PTR(&memstats_register_app_obj) },
    { MP_ROM_QSTR(MP_QSTR_app_used), MP_ROM_PTR(&memstats_app_used_obj) },
    { MP_ROM_QSTR(MP_QSTR_save_block), MP_ROM_PTR(&memstats_save_block_obj) },
    { MP_ROM_QSTR(MP_QSTR_load_block), MP_ROM_PTR(&memstats_load_block_obj) },
};
STATIC MP_DEFINE_CONST_DICT(memstats_module_globals, memstats_module_globals_table);

const mp_obj_module_t memstats_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&memstats_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_memstats_native, memstats_native_user_cmodule);
