# Release Notes

## Bug Fixes
- None

## Improvements
- Added priority-based ballooning allocator
- Kernel initializes memory manager on boot
- Memory usage per app can be tracked and limited
- Pointer-sized heap pointers remove compiler warnings
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
- Linker script adjusted so multiboot header and text load at offset 0x100000
- Modules are numbered 01-99 to enforce load order
