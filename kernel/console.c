#include <stdint.h>
#include "console.h"
#include "debuglog.h"
#include "config.h"

static volatile char *video = (char*)0xB8000;
static uint8_t attr = VGA_ATTR(VGA_WHITE, VGA_BLACK);

#define BUF_LINES 200
static uint16_t buf[BUF_LINES][80];
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
}

static void draw_screen(void) {
    uint32_t start = (count > 25 && view + 25 > count) ? count - 25 : view;
    if (count <= 25) start = 0;
    for (int r = 0; r < 25; r++) {
        uint32_t off = start + r;
        if (off >= count) {
            for (int c = 0; c < 80; c++) {
                video[(r*80+c)*2] = ' ';
                video[(r*80+c)*2+1] = attr;
            }
        } else {
            uint16_t *line = buf[idx(off)];
            for (int c = 0; c < 80; c++) {
                uint16_t v = line[c];
                video[(r*80+c)*2] = (char)v;
                video[(r*80+c)*2+1] = v >> 8;
            }
        }
    }
}

void console_init(void) {
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
    for (uint32_t c = cur_col; c < 80; c++)
        buf[cur_line][c] = pack(' ');
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
    if (c == '\n') {
        newline();
    } else if (c == '\r') {
        cur_col = 0;
    } else {
        buf[cur_line][cur_col] = pack(c);
        cur_col++;
        if (cur_col >= 80)
            newline();
    }
#ifndef NO_DEBUGLOG
    debuglog_char(c);
#endif
    view = (count > 25) ? count - 25 : 0;
    draw_screen();
}

void console_puts(const char *s) {
    for (; *s; ++s) console_putc(*s);
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
    if (cur_col == 0)
        return;
    cur_col--;
    buf[cur_line][cur_col] = pack(' ');
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
    if (view > 0) {
        view--;
        draw_screen();
    }
}

void console_scroll_down(void) {
    if (view + 25 < count) {
        view++;
        draw_screen();
    }
}
