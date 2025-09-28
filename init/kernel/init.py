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
            return tuple(modules)
        except TypeError:
            return DEFAULT_MODULES
    return DEFAULT_MODULES


def _load_module(name):
    try:
        mod = mpyrun(name)
    except Exception as exc:  # pragma: no cover - runtime diagnostics
        print("[!] failed to load %s: %s" % (name, exc))
        return None
    if mod is None:
        print("[!] module %s not available" % name)
        return None
    shell_state["loaded"][name] = mod
    return mod


def bootstrap():
    print("ExoCore MicroPython shell starting...")
    modules = _profile_modules()
    profile = _safe_get(shell_state, "profile")
    if isinstance(profile, dict):
        profile_name = _safe_get(profile, "profile", "custom")
        print("Profile: %s" % profile_name)
    else:
        print("Profile: builtin")
    print("Preloading modules: %s" % ", ".join(modules))
    for name in modules:
        _load_module(name)

    profile = _safe_get(shell_state, "profile")
    if isinstance(profile, dict):
        bootstrapper = _safe_get(profile, "bootstrap")
        if callable(bootstrapper):
            try:
                shell_state["profile_state"] = bootstrapper()
            except Exception as exc:
                print("[!] profile bootstrap failed: %s" % exc)


def _describe_env_value(key, value):
    if isinstance(value, dict):
        return "dict(%s)" % ", ".join(sorted(value.keys()))
    if callable(value):
        return "callable"
    return type(value).__name__


def cmd_help(args):
    print("Available commands:")
    for name in sorted(COMMANDS.keys()):
        print("  %-8s - %s" % (name, COMMANDS[name][1]))


def cmd_modules(args):
    if not shell_state["loaded"]:
        print("No modules loaded yet; use 'load <name>'.")
        return
    for name in sorted(shell_state["loaded"].keys()):
        mod = shell_state["loaded"][name]
        attrs = [k for k in dir(mod) if not k.startswith("__")]
        print("%s: %s" % (name, ", ".join(attrs) if attrs else "<no public symbols>"))


def cmd_load(args):
    if not args:
        print("Usage: load <module>")
        return
    name = args[0]
    if name in shell_state["loaded"]:
        print("Module %s already loaded." % name)
        return
    if _load_module(name):
        print("Module %s ready." % name)


def cmd_env(args):
    try:
        keys = sorted(env.keys())
    except AttributeError:
        keys = sorted(env)
    if not keys:
        print("env is empty")
        return
    for key in keys:
        print("%s: %s" % (key, _describe_env_value(key, env[key])))


def cmd_status(args):
    memory = _safe_get(env, "memory")
    if isinstance(memory, dict) and "heap_free" in memory:
        try:
            heap = memory["heap_free"]()
            print("Heap free: %s bytes" % heap)
        except Exception as exc:
            print("heap_free unavailable: %s" % exc)
    else:
        print("Memory module not loaded.")

    runstate = _safe_get(env, "runstate")
    if isinstance(runstate, dict):
        try:
            print("Current program: %s" % runstate["current_program"]())
        except Exception as exc:
            print("current_program unavailable: %s" % exc)
        try:
            print("User app active: %s" % runstate["current_user_app"]())
        except Exception as exc:
            print("current_user_app unavailable: %s" % exc)
        try:
            print("Debug mode: %s" % runstate["is_debug_mode"]())
        except Exception as exc:
            print("is_debug_mode unavailable: %s" % exc)
        try:
            print("VGA output: %s" % runstate["is_vga_output"]())
        except Exception as exc:
            print("is_vga_output unavailable: %s" % exc)
    else:
        print("Runstate module not loaded.")

    vga_enabled = _safe_get(env, "vga_enabled")
    if vga_enabled is not None:
        print("VGA toggle cached: %s" % vga_enabled)


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
            print("Result: %s" % (result,))
    except Exception as exc:
        print("Execution failed: %s" % exc)


def cmd_py(args):
    if not args:
        print("Usage: py <python expression>")
        return
    code = " ".join(args)
    ns = {"env": env, "mpyrun": mpyrun, "loaded": shell_state["loaded"]}
    try:
        exec(code, ns, ns)
    except Exception as exc:
        print("Python error: %s" % exc)


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
            print("  %s: %s" % (key, profile[key]))
    state = _safe_get(shell_state, "profile_state")
    if state is not None:
        print("Profile state: %s" % state)


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


def dispatch(line):
    parts = line.strip().split()
    if not parts:
        return
    command = parts[0]
    handler = _safe_get(COMMANDS, command)
    if not handler:
        print("Unknown command '%s'. Type 'help' for a list." % command)
        return
    func = handler[0]
    try:
        func(parts[1:])
    except SystemExit:
        raise
    except Exception as exc:  # pragma: no cover - runtime diagnostics
        print("[!] command failed: %s" % exc)


def repl():
    while True:
        try:
            line = input(SHELL_PROMPT)
        except EOFError:
            print("EOF")
            break
        except KeyboardInterrupt:
            print("^C")
            continue
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
    print("Exiting shell. Final heap free: %s" % heap_free)


main()
