#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);
void serial_udec(uint32_t v);
void serial_uhex(uint64_t val);
void serial_raw_putc(char c);
void serial_raw_write(const char *s);
void serial_raw_uhex(uint64_t val);

#ifdef __cplusplus
}
#endif

#endif /* SERIAL_H */
