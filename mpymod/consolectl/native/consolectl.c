#include "py/runtime.h"
#include "console.h"
#include "framebuffer.h"
#include <string.h>
#include <stdint.h>

#ifndef STATIC
#define STATIC static
#endif

STATIC mp_obj_t consolectl_write(mp_obj_t text_obj) {
    mp_uint_t len;
    const char *data = mp_obj_str_get_data(text_obj, &len);
    for (mp_uint_t i = 0; i < len; ++i) {
        console_putc(data[i]);
    }
    return mp_obj_new_int(len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(consolectl_write_obj, consolectl_write);

STATIC mp_obj_t consolectl_clear(void) {
    console_clear();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(consolectl_clear_obj, consolectl_clear);

STATIC mp_obj_t consolectl_set_attr(mp_obj_t fg_obj, mp_obj_t bg_obj) {
    uint8_t fg = (uint8_t)mp_obj_get_int(fg_obj);
    uint8_t bg = (uint8_t)mp_obj_get_int(bg_obj);
    console_set_attr(fg, bg);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(consolectl_set_attr_obj, consolectl_set_attr);

STATIC mp_obj_t consolectl_scroll(mp_obj_t lines_obj) {
    int lines = mp_obj_get_int(lines_obj);
    if (lines > 0) {
        for (int i = 0; i < lines; ++i) {
            console_scroll_down();
        }
    } else if (lines < 0) {
        for (int i = 0; i < -lines; ++i) {
            console_scroll_up();
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(consolectl_scroll_obj, consolectl_scroll);

STATIC mp_obj_t consolectl_backspace(mp_obj_t count_obj) {
    int count = mp_obj_get_int(count_obj);
    if (count < 0) {
        count = 0;
    }
    for (int i = 0; i < count; ++i) {
        console_backspace();
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(consolectl_backspace_obj, consolectl_backspace);


STATIC mp_obj_t consolectl_blit_pixels(size_t n_args, const mp_obj_t *args) {
    if (n_args < 3 || n_args > 6) {
        mp_raise_TypeError(MP_ERROR_TEXT("blit_pixels expects 3-6 args"));
    }

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[0], &bufinfo, MP_BUFFER_READ);

    uint32_t width = (uint32_t)mp_obj_get_int(args[1]);
    uint32_t height = (uint32_t)mp_obj_get_int(args[2]);
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t stride = 0;

    if (n_args >= 4) {
        x = (uint32_t)mp_obj_get_int(args[3]);
    }
    if (n_args >= 5) {
        y = (uint32_t)mp_obj_get_int(args[4]);
    }
    if (n_args >= 6) {
        stride = (uint32_t)mp_obj_get_int(args[5]);
    }
    if (stride == 0) {
        stride = width * 3u;
    }

    size_t required = (size_t)stride * (size_t)height;
    if (bufinfo.len < required) {
        mp_raise_ValueError(MP_ERROR_TEXT("pixel buffer too small"));
    }

    int ok = framebuffer_blit_rgb24(x, y, width, height, (const uint8_t *)bufinfo.buf, stride);
    return mp_obj_new_bool(ok);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(consolectl_blit_pixels_obj, 3, 6, consolectl_blit_pixels);


STATIC mp_obj_t consolectl_blit_pattern(size_t n_args, const mp_obj_t *args) {
    if (n_args < 3 || n_args > 5) {
        mp_raise_TypeError(MP_ERROR_TEXT("blit_pattern expects 3-5 args"));
    }
    uint32_t frame = (uint32_t)mp_obj_get_int(args[0]);
    uint32_t width = (uint32_t)mp_obj_get_int(args[1]);
    uint32_t height = (uint32_t)mp_obj_get_int(args[2]);
    uint32_t x0 = 0;
    uint32_t y0 = 0;
    if (n_args >= 4) {
        x0 = (uint32_t)mp_obj_get_int(args[3]);
    }
    if (n_args >= 5) {
        y0 = (uint32_t)mp_obj_get_int(args[4]);
    }

    if (width == 0 || height == 0) {
        return mp_const_false;
    }

    uint8_t rgb[3];
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            rgb[0] = (uint8_t)((x * 5u + frame * 7u) & 0xFFu);
            rgb[1] = (uint8_t)((y * 6u + frame * 5u) & 0xFFu);
            rgb[2] = (uint8_t)(((x + y) * 3u + frame * 11u) & 0xFFu);
            framebuffer_blit_rgb24(x0 + x, y0 + y, 1, 1, rgb, 3);
        }
    }
    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(consolectl_blit_pattern_obj, 3, 5, consolectl_blit_pattern);

STATIC mp_obj_t consolectl_colors(void) {
    STATIC const struct {
        const char *name;
        uint8_t value;
    } table[] = {
        {"black", VGA_BLACK},
        {"blue", VGA_BLUE},
        {"green", VGA_GREEN},
        {"cyan", VGA_CYAN},
        {"red", VGA_RED},
        {"magenta", VGA_MAGENTA},
        {"brown", VGA_BROWN},
        {"light_grey", VGA_LIGHT_GREY},
        {"dark_grey", VGA_DARK_GREY},
        {"light_blue", VGA_LIGHT_BLUE},
        {"light_green", VGA_LIGHT_GREEN},
        {"light_cyan", VGA_LIGHT_CYAN},
        {"light_red", VGA_LIGHT_RED},
        {"light_magenta", VGA_LIGHT_MAGENTA},
        {"yellow", VGA_YELLOW},
        {"white", VGA_WHITE},
    };
    mp_obj_t dict = mp_obj_new_dict(0);
    for (size_t i = 0; i < MP_ARRAY_SIZE(table); ++i) {
        mp_obj_dict_store(dict,
            mp_obj_new_str(table[i].name, strlen(table[i].name)),
            mp_obj_new_int(table[i].value));
    }
    return dict;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(consolectl_colors_obj, consolectl_colors);

STATIC const mp_rom_map_elem_t consolectl_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&consolectl_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&consolectl_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_attr), MP_ROM_PTR(&consolectl_set_attr_obj) },
    { MP_ROM_QSTR(MP_QSTR_scroll), MP_ROM_PTR(&consolectl_scroll_obj) },
    { MP_ROM_QSTR(MP_QSTR_backspace), MP_ROM_PTR(&consolectl_backspace_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_pixels), MP_ROM_PTR(&consolectl_blit_pixels_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit_pattern), MP_ROM_PTR(&consolectl_blit_pattern_obj) },
    { MP_ROM_QSTR(MP_QSTR_colors), MP_ROM_PTR(&consolectl_colors_obj) },
};
STATIC MP_DEFINE_CONST_DICT(consolectl_module_globals, consolectl_module_globals_table);

const mp_obj_module_t consolectl_native_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&consolectl_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_consolectl_native, consolectl_native_user_cmodule);
