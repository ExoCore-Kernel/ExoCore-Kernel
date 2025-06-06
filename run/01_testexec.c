#include <stdint.h>
#include "console.h"
#include "serial.h"

void _start(void) {
    console_puts("Test module executing\n");
    serial_write("Test module executing\n");
}
