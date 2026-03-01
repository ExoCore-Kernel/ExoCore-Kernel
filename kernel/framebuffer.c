#include "framebuffer.h"
#include "console.h"
#include "font_petme128_8x8.h"

typedef struct {
    uint8_t *base;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
    uint8_t bytes_per_pixel;
    uint8_t red_position;
    uint8_t red_size;
    uint8_t green_position;
    uint8_t green_size;
    uint8_t blue_position;
    uint8_t blue_size;
    uint8_t reserved_position;
    uint8_t reserved_size;
    uint32_t palette[16];
    int available;
    int enabled;
} framebuffer_state_t;

static framebuffer_state_t fb = {0};

static const uint8_t vga_palette[16][3] = {
    {0x00, 0x00, 0x00}, /* VGA_BLACK */
    {0x00, 0x00, 0xAA}, /* VGA_BLUE */
    {0x00, 0xAA, 0x00}, /* VGA_GREEN */
    {0x00, 0xAA, 0xAA}, /* VGA_CYAN */
    {0xAA, 0x00, 0x00}, /* VGA_RED */
    {0xAA, 0x00, 0xAA}, /* VGA_MAGENTA */
    {0xAA, 0x55, 0x00}, /* VGA_BROWN */
    {0xAA, 0xAA, 0xAA}, /* VGA_LIGHT_GREY */
    {0x55, 0x55, 0x55}, /* VGA_DARK_GREY */
    {0x55, 0x55, 0xFF}, /* VGA_LIGHT_BLUE */
    {0x55, 0xFF, 0x55}, /* VGA_LIGHT_GREEN */
    {0x55, 0xFF, 0xFF}, /* VGA_LIGHT_CYAN */
    {0xFF, 0x55, 0x55}, /* VGA_LIGHT_RED */
    {0xFF, 0x55, 0xFF}, /* VGA_LIGHT_MAGENTA */
    {0xFF, 0xFF, 0x55}, /* VGA_YELLOW */
    {0xFF, 0xFF, 0xFF}, /* VGA_WHITE */
};

static uint32_t scale_component(uint8_t value, uint8_t size) {
    if (size == 0) {
        return 0;
    }
    if (size >= 8) {
        return value;
    }
    uint32_t max = (1u << size) - 1u;
    return (uint32_t)((value * max + 127u) / 255u);
}

static uint32_t pack_color(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = 0;
    color |= scale_component(r, fb.red_size) << fb.red_position;
    color |= scale_component(g, fb.green_size) << fb.green_position;
    color |= scale_component(b, fb.blue_size) << fb.blue_position;
    if (fb.reserved_size > 0) {
        color |= scale_component(0xFF, fb.reserved_size) << fb.reserved_position;
    }
    return color;
}

static void refresh_palette(void) {
    for (int i = 0; i < 16; ++i) {
        fb.palette[i] = pack_color(vga_palette[i][0], vga_palette[i][1], vga_palette[i][2]);
    }
}

void framebuffer_configure(uint64_t addr, uint32_t pitch, uint32_t width, uint32_t height,
                           uint8_t bpp, uint8_t type,
                           uint8_t red_position, uint8_t red_size,
                           uint8_t green_position, uint8_t green_size,
                           uint8_t blue_position, uint8_t blue_size,
                           uint8_t reserved_position, uint8_t reserved_size) {
    if (addr == 0 || width == 0 || height == 0 || bpp == 0) {
        fb.available = 0;
        fb.enabled = 0;
        return;
    }
    fb.base = (uint8_t *)(uintptr_t)addr;
    fb.pitch = pitch;
    fb.width = width;
    fb.height = height;
    fb.bpp = bpp;
    fb.bytes_per_pixel = (uint8_t)((bpp + 7) / 8);
    fb.red_position = red_position;
    fb.red_size = red_size;
    fb.green_position = green_position;
    fb.green_size = green_size;
    fb.blue_position = blue_position;
    fb.blue_size = blue_size;
    fb.reserved_position = reserved_position;
    fb.reserved_size = reserved_size;
    fb.available = (type == 1) && (fb.bytes_per_pixel >= 2);
    if (fb.available) {
        refresh_palette();
    } else {
        fb.enabled = 0;
    }
}

void framebuffer_enable(int enabled) {
    fb.enabled = (enabled && fb.available) ? 1 : 0;
}

int framebuffer_enabled(void) {
    return fb.enabled;
}

uint32_t framebuffer_width(void) {
    return fb.width;
}

uint32_t framebuffer_height(void) {
    return fb.height;
}

static void write_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb.enabled) {
        return;
    }
    if (x >= fb.width || y >= fb.height) {
        return;
    }
    uint8_t *dst = fb.base + (y * fb.pitch) + (x * fb.bytes_per_pixel);
    for (uint8_t i = 0; i < fb.bytes_per_pixel; ++i) {
        dst[i] = (uint8_t)(color >> (8 * i));
    }
}


static int glyph_pixel_on(const uint8_t *glyph, uint32_t x, uint32_t y) {
    return (glyph[x] & (uint8_t)(1u << y)) != 0;
}

