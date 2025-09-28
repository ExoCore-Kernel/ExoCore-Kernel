#mpyexo
"""Interactive MicroPython shell showcasing kernel capabilities.

This init script boots straight into a tiny management shell that
demonstrates the integrated MicroPython environment and the helpers
exposed by the ExoCore kernel.  The shell intentionally keeps its
dependencies simple so it can run on the raw interpreter that ships
with the kernel (no precompiled bytecode required).
"""

try:
    from env import env, mpyrun
except ImportError as exc:  # pragma: no cover - defensive fallback
    print("env module unavailable:", exc)
    raise SystemExit


def _safe_get(mapping, key, default=None):
    """Return ``mapping[key]`` with broad compatibility safeguards."""
    if mapping is None:
        return default
    getter = getattr(mapping, "get", None)
    if callable(getter):
        try:
            return getter(key, default)
        except TypeError:
            try:
                return getter(key)
            except Exception:  # pragma: no cover - defensive fallback
                return default
    try:
        return mapping[key]
    except Exception:  # pragma: no cover - defensive fallback
        return default

SHELL_PROMPT = "exo> "
DEFAULT_MODULES = (
    "consolectl",
    "debugview",
    "hwinfo",
    "memstats",
    "modrunner",
    "runstatectl",
    "serialctl",
    "vga",
)

shell_state = {
    "loaded": {},
    "profile": None,
    "profile_state": None,
}


def _profile_modules():
    profile = _safe_get(env, "shell")
    if isinstance(profile, dict):
        modules = _safe_get(profile, "modules", DEFAULT_MODULES)
        if not modules:
            modules = DEFAULT_MODULES
        shell_state["profile"] = profile
        try:
            cleaned = []
            for entry in modules:
                cleaned.append(str(entry))
            return tuple(cleaned)
        except TypeError:
            return DEFAULT_MODULES
    return DEFAULT_MODULES


def _load_module(name):
    name = str(name)
    try:
        mod = mpyrun(name)
    except Exception as exc:  # pragma: no cover - runtime diagnostics
        print("[!] failed to load " + name + ": " + str(exc))
        return None
    if mod is None:
        print("[!] module " + name + " not available")
        return None
    shell_state["loaded"][name] = mod
    return mod


def bootstrap():
    print("ExoCore MicroPython shell starting...")
    modules = _profile_modules()
    profile = _safe_get(shell_state, "profile")
    if isinstance(profile, dict):
        profile_name = _safe_get(profile, "profile", "custom")
        print("Profile: " + str(profile_name))
    else:
        print("Profile: builtin")
    module_names = []
    for name in modules:
        module_names.append(name)
    print("Preloading modules: " + ", ".join(module_names))
    for name in module_names:
        _load_module(name)

    profile = _safe_get(shell_state, "profile")
    if isinstance(profile, dict):
        bootstrapper = _safe_get(profile, "bootstrap")
        if callable(bootstrapper):
            try:
                shell_state["profile_state"] = bootstrapper()
            except Exception as exc:
                print("[!] profile bootstrap failed: " + str(exc))


def _describe_env_value(key, value):
    if isinstance(value, dict):
        entries = []
        for entry in sorted(value.keys()):
            entries.append(str(entry))
        return "dict(" + ", ".join(entries) + ")"
    if callable(value):
        return "callable"
    return type(value).__name__


def cmd_help(args):
    print("Available commands:")
    for name in sorted(COMMANDS.keys()):
        label = str(name)
        if len(label) < 8:
            label = label + " " * (8 - len(label))
        description = str(COMMANDS[name][1])
        print("  " + label + " - " + description)


def cmd_modules(args):
    if not shell_state["loaded"]:
        print("No modules loaded yet; use 'load <name>'.")
        return
    for name in sorted(shell_state["loaded"].keys()):
        mod = shell_state["loaded"][name]
        attrs = [k for k in dir(mod) if not k.startswith("__")]
        details = ", ".join(attrs) if attrs else "<no public symbols>"
        print(str(name) + ": " + details)


def cmd_load(args):
    if not args:
        print("Usage: load <module>")
        return
    name = args[0]
    if name in shell_state["loaded"]:
        print("Module " + str(name) + " already loaded.")
        return
    if _load_module(name):
        print("Module " + str(name) + " ready.")


def cmd_env(args):
    try:
        keys = sorted(env.keys())
    except AttributeError:
        keys = sorted(env)
    if not keys:
        print("env is empty")
        return
    for key in keys:
        print(str(key) + ": " + _describe_env_value(key, env[key]))


