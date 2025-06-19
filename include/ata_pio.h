#ifndef ATA_PIO_H
#define ATA_PIO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ata_pio_init(void);
int ata_pio_read(uint32_t lba, void *buf, size_t count);
int ata_pio_write(uint32_t lba, const void *buf, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* ATA_PIO_H */
