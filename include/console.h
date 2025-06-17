#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** VGA color constants */
enum {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW = 14,
    VGA_WHITE = 15,
};

#define VGA_ATTR(fg, bg) (((bg) << 4) | ((fg) & 0x0F))

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

/** Delete the character before the cursor. */
void console_backspace(void);

/** Clear the screen and reset the cursor. */
void console_clear(void);

/** Read one character from the keyboard. */
char console_getc(void);

/** Set the current foreground/background attribute. */
void console_set_attr(uint8_t fg, uint8_t bg);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_H */
