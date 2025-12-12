#include "../include/console.h"
#include "../include/serial.h"

int debug_mode = 0;
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
void console_set_vga_enabled(int enabled) {(void)enabled;}
int console_vga_enabled(void) {return 0;}

void serial_init(void) {}
void serial_putc(char c) { (void)c; }
void serial_write(const char *s) { (void)s; }
void serial_udec(uint32_t v) { (void)v; }
void serial_uhex(uint64_t val) { (void)val; }
void serial_raw_putc(char c) { (void)c; }
void serial_raw_write(const char *s) { (void)s; }
void serial_raw_uhex(uint64_t val) { (void)val; }
