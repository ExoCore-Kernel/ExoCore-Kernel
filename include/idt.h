#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

typedef void (*irq_handler_t)(uint32_t num, uint32_t err, uint32_t esp);

void idt_init(void);
void register_irq_handler(uint8_t num, irq_handler_t handler);
void idt_handle_interrupt(uint32_t num, uint32_t err, uint32_t esp);

#endif /* IDT_H */
