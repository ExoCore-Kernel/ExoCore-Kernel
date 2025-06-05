#include <stdint.h>
#include "io.h"

#define KBD_DATA   0x60
#define KBD_STATUS 0x64

static void kbd_wait_read(void) {
    while (!(io_inb(KBD_STATUS) & 1)) {}
}

uint8_t ps2kbd_read_scancode(void) {
    kbd_wait_read();
    return io_inb(KBD_DATA);
}
