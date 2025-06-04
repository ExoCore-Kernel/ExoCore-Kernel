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

## New Features
- Example `memtest` module using new API
- Host test script `tests/test_mem.sh` verifying allocator
- PS/2 keyboard module and updated `console_getc`
- Interrupt descriptor table with basic IRQ handling
