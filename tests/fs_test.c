#include <stdio.h>
#include "../include/fs.h"
#include "../include/mem.h"

int main() {
    static unsigned char heap[0x1000];
    mem_init((uintptr_t)heap, sizeof(heap));
    unsigned char buf[128];
    fs_mount(buf, sizeof(buf));
    const char msg[] = "hello";
    if (fs_write(0, msg, sizeof(msg)) != sizeof(msg)) return 1;
    char tmp[16];
    if (fs_read(0, tmp, sizeof(msg)) != sizeof(msg)) return 1;
    if (__builtin_memcmp(msg, tmp, sizeof(msg)) != 0) return 1;
    printf("fs driver ok\n");
    return 0;
}
