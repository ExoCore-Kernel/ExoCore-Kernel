#ifndef PS2KBD_H
#define PS2KBD_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t ps2kbd_read_scancode(void);
int ps2kbd_try_read_scancode(uint8_t *scancode);

#ifdef __cplusplus
}
#endif

#endif /* PS2KBD_H */
