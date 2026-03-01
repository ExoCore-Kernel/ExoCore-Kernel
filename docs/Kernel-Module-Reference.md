# ExoCore Built-in MicroPython Module Reference

This document maps each built-in module to its import name, environment registration key, and primary API surface.

## Module inventory

| Module folder | Import name | Entry file | `env` key | Purpose |
|---|---|---|---|---|
| `consolectl` | `consolectl` | `init.py` | `console` | Text console and framebuffer blit utilities. |
| `debugview` | `debugview` | `init.py` | `debuglog` | Debug log buffering, console dump, and file persistence. |
| `fsbridge` | `fsbridge` | `init.py` | `fs` | Filesystem read/write and mount/capacity checks. |
| `hwinfo` | `hwinfo` | `init.py` | `hwinfo` | Hardware-oriented low-level calls (`rdtsc`, `cpuid`, port I/O). |
| `keyinput` | `keyinput` | `init.py` | `keyboard` | Keyboard character/code input APIs. |
| `memstats` | `memstats` | `init.py` | `memory` | Heap statistics and app memory bookkeeping. |
| `modrunner` | `modrunner` | `init.py` | `modules` | Run one or multiple modules by name. |
| `process` | `process` | `init.py` | none | Cooperative process tree, permissions, and runtime policy. |
| `runstatectl` | `runstatectl` | `init.py` | `runstate` | Current program/app state and debug/VGA mode toggles. |
| `serialctl` | `serialctl` | `init.py` | `serial` | Serial writes (text, decimal, hex, raw bytes). |
| `tscript` | `tscript` | `init.py` | `tscript` | Execute text script payloads and line arrays. |
| `vga` | `vga` | none | `vga` | VGA enable/disable state wiring. |
| `vga_demo` | `vgademo` | `demo.py` | `vga_demo` | Demo bootstrap that enables VGA. |
| `vga_draw` | `vga_draw` | none | `vga_draw` | Character-cell drawing primitives and shape helpers. |

## API reference by module

### `consolectl`

Primary functions:

- `write`, `clear`, `set_attr`, `scroll`, `backspace`.
- `blit_pixels`, `blit_pixels_scaled`, `framebuffer_info`.
- `fb_create`, `fb_create_bestfit`, `fb_resize`.
- `fb_set_pixel`, `fb_clear`, `fb_fill_rect`.
- `fb_present`, `fb_present_fullscreen`, `fb_sleep_hz`.

`env['console']` includes all functions above plus `colors` and `ansi`.

### `debugview`

Primary functions:

- `flush`, `dump_console`, `save_file`, `buffer`, `is_available`.

All capabilities are registered under `env['debuglog']`.

### `fsbridge`

Primary functions:

- `read`, `write`, `capacity`, `is_mounted`.

All capabilities are registered under `env['fs']`.

### `hwinfo`

Primary functions:

- `rdtsc`, `cpuid`, `inb`, `outb`.

All capabilities are registered under `env['hwinfo']`.

### `keyinput`

Primary functions:

- `read_char`, `read_code`.

All capabilities are registered under `env['keyboard']`.

### `memstats`

Primary functions:

- `heap_free`, `register_app`, `app_used`, `save_block`, `load_block`.

All capabilities are registered under `env['memory']`.

### `modrunner`

Primary functions:

- `run`.
- `run_many` (validates list/tuple input and executes sequential module runs).

All capabilities are registered under `env['modules']`.

### `process`

Exports process lifecycle and policy types:

- Classes: `ProcessManager`, `Process`.
- Exceptions: `ProcessError`, `ProcessPermissionError`, `ProcessStateError`, `ProcessNotFound`, `ProcessTerminated`, `ProcessTimeout`.
- Runtime state constants are defined through `ProcessState`.

The module focuses on process hierarchy, inherited permission constraints, runtime timeouts, and parent/child control checks.

### `runstatectl`

Primary functions:

- `current_program`, `set_current_program`.
- `current_user_app`, `set_current_user_app`.
- `is_debug_mode`, `set_debug_mode`.
- `is_vga_output`, `set_vga_output`.

All capabilities are registered under `env['runstate']`.

### `serialctl`

Primary functions:

- `write`, `write_hex`, `write_dec`, `raw_write`.

All capabilities are registered under `env['serial']`.

### `tscript`

Primary functions:

- `run`.
- `run_lines` (joins line arrays into one program string before execution).

All capabilities are registered under `env['tscript']`.

### `vga`

Primary function:

- `enable(flag)` sets `env['vga_enabled']`.

Module load also marks `env['vga'] = True`.

### `vga_demo`

On load, the module enables VGA through `vga.enable(True)` and marks `env['vga_demo'] = 'enabled'`.

### `vga_draw`

Primary drawing functions:

- Session and primitives: `start`, `clear`, `pixel`, `line`, `rect`, `present`, `show`, `is_hidden`.
- Helpers: `circle`, `ellipse`, `polygon`.

Module exports color constants and display dimensions from native bindings, then registers all drawing capabilities under `env['vga_draw']`.

## Compatibility notes

- Use raw `.py` modules only; compiled `.mpy` binaries are not supported in this kernel.
- Match import names to each module manifest `mpy_import_as` value.
- Prefer `env` lookup for dynamic capability discovery between modules.
