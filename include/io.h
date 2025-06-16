#ifndef IO_H
#define IO_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void io_outb(uint16_t port, uint8_t val);
uint8_t io_inb(uint16_t port);
void io_wait(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_H */
