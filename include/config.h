#ifndef EXOCORE_CONFIG_H
#define EXOCORE_CONFIG_H

/* Enable run-directory modules */
#define FEATURE_RUN_DIR 0

/* Fixed load address for all modules */
#define MODULE_BASE_ADDR 0x00200000

/* Module loading mode: MicroPython only, ELF only, or both */
#define MODULE_MODE_MICROPYTHON 0
#define MODULE_MODE_ELF        1
#define MODULE_MODE_BOTH       2

#ifndef MODULE_MODE
#define MODULE_MODE MODULE_MODE_MICROPYTHON
#endif

/*
 * Heap sizing
 *
 * The MicroPython runtime and the kernel allocator both need enough headroom
 * to preload bundled modules during boot. 64 KiB proved too small once the
 * process management module grew larger, so both heaps default to 192 KiB.
 */
#ifndef EXOCORE_KERNEL_HEAP_SIZE
#define EXOCORE_KERNEL_HEAP_SIZE (192 * 1024)
#endif

#ifndef EXOCORE_MICROPY_HEAP_SIZE
#define EXOCORE_MICROPY_HEAP_SIZE (192 * 1024)
#endif


#endif /* EXOCORE_CONFIG_H */
