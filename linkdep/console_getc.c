#include <stdint.h>
#include "console.h"
#include "ps2kbd.h"
#include "serial.h"

static char scancode_to_ascii(uint8_t sc) {
    switch(sc) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x39: return ' ';
        case 0x1C: return '\n';
        case 0x0E: return '\b';
        case 0x48: /* up arrow scrolls view */
            console_scroll_up();
            return 0;
        case 0x50: /* down arrow scrolls view */
            console_scroll_down();
            return 0;
        case 0x4B:
            return (char)0x80; /* history prev */
        case 0x4D:
            return (char)0x81; /* history next */
        default:   return 0;
    }
}

char console_getc(void) {
    while (1) {
        uint8_t sc = 0;
        if (ps2kbd_try_read_scancode(&sc)) {
            if (sc & 0x80) {
                continue; // ignore releases
            }
            char translated = scancode_to_ascii(sc);
            if (translated) {
                return translated;
            }
        }
        if (serial_read_ready()) {
            int ch = serial_getc();
            if (ch == '\r') {
                ch = '\n';
            }
            if (ch >= 0) {
                return (char)ch;
            }
        }
    }
}
