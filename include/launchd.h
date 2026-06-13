#ifndef LAUNCHD_H
#define LAUNCHD_H

#include "multiboot.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int install_initramfs_file(const char *path, const void *data, size_t size);
int install_embedded_initramfs(void);
int launchd_boot(multiboot_info_t *mbi);

#ifdef __cplusplus
}
#endif

#endif /* LAUNCHD_H */
