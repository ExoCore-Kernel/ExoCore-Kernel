#include <stdint.h>
#include "console.h"
#include "debuglog.h"
#include "config.h"
#include "serial.h"
#include "io.h"
#include "mem.h"
#include "framebuffer.h"
#include "bootmode.h"

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

static int console_display_enabled(void) {
    return bootmode_logs_visible() && (vga_enabled || framebuffer_enabled());
}

static uint32_t visible_rows(void) {
    if (framebuffer_enabled()) {
        uint32_t rows = framebuffer_text_rows();
        if (rows > BUF_LINES) {
            rows = BUF_LINES;
        }
        return rows ? rows : 1u;
    }
    return 25u;
}

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
    uint32_t rows = visible_rows();
    if (count <= rows) {
        return 1;
    }
    return view + rows >= count;

}

static void draw_screen(void) {
    if (!console_display_enabled()) {
        return;
    }
    uint32_t rows = visible_rows();
    uint32_t start = (count > rows && view + rows > count) ? count - rows : view;
    if (count <= rows) start = 0;
    for (uint32_t r = 0; r < rows; r++) {
        uint32_t off = start + r;
        if (off >= count) {
            for (uint32_t c = 0; c < 80; c++) {
                uint16_t cell = pack(' ');
                if (framebuffer_enabled()) {
                    framebuffer_draw_cell(c, r, cell);
                } else {
                    video[(r*80+c)*2] = ' ';
                    video[(r*80+c)*2+1] = attr;
                }
            }
        } else {
            uint16_t *line = buf[idx(off)];
            for (uint32_t c = 0; c < 80; c++) {
                uint16_t v = line[c];
                if (framebuffer_enabled()) {
                    framebuffer_draw_cell(c, r, v);
                } else {
                    video[(r*80+c)*2] = (char)v;
                    video[(r*80+c)*2+1] = v >> 8;
                }
            }
        }
    }
}

