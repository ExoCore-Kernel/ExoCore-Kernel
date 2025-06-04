#include "idt.h"
#include "console.h"
#include "panic.h"

extern void idt_load(idt_ptr_t *);
extern void *isr_stub_table[];

static idt_entry_t idt[256];
static irq_handler_t handlers[256];

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low  = base & 0xFFFF;
    idt[num].selector    = sel;
    idt[num].zero        = 0;
    idt[num].type_attr   = flags;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
}

void register_irq_handler(uint8_t num, irq_handler_t handler) {
    handlers[num] = handler;
}

void idt_init(void) {
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)isr_stub_table[i], 0x08, 0x8E);
        handlers[i] = 0;
    }
    idt_ptr_t idtp = { sizeof(idt) - 1, (uint32_t)idt };
    idt_load(&idtp);
}

void idt_handle_interrupt(uint32_t num, uint32_t err, uint32_t esp) {
    (void)esp;
    if (handlers[num]) {
        handlers[num](num, err, esp);
        return;
    }

    static const char *exc_msgs[32] = {
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
        console_puts("\n");
        panic("CPU fault");
    } else {
        console_puts("Unhandled IRQ ");
        console_udec(num);
        console_puts("\n");
        panic("Unhandled IRQ");
    }
}
