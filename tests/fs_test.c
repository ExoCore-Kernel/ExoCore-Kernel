#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../include/fs.h"
#include "../include/mem.h"

#define T_FS_O_RDONLY 0x01
#define T_FS_O_RDWR   0x03
#define T_FS_O_CREAT  0x10
#define T_FS_O_TRUNC  0x20
#define T_FS_SEEK_SET 0

static void put16(unsigned char *p, uint16_t v) {
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

static void put32(unsigned char *p, uint32_t v) {
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static void make_fat32(unsigned char *img, size_t bytes) {
    (void)bytes;
    const uint32_t total_sectors = 70000;
    const uint16_t reserved = 32;
    const uint32_t sectors_per_fat = 600;
    img[0] = 0xEB;
    img[1] = 0x58;
    img[2] = 0x90;
    img[3] = 'E'; img[4] = 'X'; img[5] = 'O'; img[6] = 'F';
    img[7] = 'A'; img[8] = 'T'; img[9] = '3'; img[10] = '2';
    put16(img + 11, 512);
    img[13] = 1;
    put16(img + 14, reserved);
    img[16] = 1;
    put16(img + 17, 0);
    put16(img + 19, 0);
    img[21] = 0xF8;
    put16(img + 22, 0);
    put32(img + 32, total_sectors);
    put32(img + 36, sectors_per_fat);
    put32(img + 44, 2);
    img[82] = 'F'; img[83] = 'A'; img[84] = 'T'; img[85] = '3'; img[86] = '2';
    img[510] = 0x55;
    img[511] = 0xAA;

    unsigned char *fat = img + reserved * 512;
    put32(fat + 0, 0x0FFFFFF8);
    put32(fat + 4, 0x0FFFFFFF);
    put32(fat + 8, 0x0FFFFFFF);
}

int main() {
    static unsigned char heap[0x1000];
    mem_init((uintptr_t)heap, sizeof(heap));

    unsigned char raw[128];
    fs_mount(raw, sizeof(raw));
    const char msg[] = "hello";
    if (fs_write(0, msg, sizeof(msg)) != sizeof(msg)) return 1;
    char tmp[16];
    if (fs_read(0, tmp, sizeof(msg)) != sizeof(msg)) return 1;
    if (__builtin_memcmp(msg, tmp, sizeof(msg)) != 0) return 1;

    const size_t image_size = 70000U * 512U;
    unsigned char *image = calloc(1, image_size);
    if (!image) return 1;
    make_fat32(image, image_size);
    fs_mount(image, image_size);
    if (!fs_is_fat32()) return 1;

    int fd = fs_open("/KERNEL.TXT", T_FS_O_CREAT | T_FS_O_RDWR | T_FS_O_TRUNC);
    if (fd < 0) return 1;
    char payload[700];
    for (size_t i = 0; i < sizeof(payload); ++i)
        payload[i] = (char)('A' + (i % 26));
    if (fs_write_fd(fd, payload, sizeof(payload)) != (long)sizeof(payload)) return 1;
    if (fs_file_size(fd) != (long)sizeof(payload)) return 1;
    if (fs_lseek_fd(fd, 0, T_FS_SEEK_SET) != 0) return 1;
    char out[700];
    if (fs_read_fd(fd, out, sizeof(out)) != (long)sizeof(out)) return 1;
    if (__builtin_memcmp(payload, out, sizeof(payload)) != 0) return 1;
    if (fs_close(fd) != 0) return 1;

    fd = fs_open("KERNEL.TXT", T_FS_O_RDONLY);
    if (fd < 0) return 1;
    if (fs_read_fd(fd, out, sizeof(out)) != (long)sizeof(out)) return 1;
    if (__builtin_memcmp(payload, out, sizeof(payload)) != 0) return 1;
    if (fs_close(fd) != 0) return 1;

    free(image);
    printf("fat32 fs driver ok\n");
    return 0;
}
