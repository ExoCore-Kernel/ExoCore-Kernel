#include <stdint.h>
#include "serial.h"

static inline void io_outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t io_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void wait_transmit(void) {
    while (!(io_inb(0x3F8 + 5) & 0x20));
}

void serial_init(void) {
    io_outb(0x3F8 + 1, 0);
    io_outb(0x3F8 + 3, 0x80);
    io_outb(0x3F8 + 0, 0x01);
    io_outb(0x3F8 + 1, 0);
    io_outb(0x3F8 + 3, 0x03);
    io_outb(0x3F8 + 2, 0xC7);
    io_outb(0x3F8 + 4, 0x0B);
}

void serial_putc(char c) {
    wait_transmit();
    io_outb(0x3F8, c);
}

void serial_write(const char *s) {
    for (; *s; ++s) serial_putc(*s);
}
