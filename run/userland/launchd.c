#include <stddef.h>
#include <stdint.h>
#include "syscall.h"
#include "vfs.h"

static char cfg[512];
static char path[128];
static char charbuf[2];

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

static int same_suffix(const char *path, const char *name) {
    const char *base = path;
    for (const char *p = path; *p; ++p) {
        if (*p == '/')
            base = p + 1;
    }
    while (*base && *name && *base == *name) {
        ++base;
        ++name;
    }
    return *base == '\0' && *name == '\0';
}

static void log_daemon_name(const char *path) {
    const char *base = path;
    for (const char *p = path; *p; ++p) {
        if (*p == '/')
            base = p + 1;
    }
    write_text("launchd: \"");
    while (*base && *base != '.') {
        char ch[2];
        ch[0] = *base++;
        ch[1] = '\0';
        write_text(ch);
    }
    write_text("\" found\n");
}

static void start_daemon(const char *path) {
    if (same_suffix(path, "shelld.elf")) {
        write_text("launchd: \"shelld\" found\n");
        write_text("launchd: starting shelld executable as PID 2\n");
    } else {
        log_daemon_name(path);
        write_text("launchd: starting configured daemon executable\n");
    }
    long pid = syscall3(SYS_PROC_SPAWN, (long)path, 0, 0);
    if (pid > 0) {
        int status = 0;
        syscall3(SYS_PROC_WAIT, pid, (long)&status, 0);
    }
}

void _start(void) {
    write_text("[launchd] real launchd ELF started as PID 1\n");
    write_text("launchd: hello from launchd!\n");
    write_text("launchd: finding LaunchDaemons\n");

    int fd = (int)syscall3(SYS_VFS_OPEN, (long)"/launchd.cfg", VFS_O_RDONLY, 0);
    if (fd < 0) {
        write_text("launchd: missing /launchd.cfg\n");
        return;
    }
    long got = syscall3(SYS_VFS_READ, fd, (long)cfg, sizeof(cfg) - 1);
    syscall3(SYS_VFS_CLOSE, fd, 0, 0);
    if (got <= 0) {
        write_text("launchd: empty /launchd.cfg\n");
        return;
    }
    cfg[got] = '\0';

    const char *cursor = cfg;
    while (*cursor) {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n')
            ++cursor;
        if (!*cursor)
            break;
        size_t i = 0;
        if (*cursor != '/' && i + 1 < sizeof(path))
            path[i++] = '/';
        while (*cursor && *cursor != ' ' && *cursor != '\t' && *cursor != '\r' && *cursor != '\n' && i + 1 < sizeof(path))
            path[i++] = *cursor++;
        path[i] = '\0';
        while (*cursor && *cursor != '\n')
            ++cursor;
        if (path[0])
            start_daemon(path);
    }

    for (;;)
        syscall3(SYS_SLEEP_MS, 1000, 0, 0);
}
