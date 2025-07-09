# ExoCore Kernel-Integrated Module Loader

The kernel embeds MicroPython modules found under `/mpymod` at build time. Each module lives in its own folder named after the module and must contain an `init.py`. Any C sources placed under `native/` within that folder are compiled and linked into the kernel.

At boot the MicroPython runtime loads every embedded module and runs its `init.py`. Modules share an environment dictionary called `env` and have access to two helper modules:

- `c` – provides `c.exec(...)` for calling native code wrappers
- `mpyrun` – runs other embedded modules by name

Modules should update `env` with any configuration or APIs they wish to expose before the main `init.py` executes.

## Directory layout
```
/mpymod/<module>/init.py
/mpymod/<module>/native/*.c  (optional)
```

## Creating a module
1. Create a folder under `mpymod/` named after your module, e.g. `mpymod/mylib`.
2. Place your module's Python code in `init.py` inside this folder.
3. Optionally add a `native` directory with C files providing native helpers.
4. Rebuild the project; the build script packages the module automatically.

During boot the kernel will execute `mpymod/mylib/init.py`. You can access the shared environment like this:
```python
from env import env
env['mylib_loaded'] = True
```

To call another module:
```python
mpyrun('other_mod')
```

Native C helpers can be invoked via:
```python
c.exec('function_name', 1, 2, 3)
```

