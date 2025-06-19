#include "../include/ata_pio.h"
#include "../include/io.h"

#define ATA_IO_BASE 0x1F0
#define ATA_CTRL_BASE 0x3F6

static void ata_wait_bsy(void) {
    while (io_inb(ATA_IO_BASE + 7) & 0x80) {
    }
}

static void ata_wait_drq(void) {
    while (!(io_inb(ATA_IO_BASE + 7) & 0x08)) {
    }
}

void ata_pio_init(void) {
    /* Disable IRQs */
    io_outb(ATA_CTRL_BASE, 0x02);
}

int ata_pio_read(uint32_t lba, void *buf, size_t count) {
    uint16_t *buffer = (uint16_t *)buf;
    for (size_t i = 0; i < count; ++i) {
        ata_wait_bsy();
        io_outb(ATA_IO_BASE + 6, 0xE0 | ((lba >> 24) & 0x0F));
        io_outb(ATA_IO_BASE + 2, 1);
        io_outb(ATA_IO_BASE + 3, (uint8_t)lba);
        io_outb(ATA_IO_BASE + 4, (uint8_t)(lba >> 8));
        io_outb(ATA_IO_BASE + 5, (uint8_t)(lba >> 16));
        io_outb(ATA_IO_BASE + 7, 0x20);
        ata_wait_bsy();
        ata_wait_drq();
        for (int w = 0; w < 256; ++w)
            buffer[w] = io_inw(ATA_IO_BASE);
        buffer += 256;
        ++lba;
    }
    return 0;
}

int ata_pio_write(uint32_t lba, const void *buf, size_t count) {
    const uint16_t *buffer = (const uint16_t *)buf;
    for (size_t i = 0; i < count; ++i) {
        ata_wait_bsy();
        io_outb(ATA_IO_BASE + 6, 0xE0 | ((lba >> 24) & 0x0F));
        io_outb(ATA_IO_BASE + 2, 1);
        io_outb(ATA_IO_BASE + 3, (uint8_t)lba);
        io_outb(ATA_IO_BASE + 4, (uint8_t)(lba >> 8));
        io_outb(ATA_IO_BASE + 5, (uint8_t)(lba >> 16));
        io_outb(ATA_IO_BASE + 7, 0x30);
        ata_wait_bsy();
        ata_wait_drq();
        for (int w = 0; w < 256; ++w)
            io_outw(ATA_IO_BASE, buffer[w]);
        buffer += 256;
        ++lba;
    }
    return 0;
}
