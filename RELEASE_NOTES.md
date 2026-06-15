## 0T0015F

### Bug Fixes
- Ran flat userland processes on their allocated process stack so `SYS_READ` accepts shell stack buffers instead of rejecting input before PS/2 or serial polling.
- Mirrored interactive console output to raw serial so QEMU `-serial stdio` shells display prompts, typed-character echo, and command output even while VGA/framebuffer output remains enabled.

### Improvements
- Prioritized pending serial input before PS/2 polling so terminal fallback keystrokes are not delayed by keyboard-controller status noise.
- Kept serial terminal compatibility aligned with the PS/2/VGA shell path for headless and fallback testing.

## 0T0018B

### Bug Fixes
- Fixed interactive shell prompt and typed-character echo rendering so `exo> ` and keystrokes appear immediately on PS/2 and serial-backed consoles instead of waiting for Enter.
- Fixed the `clear` shell command to reset the kernel console and emit a serial terminal clear sequence instead of only printing blank lines.

### Improvements
- Added a shell-accessible console clear path through the existing ioctl syscall surface.

### New Features
- None.

## 0T0014F

### Bug Fixes
- Fixed PS/2 keyboard input by ignoring break codes and only treating extended arrow scancodes as shell navigation keys.
- Removed the release ISO artifact from the source patch.

### Improvements
- Prioritized real PS/2 key events before serial fallback in the framebuffer console input path.

### New Features
- None.

## 0T0013F

### Bug Fixes
- Fixed framebuffer/PS2 arrow key delivery so shell code receives complete ANSI arrow escape sequences instead of losing Up/Down to console-only scrolling.

### Improvements
- Added interactive shell navigation that lets Left/Right cycle command history and Up/Down print recent debug log windows from the framebuffer shell path.

### New Features
- Added queued keyboard escape sequence support for framebuffer and linked console input paths.

## 0T0012F

### Bug Fixes
- Fixed framebuffer console tail-following so text uses the available framebuffer height before scrolling.
- Fixed framebuffer terminal line spacing by adding vertical padding between rendered text rows.
- Rebuilt the embedded userland shell path for interactive input validation.

### Improvements
- The framebuffer console now derives its visible row count from framebuffer height instead of the legacy 25-row VGA limit.

### New Features
- None.

# 0T0009F

## New Features
- Added a validating x86_64 ELF loader for FAT32/VFS-provided userland executables.
- Launchd now boots as PID 1 from `/launchd.elf` and child daemons spawn from configured ELF paths.

## Improvements
- Launchd boot now mirrors FAT32 userland files into VFS and uses VFS reads for executable loading.
- Process records now track executable entry points, image ranges, stack pointers, parentage, and executable paths.
- Incremented the compiled boot version to `0T0009F`.

## Bug Fixes
- Removed the default fake kernel-side launchd service path and moved boot ordering so Multiboot magic is validated before dereferencing Multiboot info.

# 0T0008F

## New Features
- Added a launchd-style userland service bootstrap that mounts a FAT32 userland image, copies `/launchd.elf` into process-owned memory, and assigns launchd PID 1 as the root userland process.
- Added `shelld` as a simple shell LaunchDaemon configured through `launchd.cfg` and loaded by launchd as PID 2 with launchd as its parent.
- Added build packaging for a FAT32 `userland.img` containing launchd, shelld, and launchd configuration.

## Improvements
- Kernel boot now logs the launchd and shelld startup path in QEMU-visible serial output after backend tests pass.
- Incremented the compiled boot version to `0T0008F`.

## Bug Fixes
- Kept raw MicroPython script packaging unchanged while adding the launch daemon image alongside existing boot modules.

# 0T0007F

