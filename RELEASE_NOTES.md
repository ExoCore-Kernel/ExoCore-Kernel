# Release Notes

## New Features
- MicroPython `vga_draw` module exposes an off-screen drawing API with hide/show control for VGA output
- Kernel init now boots an ExoDraw-driven UI demo that renders a simple layout and mirrors actions to the console for debugging
- Ring-3 syscall interface for init.elf covering memory, filesystem and process control
- init.elf executes in user mode with kernel memory protection
- Added ten kernel-integrated MicroPython libraries providing console, serial, debug logging, memory, filesystem, run-state, hardware, keyboard, TinyScript, and module execution helpers

## Improvements
- `build.sh` now performs incremental rebuilds, adds a clean target, and reuses unchanged artifacts
- IDT adds user-accessible system call gate ensuring ring-3 access

## Bug Fixes
- Multiboot header now resides in the first load segment so GRUB can detect the kernel
- Corrected typo in `build.sh` that prevented kernel compilation
- Added missing backslash for `debuglog.c` compile and linked `fs.c` so debug logging builds cleanly
- Fixed page table setup in `boot.S` so CR3 is loaded with the page directory base, preventing early triple faults
- Corrected stack pointer initialization in `boot.S` which caused a page fault and system reset
- Fixed invalid `lea` operand syntax in `boot.S` that broke the 64-bit stack setup
- Headers now use `extern "C"` guards, preventing unresolved symbols when linking with C++
- Fixed undefined references in `00_kernel_tester` by linking serial stubs and adding `strncmp`
- Corrected ELF loader to handle 64-bit program headers, fixing invalid opcode crashes on boot
- Kernel now links the I/O helper library so cpuid and rdtsc functions resolve
- Fixed missing global declaration for `end` symbol causing build failures in `kernel_main`
- Restored MicroPython module linking by embedding module sources and loading them during boot
- Enabled `sys` and properly initialized built-in modules so embedded MicroPython scripts import without errors

## Improvements
- Added priority-based ballooning allocator
- Kernel initializes memory manager on boot
- Memory usage per app can be tracked and limited
- Pointer-sized heap pointers remove compiler warnings
- Memory routines now use `uintptr_t` and `size_t` for 64-bit safety
- Static `io` helpers simplify port I/O access
- Shared `panic` routine for consistent fatal error reporting
- Build script supports multiple architectures and defaults to 64-bit
- Module base address consolidated into `config.h` ensuring consistent loading
- New `console_uhex` function for hexadecimal output
- Kernel prints module addresses and entry points in hex
- Modules build with `-m64` when using the x86_64 toolchain
- Modules now link against a serial stub for debugging output
- Removed unsupported architectures from the build script; only native and
  x86_64 cross-compilation remain

## New Features
- Example `memtest` module using new API
- Host test script `tests/test_mem.sh` verifying allocator
- PS/2 keyboard module and updated `console_getc`
- Interrupt descriptor table with basic IRQ handling
- Kernel boots directly into 64-bit long mode
- Modules can be raw binaries or ELF executables
- Non-ELF modules now execute via MicroPython instead of being skipped

## Minor Fixes
- Resolved pointer-size warnings in module loader
- Added final newline to linker script
- Build script installs `mtools` when missing, resolving "mformat" failures

## Bug Fixes
- User modules no longer hang the kernel; all example ELFs return control so
  subsequent modules execute.

## Additional Improvements
- `build.sh` checks for upstream updates and can automatically pull them
- QEMU run command now accepts an optional `nographic` argument
- QEMU command honors `QEMU_EXTRA` for additional debugging flags
- `build.sh` now detects the first configured Git remote when `origin` is
  missing so kernel updates are still checked
- Build script adds `-mstackrealign -fno-omit-frame-pointer` for consistent stack alignment


## New Features
- Automatic "kernel tester" module validates 64-bit mode and library linkage
- New GRUB entries: "ExoCore-Kernel (Debug)" enables verbose serial/VGA logs and "ExoCore-Management-shell (alpha)" boots userland modules
- Build script compiles userland modules and packages them separately
- Improved full kernel test script integrates with build.sh for detailed logs
- Boot-time kernel tests now print colored success/fail messages on VGA

