#ifndef FATFS_H
#define FATFS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize and mount a FAT filesystem located at the
 * given starting LBA sector. Returns 0 on success.
 */
int fat_mount(uint32_t lba_start);

/* Read 'count' sectors starting at LBA 'lba' into 'buffer'.
 * Returns 0 on success.
 */
int fat_read(uint32_t lba, void *buffer, size_t count);

/* Write 'count' sectors starting at LBA 'lba' from 'buffer'.
 * Returns 0 on success.
 */
int fat_write(uint32_t lba, const void *buffer, size_t count);

/* Scan the filesystem for unreadable sectors. Returns the number of
 * bad sectors encountered.
 */
int fat_scan_bad_sectors(void);

#ifdef __cplusplus
}
#endif

#endif /* FATFS_H */
