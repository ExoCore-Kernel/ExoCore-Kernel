#ifndef BOOTLOGO_H
#define BOOTLOGO_H
#include <stdint.h>
int bootlogo_install_to_vfs(void);
int bootlogo_draw_from_vfs(void);
void bootlogo_draw_progress(uint32_t percent);
#endif
