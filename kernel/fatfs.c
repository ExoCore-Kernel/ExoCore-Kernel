#include "fatfs.h"
#include "ata_pio.h"
#include "memutils.h"

/* Minimal FAT filesystem driver that uses the existing ATA PIO block
 * device driver. It supports mounting a volume located at a starting
 * LBA and performing raw sector reads and writes. The driver also scans
 * the filesystem for sectors that fail to read, treating them as bad.
 */

static uint32_t fat_start_lba = 0;

int fat_mount(uint32_t lba_start) {
    unsigned char sector[512];
    fat_start_lba = lba_start;
    if (ata_pio_read(lba_start, sector, 1) != 0)
        return -1;
    /* Verify boot sector signature */
    if (sector[510] != 0x55 || sector[511] != 0xAA)
        return -1;
    /* Simple sanity check for the string "FAT" in the header */
    for (int i = 54; i < 62; ++i) {
        if (sector[i] == 'F' && sector[i+1] == 'A' && sector[i+2] == 'T')
            return 0;
    }
    return -1;
}

int fat_read(uint32_t lba, void *buffer, size_t count) {
    return ata_pio_read(fat_start_lba + lba, buffer, count);
}

int fat_write(uint32_t lba, const void *buffer, size_t count) {
    return ata_pio_write(fat_start_lba + lba, buffer, count);
}

int fat_scan_bad_sectors(void) {
    unsigned char sector[512];
    int bad = 0;
    for (uint32_t i = 0; i < 16; ++i) {
        if (ata_pio_read(fat_start_lba + i, sector, 1) != 0)
            ++bad;
    }
    return bad;
}
