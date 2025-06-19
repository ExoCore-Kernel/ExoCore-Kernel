#include "../include/ata_pio.h"
#include "../include/memutils.h"
#include <stdint.h>

#define DISK_SECTORS 32

static unsigned char disk[DISK_SECTORS * 512];
static int bad_sector = -1;

void ata_pio_init(void) {
    memset(disk, 0, sizeof(disk));
    bad_sector = -1;
}

void mock_ata_set_bad_sector(int lba) {
    if (lba >= 0 && lba < DISK_SECTORS)
        bad_sector = lba;
}

int ata_pio_read(uint32_t lba, void *buf, size_t count) {
    if (lba + count > DISK_SECTORS)
        return -1;
    for (size_t i = 0; i < count; ++i) {
        if ((int)(lba + i) == bad_sector)
            return -1;
        memcpy((unsigned char *)buf + i * 512, disk + (lba + i) * 512, 512);
    }
    return 0;
}

int ata_pio_write(uint32_t lba, const void *buf, size_t count) {
    if (lba + count > DISK_SECTORS)
        return -1;
    for (size_t i = 0; i < count; ++i) {
        memcpy(disk + (lba + i) * 512, (const unsigned char *)buf + i * 512, 512);
    }
    return 0;
}
