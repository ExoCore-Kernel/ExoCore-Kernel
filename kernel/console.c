mkdir -p kernel
cat > kernel/console.c << 'EOF'
#include <stdint.h>
#include "console.h"

static volatile char *video = (char*)0xB8000;
static uint16_t cursor = 0;

/* Clear screen and reset cursor */
void console_init(void) {
    for (uint32_t i = 0; i < 80*25; i++) {
        video[i*2]   = ' ';
        video[i*2+1] = 0x07;
    }
    cursor = 0;
}

/* Advance cursor on screen; no scrolling for simplicity */
static void advance(void) {
    cursor++;
    if (cursor >= 80*25) {
        cursor = 0;
    }
}

/* Put one character (handles newline) */
void console_putc(char c) {
    if (c == '\n') {
        cursor = (cursor/80 + 1) * 80;
        if (cursor >= 80*25) cursor = 0;
    } else {
        video[cursor*2]   = c;
        video[cursor*2+1] = 0x07;
        advance();
    }
}

/* Print a string */
void console_puts(const char *s) {
    for (; *s; ++s) console_putc(*s);
}

/* Print an unsigned decimal */
void console_udec(uint32_t v) {
    char buf[12];
    int i = 10;
    buf[11] = '\\0';
    if (v == 0) buf[i--] = '0';
    while (v) {
        buf[i--] = '0' + (v % 10);
        v /= 10;
    }
    console_puts(&buf[i+1]);
}
EOF
