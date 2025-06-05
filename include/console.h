#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

/** Initialize the VGA text console (clears screen). */
void console_init(void);

/** Output a single character to the console. */
void console_putc(char c);

/** Output a null-terminated string. */
void console_puts(const char *s);

/** Output an unsigned decimal number. */
void console_udec(uint32_t v);

/** Output an unsigned hexadecimal number. */
void console_uhex(uint64_t val);

#endif /* CONSOLE_H */
