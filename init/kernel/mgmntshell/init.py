#mpyexo
"""Management shell profile for ExoCore.

This module is packaged only with the management shell boot option.  It
preloads a curated set of MicroPython helpers and exposes metadata used by
``init/kernel/init.py`` to present a tailored experience.  When booted via the
management shell GRUB entry, an interactive debug console is launched that can
inspect logs, memory, and runtime state from both serial and VGA output.
"""

from env import env, mpyrun

LOG_WINDOW = 10
PROMPT = "[mgmnt]$ "

MANAGEMENT_MODULES = (
    "consolectl",
    "modrunner",
    "memstats",
    "runstatectl",
    "serialctl",
    "debugview",
    "hwinfo",
    "keyinput",
)


def safe_get(mapping, key, default=None):
    if mapping is None:
        return default
    getter = getattr(mapping, "get", None)
    if callable(getter):
        try:
            return getter(key, default)
        except Exception:
            pass
    try:
        return mapping[key]
    except Exception:
        pass
    try:
        return getattr(mapping, key)
    except Exception:
        return default


def bootstrap():
    loaded = {}
    for name in MANAGEMENT_MODULES:
        try:
            mod = mpyrun(name)
        except Exception as exc:  # pragma: no cover - runtime diagnostics
            print("[mgmnt] failed to load %s: %s" % (name, exc))
            continue
        if mod is None:
            print("[mgmnt] module %s unavailable" % name)
            continue
        loaded[name] = mod
    env["shell_state"] = {
        "loaded": tuple(sorted(loaded.keys())),
        "last_boot": None,
    }
    return loaded


def module_list():
    return MANAGEMENT_MODULES


