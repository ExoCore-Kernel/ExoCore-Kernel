#include "vga_draw.h"
#include "console.h"
#include "mem.h"
#include "runstate.h"

#include <stddef.h>

static uint16_t back_buffer[VGA_DRAW_ROWS * VGA_DRAW_COLS];
static int active = 0;
static int initialised = 0;
static volatile uint64_t *front_buffer;

static inline uint16_t pack_cell(char ch, uint8_t attr) {
    return ((uint16_t)attr << 8) | (uint8_t)ch;
}

static void fill_cells(uint16_t *dst, uint32_t count, uint16_t cell) {
    if (count == 0) {
        return;
    }
    uint64_t pattern = (uint64_t)cell;
    pattern |= pattern << 16;
    pattern |= pattern << 32;
    uint32_t blocks = count / 4;
    uint32_t tail = count % 4;
    uint64_t *dst64 = (uint64_t *)dst;
    for (uint32_t i = 0; i < blocks; ++i) {
        dst64[i] = pattern;
    }
    dst += blocks * 4;
    for (uint32_t i = 0; i < tail; ++i) {
        dst[i] = cell;
    }
}

void vga_draw_init(void) {
    uint16_t clear_cell = pack_cell(' ', VGA_ATTR(VGA_WHITE, VGA_BLACK));
    fill_cells(back_buffer, VGA_DRAW_ROWS * VGA_DRAW_COLS, clear_cell);
    active = 0;
    initialised = 1;
    mem_vram_lock("vga_draw");
    front_buffer = (volatile uint64_t *)mem_vram_base();
}

void vga_draw_begin(void) {
    if (!initialised) {
        vga_draw_init();
    }
    active = 1;
}

void vga_draw_end(void) {
    active = 0;
}

int vga_draw_is_active(void) {
    return active;
}

void vga_draw_clear(char ch, uint8_t attr) {
    if (!active) {
        return;
    }
    uint16_t cell = pack_cell(ch, attr);
    fill_cells(back_buffer, VGA_DRAW_ROWS * VGA_DRAW_COLS, cell);
}

void vga_draw_set_cell(int32_t x, int32_t y, char ch, uint8_t attr) {
    if (!active) {
        return;
    }
    if (x < 0 || x >= VGA_DRAW_COLS || y < 0 || y >= VGA_DRAW_ROWS) {
        return;
    }
    back_buffer[y * VGA_DRAW_COLS + (uint32_t)x] = pack_cell(ch, attr);
}

void vga_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                   char ch, uint8_t attr) {
    if (!active) {
        return;
    }
    int32_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx - dy;
    while (1) {
        if (x0 >= 0 && x0 < VGA_DRAW_COLS && y0 >= 0 && y0 < VGA_DRAW_ROWS) {
            back_buffer[(uint32_t)y0 * VGA_DRAW_COLS + (uint32_t)x0] = pack_cell(ch, attr);
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int32_t e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void vga_draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                   char ch, uint8_t attr, int fill) {
    if (!active) {
        return;
    }
    if (w <= 0 || h <= 0) {
        return;
    }
    if (x >= VGA_DRAW_COLS || y >= VGA_DRAW_ROWS) {
        return;
    }
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (w <= 0 || h <= 0) {
        return;
    }
    if (x + w > VGA_DRAW_COLS) {
        w = VGA_DRAW_COLS - x;
    }
    if (y + h > VGA_DRAW_ROWS) {
        h = VGA_DRAW_ROWS - y;
    }
    uint16_t cell = pack_cell(ch, attr);
    if (fill) {
        for (int32_t row = 0; row < h; ++row) {
            uint16_t *line = &back_buffer[(uint32_t)(y + row) * VGA_DRAW_COLS + (uint32_t)x];
            fill_cells(line, (uint32_t)w, cell);
        }
    } else {
        uint16_t *top = &back_buffer[(uint32_t)y * VGA_DRAW_COLS + (uint32_t)x];
        fill_cells(top, (uint32_t)w, cell);
        if (h > 1) {
            uint16_t *bottom = &back_buffer[(uint32_t)(y + h - 1) * VGA_DRAW_COLS + (uint32_t)x];
            fill_cells(bottom, (uint32_t)w, cell);
            for (int32_t row = y + 1; row < y + h - 1; ++row) {
                uint16_t *line = &back_buffer[(uint32_t)row * VGA_DRAW_COLS + (uint32_t)x];
                line[0] = cell;
                if (w > 1) {
                    line[w - 1] = cell;
                }
            }
        }
    }
}

void vga_draw_present(void) {
    if (!initialised) {
        return;
    }
    volatile uint64_t *dest = front_buffer;
    const uint64_t *src = (const uint64_t *)back_buffer;
    size_t count = (VGA_DRAW_ROWS * VGA_DRAW_COLS) / 4;
    for (size_t i = 0; i < count; ++i) {
        dest[i] = src[i];
    }
}
