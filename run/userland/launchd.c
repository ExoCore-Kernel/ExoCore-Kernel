#include <stddef.h>
#include <stdint.h>
#include "syscall.h"

static long syscall3(long n, long a, long b, long c) {
    long ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(n), "D"(a), "S"(b), "d"(c) : "memory");
    return ret;
}

static void write_text(const char *text) {
    size_t len = 0;
    while (text[len])
        ++len;
    syscall3(SYS_WRITE, (long)text, (long)len, 0);
}

void _start(void) {
    write_text("launchd userland binary entered\n");
    syscall3(SYS_PROC_SPAWN, (long)"shelld.elf", 0, 0);
    for (;;) {
    }
}
