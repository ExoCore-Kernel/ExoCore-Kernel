#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void framebuffer_configure(uint64_t addr, uint32_t pitch, uint32_t width, uint32_t height,
                           uint8_t bpp, uint8_t type,
                           uint8_t red_position, uint8_t red_size,
                           uint8_t green_position, uint8_t green_size,
                           uint8_t blue_position, uint8_t blue_size,
                           uint8_t reserved_position, uint8_t reserved_size);
void framebuffer_enable(int enabled);
int framebuffer_enabled(void);
uint32_t framebuffer_width(void);
uint32_t framebuffer_height(void);
void framebuffer_draw_cell(uint32_t col, uint32_t row, uint16_t cell);
int framebuffer_blit_rgb24(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                           const uint8_t *rgb24, uint32_t stride_bytes);
int framebuffer_blit_rgb24_scaled(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                                  const uint8_t *rgb24, uint32_t src_width, uint32_t src_height,
                                  uint32_t stride_bytes);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_H */
