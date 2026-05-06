#include "py/runtime.h"
#include "io.h"

#ifndef STATIC
#define STATIC static
#endif

static int mouse_ready = 0;
static int has_phase = 0;
static uint8_t packet[3];
static uint8_t packet_index = 0;

static void mouse_wait_write(void) {
    for (uint32_t i = 0; i < 100000; ++i) {
        if ((io_inb(0x64) & 0x02) == 0) {
            return;
        }
    }
}

static int mouse_data_available(void) {
    return (io_inb(0x64) & 0x01) != 0;
}

static uint8_t mouse_read_data(void) {
    return io_inb(0x60);
}

static void mouse_write_command(uint8_t value) {
    mouse_wait_write();
    io_outb(0x64, 0xD4);
    mouse_wait_write();
    io_outb(0x60, value);
}

STATIC mp_obj_t mouse_init(void) {
    mouse_wait_write();
    io_outb(0x64, 0xA8);

    mouse_wait_write();
    io_outb(0x64, 0x20);
    uint8_t status = mouse_read_data();

    mouse_wait_write();
    io_outb(0x64, 0x60);
    mouse_wait_write();
    io_outb(0x60, status | 0x02);

    mouse_write_command(0xF6);
    if (mouse_data_available()) {
        mouse_read_data();
    }

    mouse_write_command(0xF4);
    if (mouse_data_available()) {
        mouse_read_data();
    }

    mouse_ready = 1;
    has_phase = 0;
    packet_index = 0;
    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mouse_init_obj, mouse_init);

STATIC mp_obj_t mouse_poll(void) {
    if (!mouse_ready) {
        return mp_const_none;
    }

    while (mouse_data_available()) {
        uint8_t value = mouse_read_data();
        if (!has_phase) {
            if ((value & 0x08) == 0) {
                continue;
            }
            has_phase = 1;
            packet_index = 0;
        }

        packet[packet_index++] = value;
        if (packet_index < 3) {
            continue;
        }

        packet_index = 0;
        mp_obj_t tuple[5];
        int dx = (int8_t)packet[1];
        int dy = -(int8_t)packet[2];
        tuple[0] = mp_obj_new_int(dx);
        tuple[1] = mp_obj_new_int(dy);
        tuple[2] = mp_obj_new_bool((packet[0] & 0x1) != 0);
        tuple[3] = mp_obj_new_bool((packet[0] & 0x2) != 0);
        tuple[4] = mp_obj_new_bool((packet[0] & 0x4) != 0);
        return mp_obj_new_tuple(5, tuple);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mouse_poll_obj, mouse_poll);

STATIC const mp_rom_map_elem_t mouse_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&mouse_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_poll), MP_ROM_PTR(&mouse_poll_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mouse_module_globals, mouse_module_globals_table);

const mp_obj_module_t mouse_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mouse_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_mouse_native, mouse_native_user_cmodule);
