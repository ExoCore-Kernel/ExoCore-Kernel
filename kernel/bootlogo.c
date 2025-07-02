#include "bootlogo.h"
#include "bootlogo_bmp.h"
#include "console.h"
#include "config.h"

#if FEATURE_BOOT_LOGO
static int logo_width;
static int logo_height;
static const unsigned char *logo_pixels;
static int logo_row_bytes;

struct rgb { unsigned char r, g, b; };
static const struct rgb vga_palette[16] = {
    {0,0,0},         {0,0,170},     {0,170,0},     {0,170,170},
    {170,0,0},       {170,0,170},   {170,85,0},    {170,170,170},
    {85,85,85},      {85,85,255},   {85,255,85},   {85,255,255},
    {255,85,85},     {255,85,255},  {255,255,85},  {255,255,255}
};

static int rgb_to_vga(unsigned char r, unsigned char g, unsigned char b) {
    int best = 0x7FFFFFFF;
    int best_idx = 0;
    for (int i = 0; i < 16; i++) {
        int dr = (int)r - vga_palette[i].r;
        int dg = (int)g - vga_palette[i].g;
        int db = (int)b - vga_palette[i].b;
        int dist = dr*dr + dg*dg + db*db;
        if (dist < best) { best = dist; best_idx = i; }
    }
    return best_idx;
}

void bootlogo_init(void) {
    if (assets_bootlogo_bmp_len < 54)
        return;
    const unsigned char *data = assets_bootlogo_bmp;
    uint32_t offset = *(const uint32_t*)&data[10];
    logo_width  = *(const int32_t*)&data[18];
    logo_height = *(const int32_t*)&data[22];
    uint16_t bpp = *(const uint16_t*)&data[28];
    if (bpp != 24 || logo_width <= 0 || logo_height <= 0)
        return;
    logo_pixels = data + offset;
    logo_row_bytes = (logo_width * 3 + 3) & ~3;
}

static void put_pixel_color(int x, int y, int color) {
    if (x < 0 || x >= 80 || y < 0 || y >= 25)
        return;
    volatile unsigned char *vga = (unsigned char*)0xB8000;
    int pos = (y * 80 + x) * 2;
    vga[pos] = 0xDB; /* full block character */
    vga[pos + 1] = VGA_ATTR(color, color);
}

void bootlogo_render(void) {
    if (!logo_pixels)
        return;
    for (int y = 0; y < logo_height && BOOTLOGO_POS_Y + y < 25; y++) {
        const unsigned char *row = logo_pixels + (logo_height - 1 - y) * logo_row_bytes;
        for (int x = 0; x < logo_width && BOOTLOGO_POS_X + x < 80; x++) {
            int b = row[x*3];
            int g = row[x*3 + 1];
            int r = row[x*3 + 2];
            int color = rgb_to_vga(r, g, b);
            put_pixel_color(BOOTLOGO_POS_X + x, BOOTLOGO_POS_Y + y, color);
        }
    }
}
#else
void bootlogo_init(void) {}
void bootlogo_render(void) {}
#endif