int framebuffer_blit_rgb24(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                           const uint8_t *rgb24, uint32_t stride_bytes) {
    if (!fb.enabled || rgb24 == 0 || width == 0 || height == 0) {
        return 0;
    }
    if (x >= fb.width || y >= fb.height) {
        return 0;
    }

    uint32_t max_width = fb.width - x;
    uint32_t max_height = fb.height - y;
    if (width > max_width) {
        width = max_width;
    }
    if (height > max_height) {
        height = max_height;
    }
    if (stride_bytes == 0) {
        stride_bytes = width * 3;
    }
    if (stride_bytes < (width * 3)) {
        return 0;
    }

    for (uint32_t row = 0; row < height; ++row) {
        const uint8_t *src = rgb24 + (row * stride_bytes);
        for (uint32_t col = 0; col < width; ++col) {
            const uint8_t *px = src + (col * 3);
            write_pixel(x + col, y + row, pack_color(px[0], px[1], px[2]));
        }
    }
    return 1;
}

int framebuffer_blit_rgb24_scaled(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                                  const uint8_t *rgb24, uint32_t src_width, uint32_t src_height,
                                  uint32_t stride_bytes) {
    if (!fb.enabled || rgb24 == 0 || width == 0 || height == 0 || src_width == 0 || src_height == 0) {
        return 0;
    }
    if (x >= fb.width || y >= fb.height) {
        return 0;
    }

    uint32_t max_width = fb.width - x;
    uint32_t max_height = fb.height - y;
    if (width > max_width) {
        width = max_width;
    }
    if (height > max_height) {
        height = max_height;
    }
    if (stride_bytes == 0) {
        stride_bytes = src_width * 3;
    }
    if (stride_bytes < (src_width * 3)) {
        return 0;
    }

    for (uint32_t row = 0; row < height; ++row) {
        uint32_t src_row = (row * src_height) / height;
        const uint8_t *src = rgb24 + (src_row * stride_bytes);
        for (uint32_t col = 0; col < width; ++col) {
            uint32_t src_col = (col * src_width) / width;
            const uint8_t *px = src + (src_col * 3);
            write_pixel(x + col, y + row, pack_color(px[0], px[1], px[2]));
        }
    }
    return 1;
}
void framebuffer_draw_cell(uint32_t col, uint32_t row, uint16_t cell) {
    if (!fb.enabled) {
        return;
    }
    uint8_t ch = (uint8_t)(cell & 0xFF);
    uint8_t attr = (uint8_t)((cell >> 8) & 0xFF);
    uint8_t fg = attr & 0x0F;
    uint8_t bg = (attr >> 4) & 0x0F;
    if (fg > 15) {
        fg = VGA_WHITE;
    }
    if (bg > 15) {
        bg = VGA_BLACK;
    }
    if (ch < 32 || ch > 127) {
        ch = '?';
    }
    const uint8_t *glyph = &font_petme128_8x8[(ch - 32) * 8];
    uint32_t base_x = col * 8;
    uint32_t base_y = row * 8;
    uint32_t fg_color = fb.palette[fg];
    uint32_t bg_color = fb.palette[bg];
    for (uint32_t y = 0; y < 8; ++y) {
        for (uint32_t x = 0; x < 8; ++x) {
            uint32_t color = glyph_pixel_on(glyph, x, y) ? fg_color : bg_color;
            write_pixel(base_x + x, base_y + y, color);
        }
    }
}

void framebuffer_present_text_grid(const uint16_t *cells, uint32_t cols, uint32_t rows) {
    if (!fb.enabled || cells == 0 || cols == 0 || rows == 0) {
        return;
    }
    if ((cols * 8u) > fb.width) {
        cols = fb.width / 8u;
    }
    if ((rows * 8u) > fb.height) {
        rows = fb.height / 8u;
    }

    uint32_t src_width = cols * 8u;
    uint32_t src_height = rows * 8u;
    uint32_t scale_x = (src_width > 0) ? (fb.width / src_width) : 1u;
    uint32_t scale_y = (src_height > 0) ? (fb.height / src_height) : 1u;
    uint32_t scale = scale_x < scale_y ? scale_x : scale_y;
    if (scale == 0) {
        scale = 1;
    }

    uint32_t draw_width = src_width * scale;
    uint32_t draw_height = src_height * scale;
    uint32_t offset_x = (fb.width - draw_width) / 2u;
    uint32_t offset_y = (fb.height - draw_height) / 2u;

    for (uint32_t row = 0; row < rows; ++row) {
        for (uint32_t col = 0; col < cols; ++col) {
            uint16_t cell = cells[row * cols + col];
            uint8_t ch = (uint8_t)(cell & 0xFF);
            uint8_t attr = (uint8_t)((cell >> 8) & 0xFF);
            uint8_t fg = attr & 0x0F;
            uint8_t bg = (attr >> 4) & 0x0F;
            if (fg > 15) {
                fg = VGA_WHITE;
            }
            if (bg > 15) {
                bg = VGA_BLACK;
            }
            if (ch < 32 || ch > 127) {
                ch = '?';
            }
            const uint8_t *glyph = &font_petme128_8x8[(ch - 32) * 8];
            uint32_t base_x = offset_x + (col * 8u * scale);
            uint32_t base_y = offset_y + (row * 8u * scale);
            uint32_t fg_color = fb.palette[fg];
            uint32_t bg_color = fb.palette[bg];

            for (uint32_t gy = 0; gy < 8; ++gy) {
                for (uint32_t gx = 0; gx < 8; ++gx) {
                    uint32_t color = glyph_pixel_on(glyph, gx, gy) ? fg_color : bg_color;
                    uint32_t px0 = base_x + (gx * scale);
                    uint32_t py0 = base_y + (gy * scale);
                    for (uint32_t sy = 0; sy < scale; ++sy) {
                        for (uint32_t sx = 0; sx < scale; ++sx) {
                            write_pixel(px0 + sx, py0 + sy, color);
                        }
                    }
                }
            }
        }
    }
}
