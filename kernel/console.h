#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

/* Must be called once at boot. */
void console_init(void);

/* Print one character to VGA and advance cursor. */
void console_putc(char c);

/* Print a null-terminated string to VGA. */
void console_puts(const char *s);

/* Delete the character before the cursor (like backspace). */
void console_backspace(void);

/* Clear the screen and reset cursor to (0,0). */
void console_clear(void);

#endif /* CONSOLE_H */
