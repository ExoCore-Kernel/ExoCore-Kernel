#include <stdint.h>
#include "io.h"

#define KBD_DATA   0x60
#define KBD_STATUS 0x64

static int kbd_has_data(void) {
    return (io_inb(KBD_STATUS) & 1) != 0;
}

int ps2kbd_try_read_scancode(uint8_t *scancode) {
    if (!kbd_has_data()) {
        return 0;
    }
    uint8_t value = io_inb(KBD_DATA);
    if (scancode) {
        *scancode = value;
    }
    return 1;
}

uint8_t ps2kbd_read_scancode(void) {
    uint8_t scancode = 0;
    while (!ps2kbd_try_read_scancode(&scancode)) {}
    return scancode;
}
