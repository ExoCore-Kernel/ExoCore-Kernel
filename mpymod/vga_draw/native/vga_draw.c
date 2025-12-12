#include "py/runtime.h"
#include "console.h"
#include "runstate.h"
#include "vga_draw.h"

#ifndef STATIC
#define STATIC static
#endif

static inline uint8_t clamp_color(mp_int_t value) {
    if (value < 0) {
        value = 0;
    } else if (value > 15) {
        value = 15;
    }
    return (uint8_t)value;
}

static char char_from_obj(mp_obj_t obj, char default_char) {
    if (obj == MP_OBJ_NULL || obj == mp_const_none) {
        return default_char;
    }
    size_t len = 0;
    const char *data = mp_obj_str_get_data(obj, &len);
    if (len == 0) {
        return default_char;
    }
    return data[0];
}

STATIC mp_obj_t mp_vga_draw_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_fg,
        ARG_bg,
        ARG_char,
        ARG_clear,
    };
    static const mp_arg_t allowed[] = {
        { MP_QSTR_fg,    MP_ARG_KW_ONLY | MP_ARG_INT,  { .u_int = VGA_WHITE } },
        { MP_QSTR_bg,    MP_ARG_KW_ONLY | MP_ARG_INT,  { .u_int = VGA_BLACK } },
        { MP_QSTR_char,  MP_ARG_KW_ONLY | MP_ARG_OBJ,  { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_clear, MP_ARG_KW_ONLY | MP_ARG_BOOL, { .u_bool = true } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);

    if (vga_draw_is_active()) {
        mp_raise_ValueError(MP_ERROR_TEXT("drawing session already active"));
    }

    vga_draw_begin();
    uint8_t fg = clamp_color((mp_int_t)args[ARG_fg].u_int);
    uint8_t bg = clamp_color((mp_int_t)args[ARG_bg].u_int);
    char fill = char_from_obj(args[ARG_char].u_obj, ' ');
    if (args[ARG_clear].u_bool) {
        vga_draw_clear(fill, VGA_ATTR(fg, bg));
    }
    mp_vga_output = 0;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_vga_draw_start_obj, 0, mp_vga_draw_start);

STATIC void ensure_active(void) {
    if (!vga_draw_is_active()) {
        mp_raise_ValueError(MP_ERROR_TEXT("no active drawing session"));
    }
}

STATIC mp_obj_t mp_vga_draw_clear(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_char,
        ARG_fg,
        ARG_bg,
    };
    static const mp_arg_t allowed[] = {
        { MP_QSTR_char, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_fg,   MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = VGA_WHITE } },
        { MP_QSTR_bg,   MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = VGA_BLACK } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);
    ensure_active();
    char fill = char_from_obj(args[ARG_char].u_obj, ' ');
    uint8_t fg = clamp_color((mp_int_t)args[ARG_fg].u_int);
    uint8_t bg = clamp_color((mp_int_t)args[ARG_bg].u_int);
    vga_draw_clear(fill, VGA_ATTR(fg, bg));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_vga_draw_clear_obj, 0, mp_vga_draw_clear);

