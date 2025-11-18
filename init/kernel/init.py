#mpyexo
"""ExoDraw-powered init script showcasing the kernel UI.

This script bootstraps a minimal ExoDraw interpreter that renders a
decorative layout using the VGA draw module while emitting detailed debug
logs to the console.  Everything is defined in plain ``.py`` so the kernel's
MicroPython loader can execute it without compiled bytecode.
"""

from env import env, mpyrun


LOG_PREFIX = "[exodraw] "

DEFAULT_SCRIPT = (
    "# ExoDraw UI demo program",
    "DEBUG Loading ExoDraw scripted layout",
    "CANVAS fg=LIGHT_CYAN bg=BLACK fill= ",
    "RECT 0 0 80 25 char=# fg=LIGHT_BLUE bg=BLACK fill=0",
    "RECT 2 1 76 3 char=  fg=BLACK bg=BLUE fill=1",
    "TEXT 4 2 \"ExoCore Kernel UI Demo (ExoDraw)\" fg=YELLOW bg=BLUE",
    "LINE 2 5 77 5 char=- fg=LIGHT_GREY bg=BLACK",
    "TEXT 4 4 \"Widgets\" fg=LIGHT_GREEN bg=BLACK",
    "RECT 4 7 20 8 char=. fg=LIGHT_BLUE bg=LIGHT_BLUE fill=1",
    "TEXT 6 8 \"Window A\" fg=BLACK bg=LIGHT_BLUE",
    "RECT 28 7 20 8 char=. fg=LIGHT_GREEN bg=LIGHT_GREEN fill=1",
    "TEXT 30 8 \"Window B\" fg=BLACK bg=LIGHT_GREEN",
    "RECT 52 7 20 8 char=. fg=LIGHT_RED bg=LIGHT_RED fill=1",
    "TEXT 54 8 \"Console\" fg=BLACK bg=LIGHT_RED",
    "TEXT 4 17 \"Status: UI ready\" fg=YELLOW bg=BLACK",
    "TEXT 4 18 \"VGA output shows layered panels\" fg=LIGHT_GREY bg=BLACK",
    "TEXT 4 19 \"Console logs mirror drawing steps\" fg=LIGHT_GREY bg=BLACK",
    "PRESENT",
)


def safe_get(mapping, key, default=None):
    """Return mapping[key] without assuming the mapping type."""

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
        return default


def log(message):
    text = LOG_PREFIX + str(message)
    console = safe_get(env, "console")
    writer = safe_get(console, "write") if isinstance(console, dict) else None
    if callable(writer):
        try:
            writer(text + "\n")
        except Exception:
            pass
    print(text)


def load_module(name):
    module = mpyrun(name)
    if module is None:
        raise RuntimeError("module " + str(name) + " unavailable")
    log("loaded " + str(name) + " module")
    return module


