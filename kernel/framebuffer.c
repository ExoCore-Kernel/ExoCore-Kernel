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
                           uint8_t bpp,
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
    fb.available = (fb.bytes_per_pixel >= 2);
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
        uint8_t bits = glyph[y];
        for (uint32_t x = 0; x < 8; ++x) {
            uint32_t color = (bits & 0x01) ? fg_color : bg_color;
            write_pixel(base_x + x, base_y + y, color);
            bits >>= 1;
        }
    }
}
