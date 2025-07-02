#ifndef EXOCORE_CONFIG_H
#define EXOCORE_CONFIG_H

/* Enable run-directory modules */
#define FEATURE_RUN_DIR 1

/* Fixed load address for all modules */
#define MODULE_BASE_ADDR 0x00200000

/* Display boot logo when running modules */
#define FEATURE_BOOT_LOGO 1

/* Render VGA logs alongside the logo */
#define BOOTLOGO_SHOW_LOGS 1

/* Boot logo placement */
#define BOOTLOGO_POS_X 0
#define BOOTLOGO_POS_Y 0

#endif /* EXOCORE_CONFIG_H */
