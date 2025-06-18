#include "script.h"
#include "console.h"
#include <stddef.h>

static int str_equal(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == '\0' && *b == '\0';
}

static int to_int(const char *s) {
    int neg = 0; int v = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
    }
    return neg ? -v : v;
}

void run_script(const char *code, size_t size) {
    int stack[32];
    int sp = 0;
    char tok[32];
    int tlen = 0;
    for (size_t i = 0; i <= size; i++) {
        char c = (i == size) ? ' ' : code[i];
        if (c == ' ' || c == '\n' || c == '\t' || i == size) {
            if (tlen == 0) continue;
            tok[tlen] = '\0';

            if ((tok[0] >= '0' && tok[0] <= '9') ||
                (tok[0] == '-' && tok[1] && tok[1] >= '0' && tok[1] <= '9')) {
                if (sp < 32) stack[sp++] = to_int(tok);
            } else if (str_equal(tok, "+")) {
                if (sp >= 2) { stack[sp-2] += stack[sp-1]; sp--; }
            } else if (str_equal(tok, "-")) {
                if (sp >= 2) { stack[sp-2] -= stack[sp-1]; sp--; }
            } else if (str_equal(tok, "*")) {
                if (sp >= 2) { stack[sp-2] *= stack[sp-1]; sp--; }
            } else if (str_equal(tok, "/")) {
                if (sp >= 2 && stack[sp-1] != 0) { stack[sp-2] /= stack[sp-1]; sp--; }
            } else if (str_equal(tok, "print")) {
                if (sp > 0) { console_udec((unsigned)stack[--sp]); console_putc('\n'); }
            } else if (str_equal(tok, "char")) {
                if (sp > 0) { console_putc((char)stack[--sp]); }
            } else if (str_equal(tok, "dup")) {
                if (sp > 0 && sp < 32) { stack[sp] = stack[sp-1]; sp++; }
            } else if (str_equal(tok, "drop")) {
                if (sp > 0) sp--;
            } else if (str_equal(tok, "swap")) {
                if (sp >= 2) { int t = stack[sp-1]; stack[sp-1] = stack[sp-2]; stack[sp-2] = t; }
            } else if (str_equal(tok, "exit")) {
                return;
            }
            tlen = 0;
        } else {
            if (tlen < 31) tok[tlen++] = c;
        }
    }
}

