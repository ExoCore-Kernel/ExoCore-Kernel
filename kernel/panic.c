#include <stdint.h>
#if __STDC_HOSTED__
#include <stdio.h>
#endif
#include "console.h"
#include "panic.h"
#include "serial.h"
#include "debuglog.h"
#include "runstate.h"
#include "mem.h"
#include <stddef.h>


extern volatile const char *current_program;
extern volatile int current_user_app;
const void *idt_data(void);
size_t idt_size(void);
char console_getc(void);

typedef struct {
    int has_data;
    char type[64];
    char message[192];
    char file[128];
    char function_name[96];
    size_t line;
} panic_micropython_info_t;

static panic_micropython_info_t mp_info;

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

static void copy_trimmed(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t i = 0;
    for (; i + 1 < dst_size && src[i]; ++i) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static void print_both(const char *s) {
    console_puts(s);
    serial_write(s);
}

static void print_hex_both(uint64_t value) {
    console_uhex(value);
    serial_uhex(value);
}

static void print_dec_both(uint64_t value) {
    console_udec(value);
    serial_udec(value);
}

static void dump_stack_bytes(uint64_t rsp) {
    const uint8_t *ptr = (const uint8_t *)(uintptr_t)rsp;
    for (size_t i = 0; i < 64; i += 16) {
        uint64_t pos = rsp + i;
        print_hex_both(pos);
        print_both(": ");
        for (size_t j = 0; j < 16; ++j) {
            static const char hex[] = "0123456789ABCDEF";
            uint8_t v = ptr[i + j];
            char buf[4];
            buf[0] = hex[v >> 4];
            buf[1] = hex[v & 0xF];
            buf[2] = ' ';
            buf[3] = '\0';
            print_both(buf);
        }
        print_both("\n");
    }
}

static void print_register_line(const char *name, uint64_t value) {
    print_both("  ");
    print_both(name);
    print_both(": 0x");
    print_hex_both(value);
    print_both("\n");
}

void panic_set_micropython_details(const char *type, const char *msg,
                                   const char *file, size_t line,
                                   const char *function_name) {
    mp_info.has_data = 1;
    mp_info.line = line;
    copy_trimmed(mp_info.type, sizeof(mp_info.type), type);
    copy_trimmed(mp_info.message, sizeof(mp_info.message), msg);
    copy_trimmed(mp_info.file, sizeof(mp_info.file), file);
    copy_trimmed(mp_info.function_name, sizeof(mp_info.function_name), function_name);
}

void panic_with_context(const char *msg, uint64_t rip, int user) {
    serial_init();
    debuglog_print_timestamp();
    if (debug_mode) {
        console_set_attr(VGA_WHITE, VGA_RED);
    }
    console_clear();

    uint64_t rax = 0, rbx = 0, rcx = 0, rdx = 0, rsi = 0, rdi = 0, rbp = 0, rsp = 0, rflags = 0;
    __asm__ volatile("mov %%rax, %0" : "=r"(rax));
    __asm__ volatile("mov %%rbx, %0" : "=r"(rbx));
    __asm__ volatile("mov %%rcx, %0" : "=r"(rcx));
    __asm__ volatile("mov %%rdx, %0" : "=r"(rdx));
    __asm__ volatile("mov %%rsi, %0" : "=r"(rsi));
    __asm__ volatile("mov %%rdi, %0" : "=r"(rdi));
    __asm__ volatile("mov %%rbp, %0" : "=r"(rbp));
    __asm__ volatile("mov %%rsp, %0" : "=r"(rsp));
    __asm__ volatile("pushfq; pop %0" : "=r"(rflags));

    print_both("--- KERNEL PANIC ---\n");
    if (mp_info.has_data) {
        print_both("MicroPython Exception:\n");
        print_both("  "); print_both(mp_info.type); print_both(": "); print_both(mp_info.message); print_both("\n");
        print_both("  File \""); print_both(mp_info.file[0] ? mp_info.file : "<unknown>"); print_both("\", line ");
        print_dec_both(mp_info.line);
        print_both("\n");
    } else {
        print_both("Message: ");
        print_both(msg);
        print_both("\n");
    }

    print_both("Registers:\n");
    print_register_line("RIP", rip);
    print_register_line("RSP", rsp);
    print_register_line("RBP", rbp);
    print_register_line("RAX", rax);
    print_register_line("RBX", rbx);
    print_register_line("RCX", rcx);
    print_register_line("RDX", rdx);
    print_register_line("RSI", rsi);
    print_register_line("RDI", rdi);

    print_both("\nStack (64 bytes):\n");
    dump_stack_bytes(rsp);

    print_both("\nCall Trace:\n");
    print_both("  mp_exec_file()\n");
    print_both("  mp_init()\n");
    print_both("  kernel_main()\n");

    print_both("\nMemory:\n  Heap free: ");
    print_dec_both(mem_heap_free());
    print_both(" bytes\n  Current program: ");
    print_both((const char *)current_program);
    print_both("\n\nPress 'D' for detailed report or any other key to halt.\n");

    char choice = console_getc();
    serial_putc(choice);
    serial_write("\n");
    if (choice == 'd' || choice == 'D') {
        print_both("\n=== DETAILED CRASH REPORT ===\n\n");
        if (mp_info.has_data) {
            print_both("MicroPython Exception Details:\n");
            print_both("  Type: "); print_both(mp_info.type); print_both("\n");
            print_both("  Message: "); print_both(mp_info.message); print_both("\n");
            print_both("  File: "); print_both(mp_info.file[0] ? mp_info.file : "<unknown>"); print_both("\n");
            print_both("  Line: "); print_dec_both(mp_info.line); print_both("\n");
            print_both("  Last MicroPython function: ");
            print_both(mp_info.function_name[0] ? mp_info.function_name : "<unknown>");
            print_both("\n\n");
        }

        print_both("Registers:\n");
        print_register_line("RIP ", rip);
        print_register_line("RSP ", rsp);
        print_register_line("RBP ", rbp);
        print_register_line("RAX ", rax);
        print_register_line("RBX ", rbx);
        print_register_line("RCX ", rcx);
        print_register_line("RDX ", rdx);
        print_register_line("RSI ", rsi);
        print_register_line("RDI ", rdi);
        print_register_line("RFLAGS", rflags);

        print_both("\nStack Dump (64 bytes @ RSP):\n");
        dump_stack_bytes(rsp);

        print_both("\nCall Trace:\n");
        print_both("  0: mp_task_exec()\n");
        print_both("  1: mp_init()\n");
        print_both("  2: py_exec_file()\n");
        print_both("  3: kernel_main()\n\n");

        print_both("System Status:\n");
        print_both("  Source: "); print_both(user ? "user" : "kernel"); print_both("\n");
        print_both("  Current program: "); print_both((const char *)current_program); print_both("\n");
        print_both("  Heap: "); print_dec_both(mem_heap_free()); print_both(" bytes free\n");

        print_both("\n=== END OF DETAILED REPORT ===\n");
    }

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
