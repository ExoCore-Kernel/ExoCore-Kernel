#ifndef MODEXEC_H
#define MODEXEC_H
#include <stdint.h>
#include "multiboot.h"
#ifdef __cplusplus
extern "C" {
#endif
void modexec_set_mbi(multiboot_info_t *mbi);
int modexec_run(const char *name);
#ifdef __cplusplus
}
#endif
#endif /* MODEXEC_H */