def cmd_status(args):
    memory = _safe_get(env, "memory")
    if isinstance(memory, dict) and "heap_free" in memory:
        try:
            heap = memory["heap_free"]()
            print("Heap free: " + str(heap) + " bytes")
        except Exception as exc:
            print("heap_free unavailable: " + str(exc))
    else:
        print("Memory module not loaded.")

    runstate = _safe_get(env, "runstate")
    if isinstance(runstate, dict):
        try:
            print("Current program: " + str(runstate["current_program"]()))
        except Exception as exc:
            print("current_program unavailable: " + str(exc))
        try:
            print("User app active: " + str(runstate["current_user_app"]()))
        except Exception as exc:
            print("current_user_app unavailable: " + str(exc))
        try:
            print("Debug mode: " + str(runstate["is_debug_mode"]()))
        except Exception as exc:
            print("is_debug_mode unavailable: " + str(exc))
        try:
            print("VGA output: " + str(runstate["is_vga_output"]()))
        except Exception as exc:
            print("is_vga_output unavailable: " + str(exc))
    else:
        print("Runstate module not loaded.")

    vga_enabled = _safe_get(env, "vga_enabled")
    if vga_enabled is not None:
        print("VGA toggle cached: " + str(vga_enabled))


def cmd_run(args):
    if not args:
        print("Usage: run <module>")
        return
    modules_env = _safe_get(env, "modules")
    if not isinstance(modules_env, dict) or "run" not in modules_env:
        print("modrunner not available; load 'modrunner' first.")
        return
    target = args[0]
    try:
        result = modules_env["run"](target)
        if result is not None:
            print("Result: " + str(result))
    except Exception as exc:
        print("Execution failed: " + str(exc))


def cmd_py(args):
    if not args:
        print("Usage: py <python expression>")
        return
    code = " ".join(args)
    ns = {"env": env, "mpyrun": mpyrun, "loaded": shell_state["loaded"]}
    try:
        exec(code, ns, ns)
    except Exception as exc:
        print("Python error: " + str(exc))


def cmd_profile(args):
    profile = _safe_get(shell_state, "profile")
    if not isinstance(profile, dict):
        print("No management profile active.")
        return
    print("Profile details:")
    for key in sorted(profile.keys()):
        if key == "bootstrap":
            print("  bootstrap: <callable>")
        else:
            print("  " + str(key) + ": " + str(profile[key]))
    state = _safe_get(shell_state, "profile_state")
    if state is not None:
        print("Profile state: " + str(state))


def cmd_exit(args):
    raise SystemExit


COMMANDS = {
    "help": (cmd_help, "show this help text"),
    "modules": (cmd_modules, "list loaded MicroPython modules"),
    "load": (cmd_load, "load a MicroPython module by name"),
    "env": (cmd_env, "inspect the shared kernel environment"),
    "status": (cmd_status, "display kernel run-state information"),
    "run": (cmd_run, "execute a stored module via modrunner"),
    "py": (cmd_py, "execute a one-line Python snippet"),
    "profile": (cmd_profile, "show active management profile info"),
    "exit": (cmd_exit, "leave the shell"),
    "quit": (cmd_exit, "alias for exit"),
}


def _readline(prompt):
    reader = globals().get("input")
    if callable(reader):
        try:
            return reader(prompt)
        except NameError:
            pass
    console = _safe_get(env, "console")
    if isinstance(console, dict):
        writer = console.get("write")
        if callable(writer):
            writer(prompt)
    keyboard = _safe_get(env, "keyboard")
    if isinstance(keyboard, dict):
        reader_fn = keyboard.get("read_char")
        if callable(reader_fn):
            buffer = []
            while True:
                ch = reader_fn()
                if not ch or ch == "\n" or ch == "\r":
                    break
                buffer.append(ch)
            return "".join(buffer)
    return None


def dispatch(line):
    parts = line.strip().split()
    if not parts:
        return
    command = parts[0]
    handler = _safe_get(COMMANDS, command)
    if not handler:
        print("Unknown command '" + str(command) + "'. Type 'help' for a list.")
        return
    func = handler[0]
    args = []
    for index in range(1, len(parts)):
        args.append(parts[index])
    try:
        func(args)
    except SystemExit:
        raise
    except Exception as exc:  # pragma: no cover - runtime diagnostics
        print("[!] command failed: " + str(exc))


def repl():
    while True:
        try:
            line = _readline(SHELL_PROMPT)
        except EOFError:
            print("EOF")
            break
        except KeyboardInterrupt:
            print("^C")
            continue
        if line is None:
            print("Input unavailable; exiting shell.")
            break
        if line.strip() in ("exit", "quit"):
            break
        try:
            dispatch(line)
        except SystemExit:
            break


def main():
    bootstrap()
    cmd_help(())
    repl()
    heap_free = "?"
    memory = env.get("memory")
    if isinstance(memory, dict) and "heap_free" in memory:
        try:
            heap_free = memory["heap_free"]()
        except Exception:
            heap_free = "?"
    print("Exiting shell. Final heap free: " + str(heap_free))


main()
