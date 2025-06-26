#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr_t;

typedef void (*irq_handler_t)(uint32_t num, uint32_t err, uint64_t rsp);

void idt_init(void);
void register_irq_handler(uint8_t num, irq_handler_t handler);
void idt_handle_interrupt(uint32_t num, uint32_t err, uint64_t rsp);
const void *idt_data(void);
size_t idt_size(void);

#ifdef __cplusplus
}
#endif

#endif /* IDT_H */
