#include <stdio.h>
#include "../include/fatfs.h"
#include "../include/memutils.h"
#include "../include/ata_pio.h"

/* from mock_ata_mem.c */
void mock_ata_set_bad_sector(int lba);

int main() {
    ata_pio_init();

    unsigned char boot[512] = {0};
    boot[510] = 0x55;
    boot[511] = 0xAA;
    boot[54] = 'F';
    boot[55] = 'A';
    boot[56] = 'T';
    if (ata_pio_write(0, boot, 1) != 0) return 1;

    if (fat_mount(0) != 0) {
        printf("mount failed\n");
        return 1;
    }

    unsigned char src[512];
    for (int i = 0; i < 512; ++i) src[i] = (unsigned char)i;
    if (fat_write(1, src, 1) != 0) return 1;

    unsigned char dst[512];
    if (fat_read(1, dst, 1) != 0) return 1;
    if (memcmp(src, dst, 512) != 0) {
        printf("read/write mismatch\n");
        return 1;
    }

    mock_ata_set_bad_sector(2);
    int bad = fat_scan_bad_sectors();
    if (bad != 1) {
        printf("bad sector count %d\n", bad);
        return 1;
    }

    printf("fatfs ok\n");
    return 0;
}
