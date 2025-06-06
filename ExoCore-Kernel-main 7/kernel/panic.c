#include <stdint.h>
#include "console.h"
#include "panic.h"

/* Serial I/O used for panic output */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void serial_init(void) {
    outb(0x3F8+1,0); outb(0x3F8+3,0x80);
    outb(0x3F8+0,0x01); outb(0x3F8+1,0);
    outb(0x3F8+3,0x03); outb(0x3F8+2,0xC7);
    outb(0x3F8+4,0x0B);
}
static void serial_putc(char c) {
    while (!(inb(0x3F8+5) & 0x20));
    outb(0x3F8, c);
}
static void serial_write(const char *s) {
    for (; *s; ++s) serial_putc(*s);
}

void panic(const char *msg) {
    serial_init();
    console_puts("Panic: ");
    serial_write("Panic: ");
    console_puts(msg);
    serial_write(msg);
    console_putc('\n');
    serial_write("\n");
    for (;;) __asm__("hlt");
}