## New Features
- Added a FAT32-backed filesystem driver that detects FAT32 boot sectors during `fs_mount` and supports root-directory 8.3 file create, open, read, write, seek, size, and close operations.
- Added dedicated FAT32 file-descriptor syscall numbers while keeping byte-offset `SYS_FS_READ` and `SYS_FS_WRITE` on the filesystem backing store.
- Added separate `SYS_VFS_*` syscall names and numbers so VFS calls remain distinct from the FAT32 FS syscall surface.

## Improvements
- Preserved raw in-memory backing behavior for unformatted buffers so debug logging and existing offset-based FS users continue to work.
- Extended filesystem tests to cover both raw byte access and FAT32 file operations across cluster boundaries.

## Bug Fixes
- Ensured FAT32 writes update directory file size and starting cluster metadata immediately.
- Ensured newly allocated FAT32 clusters are zeroed and linked through mirrored FAT updates.

# Release Notes

## 0T0011F

### New Features
- Added an embedded initramfs fallback containing `launchd.elf`, `launchd.cfg`, and `shelld.elf` so launchd can boot when `userland.img` is missing or unavailable.
- Added the `mpy <file.py>` shelld command to execute raw MicroPython source files through the kernel MicroPython runtime with live console output.

### Improvements
- Build now regenerates `kernel/embedded_userland.c` from the freshly linked userland ELF/config files and links it into the kernel.
- Launchd boot now converges FAT32 and initramfs setup before shared ELF validation and PID 1 handoff.
- Incremented the compiled boot version to `0T0011F`.

### Bug Fixes
- Added initramfs VFS install validation so failed writes abort boot instead of pretending success.
- Added fallback handling for unavailable FAT32 userland images and clear panic logs when no userland or init system can be used.

## 2026-06-12 Backend Driver Foundations

### New Features
- Added an in-kernel RAM VFS backend with path lookup, directories, file creation, read/write/seek/close, stat/fstat, directory iteration, chdir/getcwd, unlink, and rename support.
- Added memory-context ownership tracking with quotas, allocation accounting, peak usage, and context teardown for future syscall/process integration.
- Added a process-handler backend with PID allocation, parent/child state, current-process tracking, process-owned memory, process-owned file descriptors, exit, and wait collection.

### Improvements
- Added a boot-time backend tester that validates filesystem, memory-context, and process-handler behavior before MicroPython init runs.
- Disabled the auto-running ExoDraw/VGA demo path so boot logs focus on backend/syscall readiness tests.
- Incremented the build version to `0T0006F`.

### Bug Fixes
- Prevented the VGA demo sample from auto-executing during normal boots while keeping it available as an importable script.

## New Features
- Framebuffer-backed bitmap font renderer for ExoDraw and console output in the new "NO VGA" boot profile
- MicroPython `vga_draw` module exposes an off-screen drawing API with hide/show control for VGA output
- Kernel init now boots an ExoDraw-driven UI demo that renders a simple layout and mirrors actions to the console for debugging
- Updated ExoDraw init splash renders a full-screen blue "Hello ExoPort!" showcase highlighting canvas, rect, line, and text commands
- Ring-3 syscall interface for init.elf covering memory, filesystem and process control
- init.elf executes in user mode with kernel memory protection
- Added ten kernel-integrated MicroPython libraries providing console, serial, debug logging, memory, filesystem, run-state, hardware, keyboard, TinyScript, and module execution helpers

## Improvements
- GRUB boot menu now includes a graphics-ready "NO VGA" option that switches to gfxterm before launching the kernel
- ExoDraw init menu now paints directly to VGA with highlighted selections for each demo option
- `build.sh` now performs incremental rebuilds, adds a clean target, and reuses unchanged artifacts
- IDT adds user-accessible system call gate ensuring ring-3 access
- Kernel memory manager now includes guarded RAM/VRAM tracking with automatic use-after-free panics

