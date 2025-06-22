#ifndef IO_H
#define IO_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void io_outb(uint16_t port, uint8_t val);
uint8_t io_inb(uint16_t port);
void io_outw(uint16_t port, uint16_t val);
uint16_t io_inw(uint16_t port);
void io_outl(uint16_t port, uint32_t val);
uint32_t io_inl(uint16_t port);
void io_wait(void);
uint64_t io_rdtsc(void);
void io_cpuid(uint32_t leaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d);

#ifdef __cplusplus
}
#endif

#endif /* IO_H */
