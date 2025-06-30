#include "debuglog.h"
#include "mem.h"
#include "memutils.h"
#include "fs.h"
#include "console.h"
#include "serial.h"
#include "io.h"
#if __STDC_HOSTED__
#include <stdio.h>
#endif

static char *log_buf;
static size_t log_pos;
static size_t log_size;
static uint64_t log_start;

void debuglog_init(void) {
    log_size = 64 * 1024;
    log_buf = mem_alloc(log_size);
    if (!log_buf)
        log_size = 0;
    log_pos = 0;
    fs_mount(log_buf, log_size);
    log_start = io_rdtsc();
}

void debuglog_char(char c) {
    if (!log_buf || log_pos >= log_size)
        return;
    log_buf[log_pos++] = c;
}

const char *debuglog_buffer(void) { return log_buf; }
size_t debuglog_length(void) { return log_pos; }

#if __STDC_HOSTED__
static void json_escape(FILE *f, const char *s, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (c == '\n') fputs("\\n", f);
        else if (c == '"') fputs("\\\"", f);
        else if (c == '\\') fputs("\\\\", f);
        else fputc(c, f);
    }
}
#endif

void debuglog_save_file(void) {
#if __STDC_HOSTED__
    FILE *f = fopen("exocoredebug.txt", "w");
    if (f) {
        fprintf(f, "{\n  \"events\": [\n");
        size_t start = 0;
        int first = 1;
        for (size_t i = 0; i <= log_pos; i++) {
            if (i == log_pos || log_buf[i] == '\n') {
                size_t len = i - start;
                if (len) {
                    const char *line = log_buf + start;
                    const char *ts = 0;
                    const char *msg = line;
                    size_t ts_len = 0;
                    if (len > 6 && line[0] == '[' && !strncmp(line, "[ts=0x", 6)) {
                        ts = line + 6;
                        const char *end = NULL;
                        for (size_t off = 0; off < len - 6; off++) {
                            if (ts[off] == ']') { end = ts + off; break; }
                        }
                        if (end) {
                            ts_len = end - ts;
                            if (end + 2 <= line + len && end[1] == ' ')
                                msg = end + 2;
                            else
                                msg = end + 1;
                            len = (line + len) - msg;
                        } else {
                            ts = 0;
                            msg = line;
                            ts_len = 0;
                        }
                    }
                    fprintf(f, first ? "    { " : ",\n    { ");
                    first = 0;
                    if (ts) {
                        fprintf(f, "\"timestamp\": \"0x");
                        json_escape(f, ts, ts_len);
                        fprintf(f, "\", ");
                    }
                    fprintf(f, "\"message\": \"");
                    json_escape(f, msg, len);
                    fprintf(f, "\" }");
                }
                start = i + 1;
            }
        }
        fprintf(f, "\n  ]\n}\n");
        fclose(f);
    }
#else
    if (log_buf)
        fs_write(0, log_buf, log_pos);
#endif
}

void debuglog_flush(void) {
    if (!log_buf) return;
    fs_write(0, log_buf, log_pos);
}

void debuglog_dump_console(void) {
    if (!log_buf) return;
    for (size_t i = 0; i < log_pos; i++) {
        console_putc(log_buf[i]);
        serial_raw_putc(log_buf[i]);
    }
}

static const char hex_digits[] = "0123456789ABCDEF";

void debuglog_hexdump(const void *data, size_t len) {
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < len; i++) {
        char hi = hex_digits[p[i] >> 4];
        char lo = hex_digits[p[i] & 0xF];
        console_putc(hi); serial_raw_putc(hi);
        console_putc(lo); serial_raw_putc(lo);
        console_putc(' '); serial_raw_putc(' ');
        if ((i & 15) == 15)
            { console_putc('\n'); serial_raw_putc('\n'); }
    }
    if (len & 15)
        { console_putc('\n'); serial_raw_putc('\n'); }
}

void debuglog_memdump(const void *addr, size_t len) {
    const unsigned char *p = (const unsigned char *)addr;
    for (size_t i = 0; i < len; i += 16) {
        uint64_t pos = (uint64_t)((uintptr_t)addr + i);
        console_uhex(pos);
        serial_raw_uhex(pos);
        console_puts(": "); serial_raw_write(": ");
        size_t line_len = (len - i < 16) ? len - i : 16;
        debuglog_hexdump(p + i, line_len);
    }
}

void debuglog_print_timestamp(void) {
    uint64_t delta = io_rdtsc() - log_start;
    console_puts("["); serial_raw_write("[");
    console_puts("ts=0x"); serial_raw_write("ts=0x");
    console_uhex(delta); serial_raw_uhex(delta);
    console_puts("] "); serial_raw_write("] ");
}
