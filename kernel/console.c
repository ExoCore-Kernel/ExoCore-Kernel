#include <stdint.h>
#include "console.h"
#include "debuglog.h"
#include "config.h"
#include "serial.h"
#include "io.h"
#include "mem.h"
#include "framebuffer.h"

static volatile char *video = (char*)0xB8000;
static uint8_t attr = VGA_ATTR(VGA_WHITE, VGA_BLACK);
static int vga_enabled = 1;

#define BUF_LINES 512
static uint16_t buf[BUF_LINES][80];
static uint8_t line_len[BUF_LINES];
static uint32_t head = 0;      /* index of oldest line */
static uint32_t count = 1;     /* number of lines stored (at least 1) */
static uint32_t cur_line = 0;  /* current line index */
static uint32_t cur_col = 0;
static uint32_t view = 0;      /* top line offset from head */

static uint16_t pack(char c) { return ((uint16_t)attr << 8) | (uint8_t)c; }

static uint32_t idx(uint32_t off) { return (head + off) % BUF_LINES; }

static void clear_line(uint32_t line) {
    for (int i = 0; i < 80; i++)
        buf[line][i] = pack(' ');
    line_len[line] = 0;
  
}

static void erase_prev_char(void) {
    if (cur_col == 0) {
        return;
    }
    buf[cur_line][cur_col - 1] = pack(' ');
    cur_col--;
    if (line_len[cur_line] > cur_col)
        line_len[cur_line] = cur_col;
}

static int follow_tail(void) {
    if (count <= 25) {
        return 1;
    }
    return view + 25 >= count;

}

static void draw_screen(void) {
    if (!vga_enabled) {
        return;
    }
    uint32_t start = (count > 25 && view + 25 > count) ? count - 25 : view;
    if (count <= 25) start = 0;
    for (int r = 0; r < 25; r++) {
        uint32_t off = start + r;
        if (off >= count) {
            for (int c = 0; c < 80; c++) {
                uint16_t cell = pack(' ');
                if (framebuffer_enabled()) {
                    framebuffer_draw_cell((uint32_t)c, (uint32_t)r, cell);
                } else {
                    video[(r*80+c)*2] = ' ';
                    video[(r*80+c)*2+1] = attr;
                }
            }
        } else {
            uint16_t *line = buf[idx(off)];
            for (int c = 0; c < 80; c++) {
                uint16_t v = line[c];
                if (framebuffer_enabled()) {
                    framebuffer_draw_cell((uint32_t)c, (uint32_t)r, v);
                } else {
                    video[(r*80+c)*2] = (char)v;
                    video[(r*80+c)*2+1] = v >> 8;
                }
            }
        }
    }
}

void console_init(void) {
    if (!framebuffer_enabled()) {
        mem_vram_lock("console");
        video = (char*)mem_vram_base();
    }
    for (uint32_t l = 0; l < BUF_LINES; l++)
        clear_line(l);
    head = 0;
    count = 1;
    cur_line = 0;
    cur_col = 0;
    view = 0;
    draw_screen();
#ifndef NO_DEBUGLOG
    debuglog_print_timestamp();
#endif
    console_puts("console_init complete\n");
}

static void newline(void) {
    uint32_t line = cur_line;
    uint32_t clear_from = line_len[line];
    if (clear_from > 80)
        clear_from = 80;
    for (uint32_t c = clear_from; c < 80; c++)
        buf[line][c] = pack(' ');
    line_len[line] = clear_from;
    cur_col = 0;
    cur_line = (cur_line + 1) % BUF_LINES;
    if (count < BUF_LINES) {
        count++;
    } else {
        head = (head + 1) % BUF_LINES;
        if (view > 0) view--;
    }
    clear_line(cur_line);
}

void console_putc(char c) {
    int follow = follow_tail();
    if (!vga_enabled) {
        serial_putc(c);
    }
    if (c == '\n') {
        newline();
    } else if (c == '\r') {
        cur_col = 0;
    } else if (c == '\b') {
        erase_prev_char();
    } else {
        buf[cur_line][cur_col] = pack(c);
        cur_col++;
        if (line_len[cur_line] < cur_col)
            line_len[cur_line] = cur_col;
        if (cur_col >= 80)
            newline();
    }
#ifndef NO_DEBUGLOG
    debuglog_char(c);
#endif
    if (follow && vga_enabled) {
        view = (count > 25) ? count - 25 : 0;
    }
    draw_screen();
}

void console_puts(const char *s) {
    for (; *s; ++s) console_putc(*s);
}

static int ps2_try_read_scancode(uint8_t *scancode) {
    if ((io_inb(0x64) & 1) == 0) {
        return 0;
    }
    uint8_t value = io_inb(0x60);
    if (scancode) {
        *scancode = value;
    }
    return 1;
}

static char scancode_to_ascii(uint8_t sc) {
    switch (sc) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x39: return ' ';
        case 0x1C: return '\n';
        case 0x0E: return '\b';
        case 0x48:
            console_scroll_up();
            return 0;
        case 0x50:
            console_scroll_down();
            return 0;
        case 0x4B:
            return (char)0x80;
        case 0x4D:
            return (char)0x81;
        default:
            return 0;
    }
}

char console_getc(void) {
    while (1) {
        uint8_t sc = 0;
        if (ps2_try_read_scancode(&sc)) {
            if (sc & 0x80) {
                continue;
            }
            char translated = scancode_to_ascii(sc);
            if (translated) {
                return translated;
            }
        }
        if (serial_read_ready()) {
            int ch = serial_getc();
            if (ch == '\r') {
                ch = '\n';
            }
            if (ch >= 0) {
                return (char)ch;
            }
        }
    }
}

void console_udec(uint32_t v) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (v == 0) buf[i--] = '0';
    while (v) {
        buf[i--] = '0' + (v % 10);
        v /= 10;
    }
    console_puts(&buf[i+1]);
}

void console_uhex(uint64_t val) {
    char buf[17];
    int i = 15;
    const char *hex = "0123456789ABCDEF";
    buf[16] = '\0';
    if (val == 0) {
        buf[i--] = '0';
    }
    while (val) {
        buf[i--] = hex[val & 0xF];
        val >>= 4;
    }
    console_puts(&buf[i+1]);
}

void console_set_attr(uint8_t fg, uint8_t bg) {
    attr = VGA_ATTR(fg, bg);
}

void console_backspace(void) {
    erase_prev_char();
    if (follow_tail() && vga_enabled) {
        view = (count > 25) ? count - 25 : 0;
    }
    draw_screen();
}

void console_clear(void) {
    for (uint32_t l = 0; l < BUF_LINES; l++)
        clear_line(l);
    head = 0;
    count = 1;
    cur_line = 0;
    cur_col = 0;
    view = 0;
    draw_screen();
}

void console_scroll_up(void) {
    if (vga_enabled && view > 0) {
        view--;
        draw_screen();
    }
}

void console_scroll_down(void) {
    if (vga_enabled && view + 25 < count) {
        view++;
        draw_screen();
    }
}

void console_set_vga_enabled(int enabled) {
    vga_enabled = enabled ? 1 : 0;
}

int console_vga_enabled(void) {
    return vga_enabled;
}
