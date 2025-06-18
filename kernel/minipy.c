#include "minipy.h"
#include "console.h"
#include <stddef.h>

#define MAX_VARS 16
#define MAX_NAME 16

struct var { char name[MAX_NAME]; int value; };

static struct var vars[MAX_VARS];
static int var_count = 0;

static const char *ptr;

static int is_space(char c) { return c == ' ' || c == '\t' || c == '\r'; }
static int is_digit(char c) { return c >= '0' && c <= '9'; }
static int is_alpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }

static void skip_ws(void) { while (is_space(*ptr)) ptr++; }

static int get_var(const char *name) {
    for (int i = 0; i < var_count; i++)
        if (!strncmp(name, vars[i].name, MAX_NAME))
            return vars[i].value;
    return 0;
}

static void set_var(const char *name, int value) {
    for (int i = 0; i < var_count; i++)
        if (!strncmp(name, vars[i].name, MAX_NAME)) {
            vars[i].value = value; return; }
    if (var_count < MAX_VARS) {
        int j = 0; while (name[j] && j < MAX_NAME-1) { vars[var_count].name[j] = name[j]; j++; }
        vars[var_count].name[j] = '\0';
        vars[var_count].value = value; var_count++;
    }
}

static void parse_ident(char *out) {
    int i = 0; while (is_alpha(*ptr) || is_digit(*ptr)) {
        if (i < MAX_NAME-1) out[i++] = *ptr; ptr++; }
    out[i] = '\0';
}

static int parse_int(void) {
    int sign = 1; if (*ptr == '-') { sign = -1; ptr++; }
    int v = 0; while (is_digit(*ptr)) { v = v*10 + (*ptr - '0'); ptr++; }
    return sign * v;
}

static int parse_factor(void);

static int parse_term(void) {
    int val = parse_factor(); skip_ws();
    while (*ptr == '*' || *ptr == '/') {
        char op = *ptr++; int rhs; skip_ws(); rhs = parse_factor(); skip_ws();
        if (op == '*') val *= rhs; else if (rhs) val /= rhs;
    }
    return val;
}

static int parse_expr(void) {
    int val = parse_term(); skip_ws();
    while (*ptr == '+' || *ptr == '-') {
        char op = *ptr++; int rhs; skip_ws(); rhs = parse_term(); skip_ws();
        if (op == '+') val += rhs; else val -= rhs;
    }
    return val;
}

static int parse_factor(void) {
    skip_ws();
    if (*ptr == '(') { ptr++; int v = parse_expr(); if (*ptr == ')') ptr++; return v; }
    if (is_digit(*ptr) || (*ptr == '-' && is_digit(ptr[1]))) return parse_int();
    if (is_alpha(*ptr)) {
        char name[MAX_NAME]; parse_ident(name); return get_var(name);
    }
    return 0;
}

static void exec_line(const char *line) {
    ptr = line; skip_ws();
    if (*ptr == '\0' || *ptr == '#') return;
    if (!strncmp(ptr, "print", 5) && is_space(ptr[5])) {
        ptr += 5; skip_ws();
        int v = parse_expr(); console_udec((unsigned)v); console_putc('\n');
        return;
    }
    if (is_alpha(*ptr)) {
        char name[MAX_NAME]; parse_ident(name); skip_ws();
        if (*ptr == '=') { ptr++; skip_ws(); int v = parse_expr(); set_var(name, v); }
    }
}

void run_minipy(const char *code, size_t size) {
    char line[128]; size_t len = 0;
    for (size_t i = 0; i <= size; i++) {
        char c = (i == size) ? '\n' : code[i];
        if (c == '\r') continue;
        if (c == '\n' || c == ';') {
            line[len] = '\0';
            exec_line(line);
            len = 0;
        } else if (len < sizeof(line) - 1) {
            line[len++] = c;
        }
    }
}

