#include <stdint.h>
#include "multiboot.h"
#include "config.h"

/* I/O port access */
/* Write one byte to the given I/O port */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Read one byte from the given I/O port */
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* Initialize COM1 serial port at 0x3F8 for 115200 bps */
static void serial_init(void) {
    outb(0x3F8 + 1, 0x00);    // disable all interrupts
    outb(0x3F8 + 3, 0x80);    // enable DLAB (set baud rate divisor)
    outb(0x3F8 + 0, 0x01);    // divisor low byte (115200 bps)
    outb(0x3F8 + 1, 0x00);    // divisor high byte
    outb(0x3F8 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7);    // enable FIFO, clear them, with 14-byte threshold
    outb(0x3F8 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

/* Wait until the serial port is ready to transmit */
static int serial_is_transmit_empty(void) {
    return inb(0x3F8 + 5) & 0x20;
}

/* Send one character over serial */
static void serial_putc(char c) {
    while (!serial_is_transmit_empty());
    outb(0x3F8, c);
}

/* Send a null-terminated string over serial */
static void serial_write(const char *s) {
    for (; *s; s++) serial_putc(*s);
}

/* Send an unsigned decimal number over serial */
static void serial_udec(uint32_t v) {
    char buf[11];
    int i = 10;
    buf[i--] = '\0';
    if (!v) buf[i--] = '0';
    while (v) {
        buf[i--] = '0' + (v % 10);
        v /= 10;
    }
    serial_write(&buf[i + 1]);
}

/* Entry point called by boot.S: magic in EAX, mbi ptr in EBX */
void kernel_main(uint32_t magic, multiboot_info_t *mbi) {
    serial_init();
    serial_write("ExoCore boot: magic=0x");
    serial_udec(magic);
    serial_write(", mods_count=");
    serial_udec(mbi->mods_count);
    serial_putc('\n');

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        serial_write("ERROR: not multiboot!\n");
        for (;;) __asm__("hlt");
    }

    serial_write("Kernel start OK, loading modules...\n");

#if FEATURE_RUN_DIR
    multiboot_module_t *mods = (multiboot_module_t *)mbi->mods_addr;
    for (uint32_t i = 0; i < mbi->mods_count; i++) {
        serial_write(" Module ");
        serial_udec(i);
        serial_write(": start=0x");
        serial_udec(mods[i].mod_start);
        serial_write(" end=0x");
        serial_udec(mods[i].mod_end);
        serial_putc('\n');

        /* Invoke the module as a function */
        serial_write("  Invoking module...\n");
        void (*entry)(void) = (void (*)(void))mods[i].mod_start;
        entry();
        serial_write("  Module returned.\n");
    }
#else
    serial_write(" FEATURE_RUN_DIR disabled.\n");
#endif

    serial_write("All done, halting.\n");
    for (;;) __asm__("hlt");
}
