# ExoCore Kernel-Integrated Module Loader

The kernel integrates MicroPython modules from the `mpymod/` tree during the build process and loads them at boot.

## How module loading works

- The build pipeline scans module directories under `mpymod/`.
- Each module is configured by `manifest.json`.
- Python entry files are loaded as raw `.py` source.
- Optional native C modules are compiled and exposed to the MicroPython runtime.
- During boot, module initialization scripts register APIs in shared `env` keys.

## Module directory model

- `mpymod/<module_name>/manifest.json` defines import and entry metadata.
- `mpymod/<module_name>/init.py` contains bootstrap and API exports when used as entrypoint.
- `mpymod/<module_name>/native/*.c` contains optional native bindings.

## Shared runtime integration points

- `env`: shared dictionary used for capability registration and cross-module discovery.
- Import namespace: module name exposed through `manifest.json` `mpy_import_as`.
- Native bridge: optional bound native module imported from Python wrappers.

## Source-format requirement

ExoCore currently supports raw `.py` module source files for MicroPython integration. Compiled `.mpy` artifacts are not part of the supported module-loading path.

## Documentation map

For full module authoring and API guidance:

- `docs/Module-Development-Guide.md`
- `docs/Kernel-Module-Reference.md`
