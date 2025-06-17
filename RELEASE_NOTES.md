# Release Notes

## Bug Fixes
- Multiboot header now resides in the first load segment so GRUB can detect the kernel
- Corrected typo in `build.sh` that prevented kernel compilation
- Fixed page table setup in `boot.S` so CR3 is loaded with the page directory base, preventing early triple faults
- Corrected stack pointer initialization in `boot.S` which caused a page fault and system reset
- Fixed invalid `lea` operand syntax in `boot.S` that broke the 64-bit stack setup
- Headers now use `extern "C"` guards, preventing unresolved symbols when linking with C++
- Fixed undefined references in `00_kernel_tester` by linking serial stubs and adding `strncmp`

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
- Modules now link against a serial stub for debugging output

## New Features
- Example `memtest` module using new API
- Host test script `tests/test_mem.sh` verifying allocator
- PS/2 keyboard module and updated `console_getc`
- Interrupt descriptor table with basic IRQ handling
- Kernel boots directly into 64-bit long mode
- Modules can be raw binaries or ELF executables
- Kernel now skips non-ELF modules to avoid jumping into invalid binaries

## Minor Fixes
- Resolved pointer-size warnings in module loader
- Added final newline to linker script
- Build script installs `mtools` when missing, resolving "mformat" failures

## Additional Improvements
- `build.sh` checks for upstream updates and can automatically pull them
- QEMU run command now accepts an optional `nographic` argument
- QEMU command honors `QEMU_EXTRA` for additional debugging flags


## New Features
- Automatic "kernel tester" module validates 64-bit mode and library linkage
- New GRUB entries: "ExoCore-Kernel (Debug)" enables verbose serial/VGA logs and "ExoCore-OS (alpha)" boots userland modules
- Build script compiles userland modules and packages them separately
- Improved full kernel test script integrates with build.sh for detailed logs
