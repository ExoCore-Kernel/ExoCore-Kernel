#include <stdint.h>
#include "console.h"

static volatile char *video = (char*)0xB8000;
static uint16_t cursor = 0;

void console_init(void) {
    for (uint32_t i = 0; i < 80*25; i++) {
        video[i*2]   = ' ';
        video[i*2+1] = 0x07;
    }
    cursor = 0;
}

static void advance(void) {
    cursor++;
    if (cursor >= 80*25) cursor = 0;
}

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

void console_puts(const char *s) {
    for (; *s; ++s) console_putc(*s);
}

void console_udec(uint32_t v) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (v == 0) buf[i--] = '0';
    while (v) {
        buf[i--] = '0' + (v % 10);
        v /= 10;
    }
    console_puts(&buf[i+1]);
}

void console_uhex(uint64_t val) {
    char buf[17];
    int i = 15;
    const char *hex = "0123456789ABCDEF";
    buf[16] = '\0';
    if (val == 0) {
        buf[i--] = '0';
    }
    while (val) {
        buf[i--] = hex[val & 0xF];
        val >>= 4;
    }
    console_puts(&buf[i+1]);
}
