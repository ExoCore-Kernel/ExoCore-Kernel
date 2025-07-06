#include <stdint.h>
#if __STDC_HOSTED__
#include <stdio.h>
#endif
#include "console.h"
#include "panic.h"
#include "serial.h"
#include "debuglog.h"
#include "runstate.h"
#include <stddef.h>


extern volatile const char *current_program;
extern volatile int current_user_app;
const void *idt_data(void);
size_t idt_size(void);

static void memdump_direct(const void *addr, size_t len, uint64_t rip) {
    const unsigned char *p = (const unsigned char *)addr;
    const char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < len; i += 16) {
        uint64_t pos = (uint64_t)(uintptr_t)addr + i;
        console_uhex(pos); console_puts(": ");
        serial_uhex(pos); serial_write(": ");
        size_t line_len = (len - i < 16) ? len - i : 16;
        for (size_t j = 0; j < line_len; j++) {
            unsigned char c = p[i + j];
            char hi = hex[c >> 4];
            char lo = hex[c & 0xF];
            console_putc(hi); serial_raw_putc(hi);
            console_putc(lo); serial_raw_putc(lo);
            console_putc(' '); serial_raw_putc(' ');
        }
        if (rip >= pos && rip < pos + line_len) {
            console_puts("<- RIP\n");
            serial_write("<- RIP\n");
        } else {
            console_putc('\n');
            serial_raw_putc('\n');
        }
    }
}

#if __STDC_HOSTED__
static void hexdump_file(FILE *f, const void *data, size_t len) {
    const unsigned char *p = (const unsigned char *)data;
    const char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < len; i++) {
        fputc(hex[p[i] >> 4], f);
        fputc(hex[p[i] & 0xF], f);
    }
}
#endif

void panic_with_context(const char *msg, uint64_t rip, int user) {
    serial_init();
    debuglog_print_timestamp();
    if (debug_mode) {
        console_set_attr(VGA_WHITE, VGA_RED);
        console_clear();
        console_puts("Guru Meditation: Kernel Panic: ");
        serial_write("Panic: ");
    } else {
        console_puts("Panic: ");
        serial_write("Panic: ");
    }
    console_puts(msg);
    serial_write(msg);
    console_putc('\n');
    serial_write("\n");

    debuglog_save_file();

    console_puts("Code around RIP:\n");
    serial_write("Code around RIP:\n");
    const void *dump_addr = (const void *)(rip & ~0xFUL);
    memdump_direct(dump_addr, 64, rip);

#if __STDC_HOSTED__
    FILE *f = fopen("exocorecrash.txt", "w");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "  \"message\": \"%s\",\n", msg);
        fprintf(f, "  \"rip\": \"0x%llx\",\n", (unsigned long long)rip);
        fprintf(f, "  \"source\": \"%s\",\n", user ? "user" : "kernel");
        fprintf(f, "  \"program\": \"%s\",\n", (const char *)current_program);
        fprintf(f, "  \"idt_dump\": \"");
        hexdump_file(f, idt_data(), idt_size());
        fprintf(f, "\",\n  \"mem_dump\": \"");
        const void *addr = (const void *)(rip & ~0xFUL);
        hexdump_file(f, addr, 64);
        fprintf(f, "\"\n}\n");
        fclose(f);
    }
#endif

    debuglog_flush();
    debuglog_dump_console();
    __asm__ volatile("cli");
    for (;;) __asm__("hlt");
}

void panic(const char *msg) {
    panic_with_context(msg, (uint64_t)__builtin_return_address(0), 0);
}
