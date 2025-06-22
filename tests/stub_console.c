#include "../include/console.h"
void console_init(void) {}
void console_putc(char c) { (void)c; }
void console_puts(const char *s) { (void)s; }
void console_udec(uint32_t v) { (void)v; }
void console_uhex(uint64_t val) { (void)val; }
void console_backspace(void) {}
void console_clear(void) {}
char console_getc(void) { return 0; }
void console_set_attr(uint8_t fg, uint8_t bg) { (void)fg; (void)bg; }
void console_scroll_up(void) {}
void console_scroll_down(void) {}
