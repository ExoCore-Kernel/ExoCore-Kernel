#ifndef MODEXEC_H
#define MODEXEC_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct multiboot_info;
void modexec_set_mbi(struct multiboot_info *mbi);
int modexec_run(const char *name);
#ifdef __cplusplus
}
#endif
#endif /* MODEXEC_H */
