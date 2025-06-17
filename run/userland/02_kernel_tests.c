#include <stdint.h>
#include "console.h"
#include "serial.h"
#include "mem.h"
#include "memutils.h"
#include "io.h"
#include "configs/vnum.h"

/* pull in kernel implementations for standalone testing */
#include "../../kernel/mem.c"
#include "../../kernel/memutils.c"

extern uint32_t example_compute(uint32_t x, uint32_t y);

static int pass = 0;
static int fail = 0;

static void result(const char *name, int ok) {
    if (ok) {
        console_puts("[ ok ] ");
        serial_write("[ ok ] ");
        pass++;
    } else {
        console_puts("[fail] ");
        serial_write("[fail] ");
        fail++;
    }
    console_puts(name);
    console_putc('\n');
    serial_write(name);
    serial_putc('\n');
}

static void set_color(void) {
    volatile uint16_t *video = (uint16_t*)0xB8000;
    for (int i = 0; i < 80*25; i++) {
        video[i] = (0x1F << 8) | ' ';
    }
}

static void test_pointer_size(void) {
    result("64-bit pointer", sizeof(void*) == 8);
}

static void test_example_compute(void) {
    result("example_compute", example_compute(5, 7) == 12);
}

static int app_id;
static void test_mem_register(void) {
    app_id = mem_register_app(1);
    result("mem_register_app", app_id > 0);
}

static void *mem_a;
static void test_mem_alloc_app(void) {
    mem_a = mem_alloc_app(app_id, 512);
    result("mem_alloc_app", mem_a != NULL);
}

static void *mem_b;
static void test_mem_alloc(void) {
    mem_b = mem_alloc(256);
    result("mem_alloc", mem_b != NULL);
}

static void test_mem_used(void) {
    size_t used = mem_app_used(app_id);
    result("mem_app_used", used >= 512);
}

static void test_memcpy(void) {
    char src[4] = {'t','e','s','t'};
    char dst[4];
    memcpy(dst, src, 4);
    int ok = 1;
    for (int i = 0; i < 4; i++) if (dst[i] != src[i]) ok = 0;
    result("memcpy", ok);
}

static void test_memset(void) {
    char buf[4];
    memset(buf, 0xAA, 4);
    int ok = 1;
    for (int i = 0; i < 4; i++) if (buf[i] != (char)0xAA) ok = 0;
    result("memset", ok);
}

static void test_strncmp(void) {
    result("strncmp", strncmp("abc", "abc", 3) == 0);
}

static void test_console_putc(void) {
    console_putc('X');
    result("console_putc", 1);
}

static void test_console_puts(void) {
    console_puts("\nconsole_puts works\n");
    result("console_puts", 1);
}

static void test_console_udec(void) {
    console_puts("console_udec: ");
    console_udec(1234);
    console_putc('\n');
    result("console_udec", 1);
}

static void test_console_uhex(void) {
    console_puts("console_uhex: ");
    console_uhex(0x1A2B3C4D);
    console_putc('\n');
    result("console_uhex", 1);
}

static void test_serial_write(void) {
    serial_write("serial_write working\n");
    result("serial_write", 1);
}

static void test_serial_udec(void) {
    serial_write("serial_udec: ");
    serial_udec(5678);
    serial_putc('\n');
    result("serial_udec", 1);
}

static void test_serial_uhex(void) {
    serial_write("serial_uhex: ");
    serial_uhex(0xABCDEF);
    serial_putc('\n');
    result("serial_uhex", 1);
}

static void test_io_ports(void) {
    io_outb(0x80, 0x55);
    io_wait();
    (void)io_inb(0x80);
    result("io ports", 1);
}


void _start() {
    set_color();
    console_puts("Welcome to ExoCore-OS v" OS_VERSION "! Powered by ExoCore-Kernel v" KERNEL_VERSION "!\n");
    serial_write("Starting kernel tests\n");

    test_pointer_size();
    test_example_compute();
    test_mem_register();
    test_mem_alloc_app();
    test_mem_alloc();
    test_mem_used();
    test_memcpy();
    test_memset();
    test_strncmp();
    test_console_putc();
    test_console_puts();
    test_console_udec();
    test_console_uhex();
    test_serial_write();
    test_serial_udec();
    test_serial_uhex();
    test_io_ports();

    console_puts("Tests passed: ");
    console_udec(pass);
    console_puts(" failed: ");
    console_udec(fail);
    console_putc('\n');
    serial_write("Tests complete\n");

    for (;;) { __asm__("hlt"); }
}
