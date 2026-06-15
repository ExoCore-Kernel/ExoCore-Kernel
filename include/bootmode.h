#ifndef BOOTMODE_H
#define BOOTMODE_H
#include <stdint.h>
#define BOOT_THEME_DARK 0u
#define BOOT_THEME_WHITE 1u
void bootmode_init(void);
void bootmode_parse_cmdline(const char *cmd);
int bootmode_logs_visible(void);
void bootmode_set_logs_visible(int visible);
int bootmode_progress_visible(void);
void bootmode_set_progress_visible(int visible);
uint32_t bootmode_theme(void);
void bootmode_set_theme(uint32_t theme);
uint8_t bootmode_fg(void);
uint8_t bootmode_bg(void);
#endif
