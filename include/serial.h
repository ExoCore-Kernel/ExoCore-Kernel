#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);
void serial_udec(uint32_t v);
void serial_uhex(uint64_t val);
#endif /* SERIAL_H */