class ExoDrawInterpreter:
    def __init__(self, vga_draw_module):
        self.vga = vga_draw_module
        self.colors = getattr(vga_draw_module, "COLORS", {})
        self.width = getattr(vga_draw_module, "WIDTH", 80)
        self.height = getattr(vga_draw_module, "HEIGHT", 25)
        self.session_active = False
        self.presented = False

    def _tokenize(self, line):
        tokens = []
        current = []
        in_quote = False
        for char in line:
            if char == '"':
                in_quote = not in_quote
                continue
            if char.isspace() and not in_quote:
                if current:
                    tokens.append("".join(current))
                    current = []
            else:
                current.append(char)
        if current:
            tokens.append("".join(current))
        return tokens

    @staticmethod
    def _split_args_opts(tokens):
        args = []
        opts = {}
        for token in tokens:
            if "=" in token:
                key, value = token.split("=", 1)
                opts[key.lower()] = value
            else:
                args.append(token)
        return args, opts

    def _color_value(self, token, fallback):
        if token is None:
            token = fallback
        if isinstance(token, int):
            return token
        if token is None:
            return self.colors.get("WHITE", 15)
        name = str(token).upper()
        if name in self.colors:
            return self.colors[name]
        try:
            return int(token)
        except Exception:
            return self.colors.get("WHITE", 15)

    @staticmethod
    def _flag(value, default=False):
        if value is None:
            return default
        text = str(value).lower()
        return text not in ("0", "false", "no", "off")

    @staticmethod
    def _char_value(value, default):
        if value is None:
            return default
        string = str(value)
        if not string:
            return default
        return string[0]

    @staticmethod
    def _to_int(value, label):
        try:
            return int(value)
        except Exception as exc:
            raise ValueError(label + " expects integer") from exc

    def _ensure_session(self):
        if not self.session_active:
            self.vga.start()
            self.session_active = True
            log("started default ExoDraw canvas")

    def _draw_text(self, x, y, text, fg, bg):
        if y < 0 or y >= self.height:
            return
        for offset, char in enumerate(text):
            xpos = x + offset
            if xpos < 0 or xpos >= self.width:
                continue
            self.vga.pixel(x=xpos, y=y, char=char, fg=fg, bg=bg)

    def _handle_debug(self, args, opts, line_no):
        message = " ".join(args) if args else "line " + str(line_no)
        log("DEBUG line " + str(line_no) + ": " + message)

    def _handle_canvas(self, args, opts, line_no):
        fg = self._color_value(opts.get("fg"), "WHITE")
        bg = self._color_value(opts.get("bg"), "BLACK")
        fill = self._char_value(opts.get("fill"), " ")
        clear = self._flag(opts.get("clear"), True)
        self.vga.start(fg=fg, bg=bg, char=fill, clear=clear)
        self.session_active = True
        log("Canvas ready at " + str(self.width) + "x" + str(self.height) + " fg=" + str(fg) + " bg=" + str(bg))

    def _handle_rect(self, args, opts, line_no):
        if len(args) < 4:
            raise ValueError("RECT requires x y w h")
        x = self._to_int(args[0], "x")
        y = self._to_int(args[1], "y")
        w = self._to_int(args[2], "w")
        h = self._to_int(args[3], "h")
        char = self._char_value(opts.get("char"), "#")
        fg = self._color_value(opts.get("fg"), "LIGHT_CYAN")
        bg = self._color_value(opts.get("bg"), "BLACK")
        fill = self._flag(opts.get("fill"), False)
        self._ensure_session()
        self.vga.rect(x=x, y=y, w=w, h=h, char=char, fg=fg, bg=bg, fill=fill)
        log("RECT line " + str(line_no) + " at (" + str(x) + "," + str(y) + ") size " + str(w) + "x" + str(h) + " fill=" + str(fill))

    def _handle_line(self, args, opts, line_no):
        if len(args) < 4:
            raise ValueError("LINE requires x0 y0 x1 y1")
        x0 = self._to_int(args[0], "x0")
        y0 = self._to_int(args[1], "y0")
        x1 = self._to_int(args[2], "x1")
        y1 = self._to_int(args[3], "y1")
        char = self._char_value(opts.get("char"), "-")
        fg = self._color_value(opts.get("fg"), "LIGHT_GREY")
        bg = self._color_value(opts.get("bg"), "BLACK")
        self._ensure_session()
        self.vga.line(x0=x0, y0=y0, x1=x1, y1=y1, char=char, fg=fg, bg=bg)
        log("LINE line " + str(line_no) + " from (" + str(x0) + "," + str(y0) + ") to (" + str(x1) + "," + str(y1) + ")")

    def _handle_text(self, args, opts, line_no):
        if len(args) < 3:
            raise ValueError("TEXT requires x y text")
        x = self._to_int(args[0], "x")
        y = self._to_int(args[1], "y")
        text = " ".join(args[2:])
        fg = self._color_value(opts.get("fg"), "WHITE")
        bg = self._color_value(opts.get("bg"), "BLACK")
        self._ensure_session()
        self._draw_text(x, y, text, fg, bg)
        log("TEXT line " + str(line_no) + " at (" + str(x) + "," + str(y) + ") -> " + text)

    def _handle_present(self, args, opts, line_no):
        unhide = self._flag(opts.get("unhide"), True)
        self._ensure_session()
        if unhide:
            self.vga.present(True)
            self.session_active = False
        else:
            self.vga.present()
        self.presented = True
        log("PRESENT line " + str(line_no) + " unhide=" + str(unhide))

    def run(self, lines):
        handlers = {
            "DEBUG": self._handle_debug,
            "CANVAS": self._handle_canvas,
            "RECT": self._handle_rect,
            "LINE": self._handle_line,
            "TEXT": self._handle_text,
            "PRESENT": self._handle_present,
        }

        for line_no, raw in enumerate(lines, start=1):
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            tokens = self._tokenize(line)
            if not tokens:
                continue
            command = tokens[0].upper()
            handler = handlers.get(command)
            if handler is None:
                log("Skipping unknown command on line " + str(line_no) + ": " + command)
                continue
            args, opts = self._split_args_opts(tokens[1:])
            try:
                handler(args, opts, line_no)
            except Exception as exc:
                log("Error on line " + str(line_no) + ": " + str(exc))

        if self.session_active and not self.presented:
            self.vga.present(True)
            log("Auto-presented final frame")


def enable_vga_output():
    vga_module = load_module("vga")
    enable_fn = getattr(vga_module, "enable", None)
    if callable(enable_fn):
        enable_fn(True)
        log("VGA output enabled")
    else:
        log("VGA module missing enable(); assuming VGA already active")


def main():
    log("Starting ExoDraw kernel init demo")
    console = safe_get(env, "console")
    clearer = safe_get(console, "clear")
    if callable(clearer):
        try:
            clearer()
        except Exception:
            pass

    enable_vga_output()
    vga_draw_module = load_module("vga_draw")
    interpreter = ExoDrawInterpreter(vga_draw_module)
    interpreter.run(DEFAULT_SCRIPT)
    log("ExoDraw UI demo complete")


main()
