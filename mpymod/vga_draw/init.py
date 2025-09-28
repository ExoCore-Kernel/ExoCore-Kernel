from env import env
from vga_draw_native import (
    start as _start,
    clear as _clear,
    pixel as _pixel,
    line as _line,
    rect as _rect,
    present as _present,
    show as _show,
    is_hidden as _is_hidden,
    WIDTH as WIDTH,
    HEIGHT as HEIGHT,
    VGA_BLACK,
    VGA_BLUE,
    VGA_GREEN,
    VGA_CYAN,
    VGA_RED,
    VGA_MAGENTA,
    VGA_BROWN,
    VGA_LIGHT_GREY,
    VGA_DARK_GREY,
    VGA_LIGHT_BLUE,
    VGA_LIGHT_GREEN,
    VGA_LIGHT_CYAN,
    VGA_LIGHT_RED,
    VGA_LIGHT_MAGENTA,
    VGA_YELLOW,
    VGA_WHITE,
)

start = _start
clear = _clear
pixel = _pixel
line = _line
rect = _rect
present = _present
show = _show
is_hidden = _is_hidden

COLORS = {
    'BLACK': VGA_BLACK,
    'BLUE': VGA_BLUE,
    'GREEN': VGA_GREEN,
    'CYAN': VGA_CYAN,
    'RED': VGA_RED,
    'MAGENTA': VGA_MAGENTA,
    'BROWN': VGA_BROWN,
    'LIGHT_GREY': VGA_LIGHT_GREY,
    'DARK_GREY': VGA_DARK_GREY,
    'LIGHT_BLUE': VGA_LIGHT_BLUE,
    'LIGHT_GREEN': VGA_LIGHT_GREEN,
    'LIGHT_CYAN': VGA_LIGHT_CYAN,
    'LIGHT_RED': VGA_LIGHT_RED,
    'LIGHT_MAGENTA': VGA_LIGHT_MAGENTA,
    'YELLOW': VGA_YELLOW,
    'WHITE': VGA_WHITE,
}

env['vga_draw'] = {
    'width': WIDTH,
    'height': HEIGHT,
    'colors': COLORS,
}
