#ifndef PS2KBD_H
#define PS2KBD_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t ps2kbd_read_scancode(void);

#ifdef __cplusplus
}
#endif

#endif /* PS2KBD_H */
