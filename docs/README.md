# ExoCore Kernel Documentation Index

This index lists core documentation for building, integrating, and using MicroPython modules in ExoCore Kernel.

## Module development and integration

- `Kernel-Integrated-Module-Loader.md`:
  - Module discovery and boot-time loading model.
  - Manifest and entrypoint expectations.
  - Shared runtime integration points.
  - Raw `.py` source requirement.

- `Module-Development-Guide.md`:
  - Full authoring workflow for new modules.
  - `manifest.json` contract details.
  - `init.py` design and registration rules.
  - Native C binding integration and validation checklist.
  - Troubleshooting for loader/runtime failures.

- `Kernel-Module-Reference.md`:
  - Built-in module inventory.
  - Import names, entry files, and `env` keys.
  - API surface summary for each bundled kernel module.

## Recommended read order for new contributors

1. `Kernel-Integrated-Module-Loader.md`
2. `Module-Development-Guide.md`
3. `Kernel-Module-Reference.md`
