# Release Notes

## Bug Fixes
- Multiboot header now resides in the first load segment so GRUB can detect the kernel
- Corrected typo in `build.sh` that prevented kernel compilation

## Improvements
- Added priority-based ballooning allocator
- Kernel initializes memory manager on boot
- Memory usage per app can be tracked and limited
- Pointer-sized heap pointers remove compiler warnings
- Memory routines now use `uintptr_t` and `size_t` for 64-bit safety
- Static `io` helpers simplify port I/O access
- Shared `panic` routine for consistent fatal error reporting
- Build script supports multiple architectures and defaults to 64-bit
- New `console_uhex` function for hexadecimal output
- Kernel prints module addresses and entry points in hex
- Modules build with `-m64` when using the x86_64 toolchain

## New Features
- Example `memtest` module using new API
- Host test script `tests/test_mem.sh` verifying allocator
- PS/2 keyboard module and updated `console_getc`
- Interrupt descriptor table with basic IRQ handling
- Kernel boots directly into 64-bit long mode
- Modules can be raw binaries or ELF executables

## Minor Fixes
- Resolved pointer-size warnings in module loader
- Added final newline to linker script
- Build script installs `mtools` when missing, resolving "mformat" failures
- Build script installs remaining prerequisites (cross compilers, NASM, GRUB,
  QEMU) automatically if absent

## Additional Improvements
- `build.sh` checks for upstream updates and can automatically pull them