class DebugShell:
    def __init__(self, modules):
        self.modules = modules
        self.console = safe_get(env, "console")
        self.serial = safe_get(env, "serial")
        self.debuglog = safe_get(env, "debuglog")
        self.keyboard = safe_get(env, "keyboard")
        self.history = []
        self.history_index = -1
        self.cmd_buffer = []
        self.log_lines = self._read_logs()
        self.log_cursor = len(self.log_lines)

    def _write(self, text, newline=True):
        suffix = "\n" if newline else ""
        payload = str(text) + suffix
        writer = safe_get(self.console, "write") if isinstance(self.console, dict) else None
        if callable(writer):
            try:
                writer(payload)
            except Exception:
                pass
        serial_writer = safe_get(self.serial, "write") if isinstance(self.serial, dict) else None
        if callable(serial_writer):
            try:
                serial_writer(payload)
            except Exception:
                pass
        if writer is None and serial_writer is None:
            print(payload, end="")

    def _read_logs(self):
        reader = safe_get(self.debuglog, "buffer") if isinstance(self.debuglog, dict) else None
        if not callable(reader):
            return []
        try:
            raw = reader()
        except Exception:
            return []
        if isinstance(raw, (bytes, bytearray)):
            try:
                return bytes(raw).decode(errors="ignore").splitlines()
            except Exception:
                return []
        return []

    def _render_log_window(self):
        self.log_lines = self._read_logs() or self.log_lines
        if not self.log_lines:
            self._write("[logs] no buffered entries")
            return
        self.log_cursor = min(max(self.log_cursor, 0), len(self.log_lines))
        start = max(0, self.log_cursor - LOG_WINDOW)
        end = min(len(self.log_lines), start + LOG_WINDOW)
        self._write("--- debug log [%d-%d/%d] ---" % (start + 1, end, len(self.log_lines)))
        idx = start
        while idx < end:
            self._write("  " + self.log_lines[idx])
            idx += 1

    def _render_prompt(self):
        self._write(PROMPT + "".join(self.cmd_buffer), newline=True)

    def _recall_history(self, direction):
        if not self.history:
            return
        if self.history_index == -1:
            self.history_index = len(self.history)
        self.history_index = min(max(self.history_index + direction, 0), len(self.history) - 1)
        recalled = self.history[self.history_index]
        self.cmd_buffer = list(recalled)
        self._write("\n[history] " + recalled)
        self._render_prompt()

    def _handle_command(self, command):
        text = command.strip()
        if not text:
            self._render_prompt()
            return
        self.history.append(text)
        self.history_index = -1
        parts = text.split()
        head = parts[0].lower()
        args = parts[1:]

        if head == "help":
            self._write("Commands: help, logs, modules, mem, state, hw, echo, flush, easter")
            self._write("Arrows: Up/Down scroll logs, Left/Right cycle commands")
        elif head == "logs":
            self.log_cursor = len(self.log_lines)
            self._render_log_window()
        elif head == "modules":
            loaded = safe_get(env, "shell_state", {}).get("loaded", ())
            self._write("Loaded modules: " + ", ".join(loaded))
        elif head == "mem":
            heap_fn = safe_get(safe_get(env, "memory"), "heap_free")
            if callable(heap_fn):
                try:
                    self._write("Heap free: " + str(heap_fn()) + " bytes")
                except Exception as exc:
                    self._write("mem error: " + str(exc))
            else:
                self._write("memory stats unavailable")
        elif head == "state":
            runstate = safe_get(env, "runstate", {})
            current = safe_get(runstate, "current_program")
            debug_flag = safe_get(runstate, "is_debug_mode")
            vga_flag = safe_get(runstate, "is_vga_output")
            self._write("program=" + str(current() if callable(current) else "<unknown>") +
                        " debug=" + str(bool(debug_flag() if callable(debug_flag) else False)) +
                        " vga=" + str(bool(vga_flag() if callable(vga_flag) else False)))
        elif head == "hw":
            hw = safe_get(env, "hwinfo", {})
            cpuid_fn = safe_get(hw, "cpuid")
            if callable(cpuid_fn):
                try:
                    self._write("cpuid: " + str(cpuid_fn()))
                except Exception:
                    self._write("cpuid unavailable")
            else:
                self._write("hwinfo unavailable")
        elif head == "flush":
            flusher = safe_get(self.debuglog, "flush") if isinstance(self.debuglog, dict) else None
            if callable(flusher):
                try:
                    flusher()
                    self._write("flushed debug log")
                except Exception as exc:
                    self._write("flush failed: " + str(exc))
            else:
                self._write("no debuglog backend")
        elif head == "echo":
            self._write(" ".join(args))
        elif head == "easter":
            egg = "".join(args) or "starlit sky"
            self._write("[easter] guided by " + egg + " — keep exploring!")
        else:
            self._write("Unknown command '" + head + "'. Type help.")
        self._render_prompt()

    def _read_key(self):
        if self.keyboard is None:
            reader = safe_get(env, "input")
            if callable(reader):
                line = reader(PROMPT)
                return ("line", line)
            return (None, None)

        reader = safe_get(self.keyboard, "read_code")
        if not callable(reader):
            return (None, None)

        code = reader()
        if code == 27:
            second = reader()
            if second == 91:
                third = reader()
                if third == 65:
                    return ("log", -1)
                if third == 66:
                    return ("log", 1)
                if third == 67:
                    return ("history", 1)
                if third == 68:
                    return ("history", -1)
            return (None, None)
        if code in (10, 13):
            return ("enter", None)
        if code in (8, 127):
            return ("backspace", None)
        if code is None:
            return (None, None)
        try:
            if 32 <= int(code) < 127:
                return ("text", chr(int(code)))
        except Exception:
            pass
        return (None, None)

    def run(self):
        self._write("[mgmnt] Debug shell online. Type help for commands.")
        self._render_prompt()
        while True:
            action, payload = self._read_key()
            if action is None:
                continue
            if action == "line":
                self._handle_command(str(payload or ""))
                continue
            if action == "enter":
                command = "".join(self.cmd_buffer)
                self.cmd_buffer = []
                self._write("")
                self._handle_command(command)
                continue
            if action == "backspace":
                if self.cmd_buffer:
                    self.cmd_buffer.pop()
                    self._render_prompt()
                continue
            if action == "text":
                self.cmd_buffer.append(payload)
                self._render_prompt()
                continue
            if action == "log":
                self.log_cursor = min(max(self.log_cursor + payload, 0), len(self.log_lines))
                self._render_log_window()
                self._render_prompt()
                continue
            if action == "history":
                self._recall_history(payload)
                continue


def _start_shell():
    modules = bootstrap()
    env["shell"] = {
        "profile": "management",
        "modules": MANAGEMENT_MODULES,
        "bootstrap": bootstrap,
        "module_list": module_list,
    }
    shell = DebugShell(modules)
    shell.run()


_start_shell()
