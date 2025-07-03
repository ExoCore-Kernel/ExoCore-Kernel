#ifndef EXOCORE_CONFIG_H
#define EXOCORE_CONFIG_H

/* Enable run-directory modules */
#define FEATURE_RUN_DIR 1

/* Fixed load address for all modules */
#define MODULE_BASE_ADDR 0x00200000

/* Module loading mode: MicroPython only, ELF only, or both */
#define MODULE_MODE_MICROPYTHON 0
#define MODULE_MODE_ELF        1
#define MODULE_MODE_BOTH       2

#ifndef MODULE_MODE
#define MODULE_MODE MODULE_MODE_MICROPYTHON
#endif


#endif /* EXOCORE_CONFIG_H */