## New Features
- Simple interactive shell with command history
- Paging support using a swap buffer when memory is exhausted
- Console now supports backspace and clear operations
- Arrow key navigation in console_getc for command history
- Memory manager now supports saving and retrieving per-app data
- Shell commands added for saving strings and showing memory usage
- Shell 'help' command replaces 'elp' and graphics test draws colored rectangles

- Added TinyScript interpreter for text-based modules
- MicroPython runtime integrated; '.py' modules now execute via embedded interpreter
- Compiled '.mpy' files are packaged in the ISO and loaded instead of '.py'
- Added `run/micropython_test.py` example to verify MicroPython script loading
- Files starting with `#mpyexo` execute as MicroPython scripts when not valid ELF binaries

## Bug Fixes
- Fixed MicroPython build errors by adding missing include path and implementing libc stubs.
- Non-ELF modules without .py or .mpy extensions are skipped instead of executing via MicroPython.
- Python module extension matching is now case-insensitive so files like `.PY` and `.MPY` load correctly.

- Loader now detects script extensions using a simplified lowercase comparison to avoid skipping valid TinyScript or MicroPython modules.

- Fixed unmatched closing brace in kernel module loader
- Userland Python modules now include the file path in the module string so the
  loader detects '.py' and '.mpy' correctly

## New Features
- Memory-backed filesystem driver that can mount a storage region
  and read or write arbitrary bytes.
- Host test script `test_fs.sh` ensures driver functionality.
- Basic ATA PIO block device driver for reading and writing disk sectors.
- FAT filesystem driver using ATA, able to mount real disks and scan for bad sectors.
- Debug log buffer saved to exocoredebug.txt on halt or panic
- debuglog_save_file writes the in-memory log to exocoredebug.txt
- New hexdump helper and verbose module loader output for easier debugging
- Boot sequence now logs memory manager location and IDT initialization
- Added debuglog_memdump for dumping memory with addresses
- Memory, filesystem and module loader operations now print detailed diagnostics
- Timestamped debug events using rdtsc for cycle counts
- CPU vendor and feature bits logged during boot
- Stack pointer and memory map recorded on entry
- Serial and console initialization messages
- Heap end address, free space after each module and at shutdown
- Application memory operations show app IDs and handles
- Filesystem reads and writes include data hexdumps
- Panic handler prints a timestamp before flushing the log
- Crash log exocorecrash.txt records RIP, program source and dumps IDT and nearby memory
- Debug log saved in JSON format for easier machine parsing
- Structured JSON events include timestamped messages for each step
- Debug logs now only print when debug mode is enabled
- MicroPython console output routed to VGA unless 'nompvga' kernel flag
- Raw '.py' modules are now copied directly to the ISO instead of being compiled to '.mpy'
- Fixed MicroPython output not appearing on serial by routing stdout/stderr to serial
- Carriage returns in MicroPython output no longer show as garbage on the VGA console
- Verbose boot logs only appear when debug mode is enabled
- Documented kernel-integrated MicroPython module loader design
- Added usage instructions and native module example
- Build script now detects modules in `mpymod` and links listed native sources
- Removed obsolete example VGA MicroPython module causing build errors
- Kernel automatically loads modules from /mpymod at boot using a persistent MicroPython runtime
- Build script keeps the Micropython source up to date automatically
- Fixed stack initialization for MicroPython runtime to prevent general protection faults
- Kernel now halts on page fault exceptions instead of rebooting
- Debug mode panics show a 'Guru Meditation' red screen
- Panics disable interrupts to avoid reboot loops
- Freed memory is overwritten with 0xDEADBEEF when debug mode
- Build number tracking moved to `build.txt` with automatic reset on major updates
- Build script optionally stores GitHub credentials via `GITHUB_USER` and `GITHUB_TOKEN`
\n- Kernel boots by running init from init/kernel/init.py. Module loader removed.
- Boot now searches only for raw init/kernel/init.py and no longer accepts .mpy files
- Added internal strstr implementation to fix missing symbol during linking

- Reintroduced mpymod loader executing init.py from each module
- Documented how to create and use custom mpymod modules
- Fixed mpymod native build by adding missing MicroPython include paths
- mpymod loader now stores scripts in the `env` module so boot doesn't fail when
  builtins are read-only
- Removed MicroPython mpymod loader again and added support for init/kernel/init.elf

- Fixed build failures due to missing STATIC macro and multiboot type mismatch