## Bug Fixes
- `novgacon` now keeps framebuffer console rendering active so ExoDraw output remains visible in the NO VGA boot profile
- Fixed the VGA demo selector so it no longer logs menu entries without drawing them to the screen
- `vga_draw.present()` now unhides VGA output by default so ExoDraw scenes reach the visible display without requiring an explicit flag
- ExoDraw boot no longer raises a NameError because init scripts reseed missing globals before running
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
- Fixed `init/kernel/init.py` text command parsing to avoid MicroPython syntax faults during boot

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

- Fixed MicroPython init script parsing by removing unsupported exception chaining in `init/kernel/init.py`
- Fixed build failures due to missing STATIC macro and multiboot type mismatch

## 0T0010F

### New Features
- Added an interactive `shelld` command loop with built-in help, shell metadata, file, directory, process-launch, FAT alias, and diagnostic commands.
- Added syscall numbers and dispatch support for process identity/list/wait/kill, fd read/write/dup, VFS rmdir/access, uptime/sleep, exec/spawn, dmesg, memory info, sync, ioctl, mount/disk placeholders, and power controls.

### Improvements
- Added VFS helpers for removing empty directories and path access checks.
- Added process helpers for listing processes, killing processes, and duplicating process file descriptors.
- Updated the compiled boot version to `0T0010F`.

### Bug Fixes
- `SYS_READ` now supports fd `0` by reading serial keyboard input for the interactive shell.

## 0T0016F

### Bug fixes
- Protected PID 1 from user kill requests and added a clean panic path if init exits internally.
- Converted user-triggered allocation failures in process spawning, ELF loading, and MicroPython file loading into returned errors instead of kernel panics.
- Cleaned up partially allocated process images and file buffers on failed executable loads.
- Fixed duplicated boot/backend console output caused by writing the same message through both console mirroring and direct serial logging.

### Improvements
- Added a growable kernel heap with committed/max accounting, growth logging, and allocation failure counters.
- Expanded the `mem` shell command with heap used/free/committed/max/grow/failure fields.
- Improved `kill`, `run`, `spawn`, and `mpy` shell error messages for protected PID and out-of-memory cases.

### New features
- Added backend coverage for PID 1 kill protection and normal child kill behavior.

## 0T0018B

### New Features
- Added dark, no-logs, and white framebuffer boot mode state with GRUB entries for clean-logo boot and inverted white console mode.
- Added an embedded `/boot/logo.exoimg` boot logo flow that installs the compiled logo into VFS, reloads it through VFS, validates it, and draws it centered with alpha blending.
- Added framebuffer syscalls for display information, log visibility/theme control, screen clear, and RGB pixel drawing.
- Added `tools/png_to_exoimg.py` for converting PNG assets into the EXOIMG1 RGBA8888 format.

### Improvements
- Preserved debug log buffering and serial output while allowing early framebuffer/VGA log output to be hidden until shelld enables it.
- Updated shelld to enable kernel log visibility as it reaches the interactive prompt.
- Added backend coverage for command-line boot mode parsing, log visibility transitions, theme state, and `.exoimg` validation failures.

### Bug Fixes
- Made framebuffer RGBA drawing respect the multiboot framebuffer channel masks instead of assuming a fixed RGB/BGR memory order.

### Follow-up
- Removed the committed binary `assets/logo.exoimg`; local `assets/logo.exoimg` and generated `kernel/embedded_logo.c` are ignored so logo binaries can be supplied outside review.
- Updated the boot-logo build path to tolerate a missing local logo asset and continue booting cleanly until one is added.
## 0T0022F

### New Features
- Added an optional smooth framebuffer boot progress bar that can be enabled with the `bootprogress`, `progress`, or `splashprogress` kernel command-line flag.

### Improvements
- Recentered the boot logo as a logo-plus-progress splash group so the visible splash remains balanced when the progress bar is active.
- In no-logs boots, `shelld` now clears the splash/log display before showing a clean top-left `exo> ` prompt.

### Bug Fixes
- Kept the no-logs shell path from re-enabling hidden boot logs as it reaches the first interactive prompt.
- Incremented the compiled boot version to `0T0022F`.
