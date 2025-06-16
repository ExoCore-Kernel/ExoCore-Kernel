#include "idt.h"
#include "console.h"
#include "panic.h"
#include "serial.h"

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

        if (num == 14) {
            console_puts("Press any key to reboot...\n");
            serial_write("Press any key to reboot...\n");
            wait_keypress();
            reboot();
        } else if (num == 3 || num == 1) {
            return;
        } else {
            panic("Fatal exception");
        }
    } else {
        console_puts("Unhandled IRQ ");
        console_udec(num);
        console_puts("\n");
        serial_write("Unhandled IRQ ");
        serial_udec(num);
        serial_write("\n");
        panic("Unhandled IRQ");
    }
}
