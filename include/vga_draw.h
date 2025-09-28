#ifndef VGA_DRAW_H
#define VGA_DRAW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VGA_DRAW_COLS 80
#define VGA_DRAW_ROWS 25

void vga_draw_init(void);
void vga_draw_begin(void);
void vga_draw_end(void);
int  vga_draw_is_active(void);
void vga_draw_clear(char ch, uint8_t attr);
void vga_draw_set_cell(int32_t x, int32_t y, char ch, uint8_t attr);
void vga_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                   char ch, uint8_t attr);
void vga_draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                   char ch, uint8_t attr, int fill);
void vga_draw_present(void);

#ifdef __cplusplus
}
#endif

#endif /* VGA_DRAW_H */
