# ExoCore Kernel-Integrated Module Loader

This document outlines the design for a kernel level loader capable of scanning directories under `/mpymod` at boot. Each directory provides a `manifest.json` describing a MicroPython bytecode module and any associated native modules that should be registered.

## Directory layout
```
/mpymod/<name>/manifest.json
/mpymod/<name>/<module>.mpy
/mpymod/<name>/native/
```

The manifest contains:
- `mpy_entry` – path to the `.mpy` file to execute
- `mpy_import_as` – name used when inserting the module into `sys.modules`
- `c_modules` – list of native modules, specified as objects containing `name` and `path`

Example:
```json
{
  "mpy_entry": "mod.mpy",
  "mpy_import_as": "testmod",
  "c_modules": [
    {"name": "board", "path": "native/board.o"}
  ]
}
```

## Boot behaviour
1. The kernel enumerates `/mpymod/*` folders.
2. For each folder the manifest is parsed.
3. The `.mpy` file is loaded through the MicroPython C API using `mp_embed_exec_mpy`.
4. The resulting module object is stored in `sys.modules` under the name provided by `mpy_import_as`.
5. Entries in `c_modules` are matched against built‑in native modules and registered if present.

All modules are loaded before any user applications run so Python code can invoke them immediately without a separate loader.

## Using the loader

1. Compile your Python code into bytecode using `mpy-cross`:
   ```bash
   mpy-cross mylib.py
   ```
   This produces `mylib.mpy`.
2. Create `/mpymod/mylib/manifest.json` with the following fields:
   ```json
   {
     "mpy_entry": "mylib.mpy",
     "mpy_import_as": "mylib",
     "c_modules": []
   }
   ```
3. Place `mylib.mpy` alongside the manifest in the same folder on the boot image.
4. Rebuild the ISO and boot the kernel. At the console you can now run:
   ```python
   import mylib
   ```
   and the module loads automatically.

## Creating native libraries

Native modules may be written in C for performance or hardware access.
Compile the object file with your cross compiler and list it in the manifest:

```json
{
  "mpy_entry": "mylib.mpy",
  "mpy_import_as": "mylib",
  "c_modules": [
    {"name": "board", "path": "native/board.o"}
  ]
}
```

The kernel matches the `name` field with built‑in module stubs. When present the
object is loaded so `import mylib` exposes the native functionality.
