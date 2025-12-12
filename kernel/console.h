#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>

/* VGA color constants */
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

/* Must be called once at boot. */
void console_init(void);

/* Print one character to VGA and advance cursor. */
void console_putc(char c);

/* Print a null-terminated string to VGA. */
void console_puts(const char *s);

/* Print an unsigned decimal number. */
void console_udec(uint32_t v);

/* Print an unsigned hexadecimal number. */
void console_uhex(uint64_t val);

/* Delete the character before the cursor (like backspace). */
void console_backspace(void);

/* Clear the screen and reset cursor to (0,0). */
void console_clear(void);

/* Set the current foreground/background attribute. */
void console_set_attr(uint8_t fg, uint8_t bg);

/* Scroll the visible console up by one line if possible */
void console_scroll_up(void);

/* Scroll the visible console down by one line if possible */
void console_scroll_down(void);

/* Enable or disable VGA-backed console rendering. */
void console_set_vga_enabled(int enabled);

/* Query whether the VGA text console is currently active. */
int console_vga_enabled(void);

#endif /* CONSOLE_H */
