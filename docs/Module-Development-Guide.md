# ExoCore MicroPython Module Development Guide

This guide defines the end-to-end workflow for creating and integrating MicroPython modules in ExoCore Kernel.

## Runtime and packaging constraints

- ExoCore loads module source from raw `.py` files.
- Compiled `.mpy` artifacts are not supported by this kernel.
- Every module must be packaged under `mpymod/<module_name>/`.
- Every module must ship a `manifest.json` file.
- Most modules should include `init.py` as their bootstrap script.

## Required module layout

Each module directory under `mpymod/` should contain:

- `manifest.json`: import name, entry file, and optional native bindings.
- `init.py`: module bootstrap code and exported APIs.
- `native/*.c` (optional): C-backed functions exposed to MicroPython.

## `manifest.json` contract

The module loader expects a JSON document with these keys:

- `mpy_import_as`: the import name visible to MicroPython (`import <name>`).
- `mpy_entry`: Python entry file to execute during load.
- `c_modules` (optional): list of native C modules to compile and expose.

Guidelines:

- Keep `mpy_import_as` aligned with the module folder intent.
- Use `mpy_entry: "init.py"` when your bootstrap file is `init.py`.
- If a module is native-first, `mpy_entry` may be empty and Python code can still exist for utility wrappers.
- Ensure each `c_modules[].name` matches the symbol imported in Python.

## Authoring `init.py`

`init.py` is executed by the kernel during module initialization when configured as the manifest entry.

Recommended structure:

1. Print a concise module-loaded banner for boot traceability.
2. Import `env` and register module capabilities there for cross-module discovery.
3. Import native bindings when needed.
4. Provide validation and type-normalization at module boundaries.
5. Export stable function names that scripts can rely on.

`env` registration guidance:

- Register a top-level key that matches feature scope (`console`, `fs`, `memory`, and similar).
- Store callable references rather than precomputed output where possible.
- Keep values JSON-like where practical to simplify inspection tooling.

## Native module integration

For C-backed modules:

- Add source under `mpymod/<module_name>/native/`.
- Register source files in `manifest.json` under `c_modules`.
- Import the bound symbol from Python (for example, `<name>_native`) and wrap it with MicroPython-safe argument handling.
- Keep Python wrappers thin and deterministic.

## Module API design rules

- Prefer explicit function names over overloaded behavior.
- Normalize untrusted inputs in Python before calling native functions.
- Use deterministic return values (`True/False`, dict payloads, or typed scalars).
- Expose feature groups in `env` for shared discovery.
- Keep module side effects limited to initialization and explicit API calls.

## Inter-module usage patterns

Modules can interact in two ways:

- Direct import via `mpy_import_as` names.
- Shared capability lookup via `env` keys populated during initialization.

Use `env` when decoupling from concrete module names is important.
Use direct import when stable function access is required.

## Validation checklist for new modules

Before merging a new module:

- Confirm `manifest.json` is valid JSON.
- Confirm entry script path exists and is raw `.py`.
- Confirm native module names match Python imports.
- Confirm module registers expected `env` keys.
- Confirm kernel boots and module load banner appears.
- Confirm API calls succeed in MicroPython runtime.

## Troubleshooting

If a module does not appear at runtime:

- Verify the module folder is inside `mpymod/`.
- Verify `manifest.json` fields and spelling.
- Verify `mpy_entry` points to an existing file.
- Verify native symbols can be imported when using C modules.
- Check boot logs for module load output.

If a module imports but APIs fail:

- Validate parameter normalization in Python wrapper code.
- Validate native function signatures and argument counts.
- Validate module registration in `env` and downstream lookup keys.
