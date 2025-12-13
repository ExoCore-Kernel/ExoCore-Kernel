#include "idt.h"
#include "console.h"
#include "panic.h"
#include "serial.h"
#include "runstate.h"
#include <stddef.h>
#include <string.h>

extern void idt_load(idt_ptr_t *);
extern void *isr_stub_table[];

static idt_entry_t idt[256];
static irq_handler_t handlers[256];

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline uint64_t read_cr2(void) {
    uint64_t val;
    __asm__ volatile("mov %%cr2, %0" : "=r"(val));
    return val;
}

static void set_code_literal(char *dst, size_t size, const char *text) {
    if (!dst || size == 0 || !text) {
        return;
    }
    size_t i = 0;
    while (i + 1 < size && text[i]) {
        dst[i] = text[i];
        ++i;
    }
    dst[i] = '\0';
}

static void append_hex(char *dst, size_t size, uint64_t value) {
    static const char hex[] = "0123456789ABCDEF";
    size_t len = strlen(dst);
    if (len + 3 >= size) {
        return;
    }
    dst[len++] = '0';
    dst[len++] = 'x';
    int started = 0;
    for (int shift = 60; shift >= 0 && len + 1 < size; shift -= 4) {
        char digit = hex[(value >> shift) & 0xF];
        if (!started && digit == '0' && shift > 0) {
            continue;
        }
        started = 1;
        dst[len++] = digit;
    }
    if (!started && len + 1 < size) {
        dst[len++] = '0';
    }
    dst[len] = '\0';
}

static void set_exception_error_code(uint32_t num, uint32_t err, uint64_t rip) {
    char buffer[64];
    buffer[0] = '\0';

    switch (num) {
        case 14:
            set_code_literal(buffer, sizeof(buffer), "PAGE_FAULT@");
            append_hex(buffer, sizeof(buffer), read_cr2());
            break;
        case 13:
            set_code_literal(buffer, sizeof(buffer), "GPROT_FAULT@");
            append_hex(buffer, sizeof(buffer), err);
            break;
        case 8:
            set_code_literal(buffer, sizeof(buffer), "DOUBLE_FAULT");
            break;
        case 12:
            set_code_literal(buffer, sizeof(buffer), "STACK_FAULT");
            break;
        case 6:
            set_code_literal(buffer, sizeof(buffer), "INVALID_OPCODE");
            break;
        case 0:
            set_code_literal(buffer, sizeof(buffer), "DIVIDE_BY_ZERO");
            break;
        default:
            set_code_literal(buffer, sizeof(buffer), "CPU_EXCEPTION@");
            append_hex(buffer, sizeof(buffer), rip);
            break;
    }

    panic_set_error_code(buffer);
}

static void wait_keypress(void) {
    while (!(inb(0x64) & 1)) {}
    (void)inb(0x60);
}

static void reboot(void) {
    outb(0x64, 0xFE);
    for (;;) __asm__("hlt");
}

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel,
                         uint8_t flags, uint8_t ist) {
    idt[num].offset_low  = base & 0xFFFF;
    idt[num].selector    = sel;
    idt[num].ist         = ist;
    idt[num].type_attr   = flags;
    idt[num].offset_mid  = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].zero        = 0;
}

void idt_set_user_gate(uint8_t num, void *base) {
    idt_set_gate(num, (uint64_t)(uintptr_t)base, 0x08, 0xEE, 0);
}

void register_irq_handler(uint8_t num, irq_handler_t handler) {
    handlers[num] = handler;
}

void idt_init(void) {
    for (int i = 0; i < 256; i++) {
        uint8_t ist = (i == 8) ? 1 : 0; /* use IST1 for double fault */
        idt_set_gate(i, (uint64_t)(uintptr_t)isr_stub_table[i], 0x08, 0x8E, ist);
        handlers[i] = 0;
    }
    idt_ptr_t idtp = { sizeof(idt) - 1, (uint64_t)idt };
    idt_load(&idtp);
}

void idt_handle_interrupt(uint32_t num, uint32_t err, uint64_t rsp) {
    uint64_t *stack = (uint64_t*)rsp;
    uint64_t rip = stack[17];
    if (handlers[num]) {
        handlers[num](num, err, rsp);
        return;
    }

    static const char *const exc_msgs[32] = {
        "Divide by zero",
        "Debug",
        "Non-maskable interrupt",
        "Breakpoint",
        "Overflow",
        "Bound range exceeded",
        "Invalid opcode",
        "Device not available",
        "Double fault",
        "Coprocessor segment overrun",
        "Invalid TSS",
        "Segment not present",
        "Stack segment fault",
        "General protection fault",
        "Page fault",
        "Reserved",
        "Floating point error",
        "Alignment check",
        "Machine check",
        "SIMD floating point",
        "Virtualization",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved"
    };

    if (num < 32) {
        console_puts("EXCEPTION: ");
        console_puts(exc_msgs[num]);
        console_puts(" at 0x");
        console_uhex(rip);
        console_puts(" err=0x");
        console_uhex(err);
        console_puts("\n");

        serial_write("EXCEPTION: ");
        serial_write(exc_msgs[num]);
        serial_write(" at 0x");
        serial_uhex(rip);
        serial_write(" err=0x");
        serial_uhex(err);
        serial_write("\n");

        if (num == 3 || num == 1) {
            return;
        } else {
            const char *type = current_user_app ? "User app" : "Kernel process";
            console_puts(type);
            console_puts(" crashed: ");
            console_puts((const char *)current_program);
            console_putc('\n');
            serial_write(type);
            serial_write(" crashed: ");
            serial_write((const char *)current_program);
            serial_write("\n");
            set_exception_error_code(num, err, rip);
            panic_with_context("Fatal exception", rip, current_user_app);
        }
    } else {
        console_puts("Unhandled IRQ ");
        console_udec(num);
        console_puts("\n");
        serial_write("Unhandled IRQ ");
        serial_udec(num);
        serial_write("\n");
        char buffer[32];
        set_code_literal(buffer, sizeof(buffer), "IRQ@");
        append_hex(buffer, sizeof(buffer), num);
        panic_set_error_code(buffer);
        panic_with_context("Unhandled IRQ", rip, 0);
    }
}

const void *idt_data(void) { return idt; }
size_t idt_size(void) { return sizeof(idt); }
