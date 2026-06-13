#include "console.h"
#include "serial.h"
#include "py/mphal.h"
#include "runstate.h"

mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    serial_write(str);
    if (!mp_vga_output) return len;
    for (size_t i = 0; i < len; i++) {
        console_putc(str[i]);
    }
    return len;
}

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    serial_write(str);
    if (!mp_vga_output) return;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == '\n') console_putc('\r');
        console_putc(c);
    }
}

mp_uint_t mp_hal_stderr_tx_strn(const char *str, size_t len) {
    serial_write(str);
    if (!mp_vga_output) return len;
    for (size_t i = 0; i < len; i++) {
        console_putc(str[i]);
    }
    return len;
}