STATIC mp_obj_t mp_vga_draw_pixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_x,
        ARG_y,
        ARG_char,
        ARG_fg,
        ARG_bg,
    };
    static const mp_arg_t allowed[] = {
        { MP_QSTR_x,    MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_y,    MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_char, MP_ARG_OBJ,                  { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_fg,   MP_ARG_INT,                  { .u_int = VGA_WHITE } },
        { MP_QSTR_bg,   MP_ARG_INT,                  { .u_int = VGA_BLACK } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);
    ensure_active();
    int32_t x = (int32_t)(mp_int_t)args[ARG_x].u_int;
    int32_t y = (int32_t)(mp_int_t)args[ARG_y].u_int;
    char ch = char_from_obj(args[ARG_char].u_obj, (char)0xDB);
    uint8_t fg = clamp_color((mp_int_t)args[ARG_fg].u_int);
    uint8_t bg = clamp_color((mp_int_t)args[ARG_bg].u_int);
    vga_draw_set_cell(x, y, ch, VGA_ATTR(fg, bg));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_vga_draw_pixel_obj, 0, mp_vga_draw_pixel);

STATIC mp_obj_t mp_vga_draw_line(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_x0,
        ARG_y0,
        ARG_x1,
        ARG_y1,
        ARG_char,
        ARG_fg,
        ARG_bg,
    };
    static const mp_arg_t allowed[] = {
        { MP_QSTR_x0,   MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_y0,   MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_x1,   MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_y1,   MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_char, MP_ARG_OBJ,                  { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_fg,   MP_ARG_INT,                  { .u_int = VGA_WHITE } },
        { MP_QSTR_bg,   MP_ARG_INT,                  { .u_int = VGA_BLACK } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);
    ensure_active();
    int32_t x0 = (int32_t)(mp_int_t)args[ARG_x0].u_int;
    int32_t y0 = (int32_t)(mp_int_t)args[ARG_y0].u_int;
    int32_t x1 = (int32_t)(mp_int_t)args[ARG_x1].u_int;
    int32_t y1 = (int32_t)(mp_int_t)args[ARG_y1].u_int;
    char ch = char_from_obj(args[ARG_char].u_obj, (char)0xDB);
    uint8_t fg = clamp_color((mp_int_t)args[ARG_fg].u_int);
    uint8_t bg = clamp_color((mp_int_t)args[ARG_bg].u_int);
    vga_draw_line(x0, y0, x1, y1, ch, VGA_ATTR(fg, bg));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_vga_draw_line_obj, 0, mp_vga_draw_line);

STATIC mp_obj_t mp_vga_draw_rect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_x,
        ARG_y,
        ARG_w,
        ARG_h,
        ARG_char,
        ARG_fg,
        ARG_bg,
        ARG_fill,
    };
    static const mp_arg_t allowed[] = {
        { MP_QSTR_x,    MP_ARG_REQUIRED | MP_ARG_INT,  { .u_int = 0 } },
        { MP_QSTR_y,    MP_ARG_REQUIRED | MP_ARG_INT,  { .u_int = 0 } },
        { MP_QSTR_w,    MP_ARG_REQUIRED | MP_ARG_INT,  { .u_int = 0 } },
        { MP_QSTR_h,    MP_ARG_REQUIRED | MP_ARG_INT,  { .u_int = 0 } },
        { MP_QSTR_char, MP_ARG_OBJ,                   { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_fg,   MP_ARG_INT,                   { .u_int = VGA_WHITE } },
        { MP_QSTR_bg,   MP_ARG_INT,                   { .u_int = VGA_BLACK } },
        { MP_QSTR_fill, MP_ARG_BOOL,                  { .u_bool = false } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);
    ensure_active();
    int32_t x = (int32_t)(mp_int_t)args[ARG_x].u_int;
    int32_t y = (int32_t)(mp_int_t)args[ARG_y].u_int;
    int32_t w = (int32_t)(mp_int_t)args[ARG_w].u_int;
    int32_t h = (int32_t)(mp_int_t)args[ARG_h].u_int;
    char ch = char_from_obj(args[ARG_char].u_obj, (char)0xDB);
    uint8_t fg = clamp_color((mp_int_t)args[ARG_fg].u_int);
    uint8_t bg = clamp_color((mp_int_t)args[ARG_bg].u_int);
    vga_draw_rect(x, y, w, h, ch, VGA_ATTR(fg, bg), args[ARG_fill].u_bool);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_vga_draw_rect_obj, 0, mp_vga_draw_rect);

STATIC mp_obj_t mp_vga_draw_present(size_t n_args, const mp_obj_t *args) {
    bool unhide = true;
    if (n_args > 0) {
        unhide = mp_obj_is_true(args[0]);
    }
    ensure_active();
    vga_draw_present();
    if (unhide) {
        mp_vga_output = 1;
        vga_draw_end();
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_vga_draw_present_obj, 0, 1, mp_vga_draw_present);

STATIC mp_obj_t mp_vga_draw_show(void) {
    ensure_active();
    vga_draw_present();
    mp_vga_output = 1;
    vga_draw_end();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_vga_draw_show_obj, mp_vga_draw_show);

STATIC mp_obj_t mp_vga_draw_is_hidden(void) {
    return mp_obj_new_bool(mp_vga_output == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_vga_draw_is_hidden_obj, mp_vga_draw_is_hidden);

STATIC const mp_rom_map_elem_t vga_draw_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_vga_draw_native) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&mp_vga_draw_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&mp_vga_draw_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&mp_vga_draw_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&mp_vga_draw_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&mp_vga_draw_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_present), MP_ROM_PTR(&mp_vga_draw_present_obj) },
    { MP_ROM_QSTR(MP_QSTR_show), MP_ROM_PTR(&mp_vga_draw_show_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_hidden), MP_ROM_PTR(&mp_vga_draw_is_hidden_obj) },
    { MP_ROM_QSTR(MP_QSTR_WIDTH), MP_ROM_INT(VGA_DRAW_COLS) },
    { MP_ROM_QSTR(MP_QSTR_HEIGHT), MP_ROM_INT(VGA_DRAW_ROWS) },
    { MP_ROM_QSTR(MP_QSTR_VGA_BLACK), MP_ROM_INT(VGA_BLACK) },
    { MP_ROM_QSTR(MP_QSTR_VGA_BLUE), MP_ROM_INT(VGA_BLUE) },
    { MP_ROM_QSTR(MP_QSTR_VGA_GREEN), MP_ROM_INT(VGA_GREEN) },
    { MP_ROM_QSTR(MP_QSTR_VGA_CYAN), MP_ROM_INT(VGA_CYAN) },
    { MP_ROM_QSTR(MP_QSTR_VGA_RED), MP_ROM_INT(VGA_RED) },
    { MP_ROM_QSTR(MP_QSTR_VGA_MAGENTA), MP_ROM_INT(VGA_MAGENTA) },
    { MP_ROM_QSTR(MP_QSTR_VGA_BROWN), MP_ROM_INT(VGA_BROWN) },
    { MP_ROM_QSTR(MP_QSTR_VGA_LIGHT_GREY), MP_ROM_INT(VGA_LIGHT_GREY) },
    { MP_ROM_QSTR(MP_QSTR_VGA_DARK_GREY), MP_ROM_INT(VGA_DARK_GREY) },
    { MP_ROM_QSTR(MP_QSTR_VGA_LIGHT_BLUE), MP_ROM_INT(VGA_LIGHT_BLUE) },
    { MP_ROM_QSTR(MP_QSTR_VGA_LIGHT_GREEN), MP_ROM_INT(VGA_LIGHT_GREEN) },
    { MP_ROM_QSTR(MP_QSTR_VGA_LIGHT_CYAN), MP_ROM_INT(VGA_LIGHT_CYAN) },
    { MP_ROM_QSTR(MP_QSTR_VGA_LIGHT_RED), MP_ROM_INT(VGA_LIGHT_RED) },
    { MP_ROM_QSTR(MP_QSTR_VGA_LIGHT_MAGENTA), MP_ROM_INT(VGA_LIGHT_MAGENTA) },
    { MP_ROM_QSTR(MP_QSTR_VGA_YELLOW), MP_ROM_INT(VGA_YELLOW) },
    { MP_ROM_QSTR(MP_QSTR_VGA_WHITE), MP_ROM_INT(VGA_WHITE) },
};
STATIC MP_DEFINE_CONST_DICT(vga_draw_module_globals, vga_draw_module_globals_table);

const mp_obj_module_t vga_draw_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&vga_draw_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_vga_draw_native, vga_draw_native_user_cmodule);
