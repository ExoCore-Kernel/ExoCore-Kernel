#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void framebuffer_configure(uint64_t addr, uint32_t pitch, uint32_t width, uint32_t height,
                           uint8_t bpp,
                           uint8_t red_position, uint8_t red_size,
                           uint8_t green_position, uint8_t green_size,
                           uint8_t blue_position, uint8_t blue_size,
                           uint8_t reserved_position, uint8_t reserved_size);
void framebuffer_enable(int enabled);
int framebuffer_enabled(void);
void framebuffer_draw_cell(uint32_t col, uint32_t row, uint16_t cell);

#ifdef __cplusplus
}
#endif

#endif /* FRAMEBUFFER_H */