void console_init(void) {
    console_apply_boot_theme();
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

static void console_emit_char(char c) {
    int follow = follow_tail();
    /* Mirror interactive console output to COM1 so QEMU -serial stdio and
     * headless compatibility shells show prompts, echo, and command output.
     * Use the raw serial writer to avoid feeding mirrored bytes back into the
     * debug log a second time.
     */
    serial_raw_putc(c);
    if (c == '\n') {
        newline();
    } else if (c == '\r') {
        cur_col = 0;
    } else if (c == '\b') {
        erase_prev_char();
    } else {
        if (cur_col >= 80) {
            newline();
        }
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
    if (follow && console_display_enabled()) {
        uint32_t rows = visible_rows();
        view = (count > rows) ? count - rows : 0;
    }
    if (!follow) {
        draw_screen();
    }
}

void console_flush(void) {
    draw_screen();
}

void console_putc(char c) {
    console_emit_char(c);
    console_flush();
}

void console_write(const char *s, size_t len) {
    if (!s || len == 0) {
        return;
    }
    for (size_t i = 0; i < len; ++i) {
        console_emit_char(s[i]);
    }
    console_flush();
}

void console_puts(const char *s) {
    if (!s) {
        return;
    }
    const char *p = s;
    while (*p) {
        ++p;
    }
    console_write(s, (size_t)(p - s));
}

static int ps2_try_read_scancode(uint8_t *scancode) {
    uint8_t status = io_inb(0x64);
    if ((status & 1) == 0) {
        return 0;
    }
    if (status & 0x20) {
        (void)io_inb(0x60);
        return 0;
    }
    uint8_t value = io_inb(0x60);
    if (scancode) {
        *scancode = value;
    }
    return 1;
}

static int kbd_shift = 0;
static int kbd_extended = 0;
static char pending_input[3];
static int pending_len = 0;
static int pending_pos = 0;

static char queue_escape_sequence(char final) {
    pending_input[0] = 27;
    pending_input[1] = '[';
    pending_input[2] = final;
    pending_len = 3;
    pending_pos = 1;
    return pending_input[0];
}

static char scancode_to_ascii(uint8_t sc) {
    if (sc == 0xE0) {
        kbd_extended = 1;
        return 0;
    }
    int extended = kbd_extended;
    kbd_extended = 0;
    if (sc & 0x80) {
        uint8_t released = sc & 0x7F;
        if (released == 0x2A || released == 0x36) {
            kbd_shift = 0;
        }
        return 0;
    }
    if (sc == 0x2A || sc == 0x36) {
        kbd_shift = 1;
        return 0;
    }
    char normal = 0;
    char shifted = 0;
    switch (sc) {
        case 0x01: normal = 27; shifted = 27; break;
        case 0x02: normal = '1'; shifted = '!'; break;
        case 0x03: normal = '2'; shifted = '@'; break;
        case 0x04: normal = '3'; shifted = '#'; break;
        case 0x05: normal = '4'; shifted = '$'; break;
        case 0x06: normal = '5'; shifted = '%'; break;
        case 0x07: normal = '6'; shifted = '^'; break;
        case 0x08: normal = '7'; shifted = '&'; break;
        case 0x09: normal = '8'; shifted = '*'; break;
        case 0x0A: normal = '9'; shifted = '('; break;
        case 0x0B: normal = '0'; shifted = ')'; break;
        case 0x0C: normal = '-'; shifted = '_'; break;
        case 0x0D: normal = '='; shifted = '+'; break;
        case 0x0E: normal = '\b'; shifted = '\b'; break;
        case 0x0F: normal = '\t'; shifted = '\t'; break;
        case 0x10: normal = 'q'; shifted = 'Q'; break;
        case 0x11: normal = 'w'; shifted = 'W'; break;
        case 0x12: normal = 'e'; shifted = 'E'; break;
        case 0x13: normal = 'r'; shifted = 'R'; break;
        case 0x14: normal = 't'; shifted = 'T'; break;
        case 0x15: normal = 'y'; shifted = 'Y'; break;
        case 0x16: normal = 'u'; shifted = 'U'; break;
        case 0x17: normal = 'i'; shifted = 'I'; break;
        case 0x18: normal = 'o'; shifted = 'O'; break;
        case 0x19: normal = 'p'; shifted = 'P'; break;
        case 0x1A: normal = '['; shifted = '{'; break;
        case 0x1B: normal = ']'; shifted = '}'; break;
        case 0x1C: normal = '\n'; shifted = '\n'; break;
        case 0x1E: normal = 'a'; shifted = 'A'; break;
        case 0x1F: normal = 's'; shifted = 'S'; break;
        case 0x20: normal = 'd'; shifted = 'D'; break;
        case 0x21: normal = 'f'; shifted = 'F'; break;
        case 0x22: normal = 'g'; shifted = 'G'; break;
        case 0x23: normal = 'h'; shifted = 'H'; break;
        case 0x24: normal = 'j'; shifted = 'J'; break;
        case 0x25: normal = 'k'; shifted = 'K'; break;
        case 0x26: normal = 'l'; shifted = 'L'; break;
        case 0x27: normal = ';'; shifted = ':'; break;
        case 0x28: normal = '\''; shifted = '"'; break;
        case 0x29: normal = '`'; shifted = '~'; break;
        case 0x2B: normal = '\\'; shifted = '|'; break;
        case 0x2C: normal = 'z'; shifted = 'Z'; break;
        case 0x2D: normal = 'x'; shifted = 'X'; break;
        case 0x2E: normal = 'c'; shifted = 'C'; break;
        case 0x2F: normal = 'v'; shifted = 'V'; break;
        case 0x30: normal = 'b'; shifted = 'B'; break;
        case 0x31: normal = 'n'; shifted = 'N'; break;
        case 0x32: normal = 'm'; shifted = 'M'; break;
        case 0x33: normal = ','; shifted = '<'; break;
        case 0x34: normal = '.'; shifted = '>'; break;
        case 0x35: normal = '/'; shifted = '?'; break;
        case 0x39: normal = ' '; shifted = ' '; break;
        case 0x48: return extended ? queue_escape_sequence('A') : 0;
        case 0x50: return extended ? queue_escape_sequence('B') : 0;
        case 0x4B: return extended ? queue_escape_sequence('D') : 0;
        case 0x4D: return extended ? queue_escape_sequence('C') : 0;
        default: return 0;
    }
    return kbd_shift ? shifted : normal;
}

char console_getc(void) {
    while (1) {
        if (pending_pos < pending_len) {
            return pending_input[pending_pos++];
        }
        pending_len = 0;
        pending_pos = 0;
        if (serial_read_ready()) {
            int ch = serial_getc();
            if (ch == '\r') {
                ch = '\n';
            }
            if (ch >= 0) {
                return (char)ch;
            }
        }
        uint8_t sc = 0;
        if (ps2_try_read_scancode(&sc)) {
            char translated = scancode_to_ascii(sc);
            if (translated) {
                return translated;
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
    if (follow_tail() && console_display_enabled()) {
        uint32_t rows = visible_rows();
        view = (count > rows) ? count - rows : 0;
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
    if (console_display_enabled() && view > 0) {
        view--;
        draw_screen();
    }
}

void console_scroll_down(void) {
    if (console_display_enabled() && view + visible_rows() < count) {
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

void console_set_logs_visible(int visible) { bootmode_set_logs_visible(visible); if (visible) draw_screen(); }
int console_logs_visible(void) { return bootmode_logs_visible(); }
void console_apply_boot_theme(void) { attr = VGA_ATTR(bootmode_fg(), bootmode_bg()); }
