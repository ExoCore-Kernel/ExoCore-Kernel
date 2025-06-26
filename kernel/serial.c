#include <stdint.h>
#include "serial.h"
#include "debuglog.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void serial_init(void) {
    outb(0x3F8 + 1, 0);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x01);
    outb(0x3F8 + 1, 0);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
#ifndef NO_DEBUGLOG
    debuglog_print_timestamp();
#endif
    serial_write("serial_init complete\n");
}

void serial_putc(char c) {
    while (!(inb(0x3F8 + 5) & 0x20)) {}
    outb(0x3F8, c);
#ifndef NO_DEBUGLOG
    debuglog_char(c);
#endif
}

void serial_write(const char *s) {
    for (; *s; ++s) serial_putc(*s);
}

void serial_udec(uint32_t v) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (v == 0) buf[i--] = '0';
    while (v) {
        buf[i--] = '0' + (v % 10);
        v /= 10;
    }
    serial_write(&buf[i + 1]);
}

void serial_uhex(uint64_t val) {
    char buf[17];
    int i = 15;
    const char *hex = "0123456789ABCDEF";
    buf[16] = '\0';
    if (val == 0) buf[i--] = '0';
    while (val) {
        buf[i--] = hex[val & 0xF];
        val >>= 4;
    }
    serial_write(&buf[i + 1]);
}
