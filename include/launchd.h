#ifndef LAUNCHD_H
#define LAUNCHD_H

#include "multiboot.h"

#ifdef __cplusplus
extern "C" {
#endif

int launchd_boot(multiboot_info_t *mbi);

#ifdef __cplusplus
}
#endif

#endif /* LAUNCHD_H */
