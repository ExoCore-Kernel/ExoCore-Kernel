#mpyexo
"""ExoDraw-powered init script showcasing the kernel UI.

This script bootstraps a minimal ExoDraw interpreter that renders a
decorative layout using the VGA draw module while emitting detailed debug
logs to the console.  Everything is defined in plain ``.py`` so the kernel's
MicroPython loader can execute it without compiled bytecode.

An interactive prompt is included so the user can trigger multiple ExoDraw
layouts and see how rectangles, lines, color swaps, and staged ``PRESENT``
calls look when layered together.
"""

from env import env, mpyrun


LOG_PREFIX = "[exodraw] "
PIXEL_CHAR = "\xDB"

DEFAULT_SCRIPT = (
    "# ExoDraw Hello splash",
    "DEBUG Loading full-screen ExoDraw showcase",
    "CANVAS fg=BLUE bg=BLUE clear=1",
    "RECT 0 0 80 25 fg=LIGHT_BLUE bg=BLUE fill=0",
    "LINE 0 4 79 4 fg=LIGHT_CYAN bg=BLUE",
    "LINE 0 20 79 20 fg=LIGHT_CYAN bg=BLUE",
    "RECT 3 6 74 13 fg=BLUE bg=BLUE fill=1",
    "TEXT 5 7 \"Canvas + Rect demo\" fg=YELLOW bg=BLUE",
    "TEXT 5 9 \"Line segments draw cyan guides\" fg=WHITE bg=BLUE",
    "TEXT 33 12 \"Hello ExoPort!\" fg=WHITE bg=BLUE",
    "TEXT 5 15 \"Text rendering keeps the blue stage\" fg=WHITE bg=BLUE",
    "TEXT 5 17 \"PRESENT commits the scene to VGA\" fg=WHITE bg=BLUE",
    "PRESENT",
)


SHOWCASE_SCRIPTS = {
    "grid": (
        "# Blueprint grid + info",
        "DEBUG Drawing cyan blueprint grid",
        "CANVAS fg=BLACK bg=BLACK clear=1",
        "RECT 1 1 78 23 fg=CYAN bg=BLACK fill=0",
        "LINE 1 12 78 12 fg=LIGHT_BLUE bg=BLACK",
        "LINE 39 1 39 23 fg=LIGHT_BLUE bg=BLACK",
        "TEXT 3 3 \"Lines mark the live axes\" fg=YELLOW bg=BLACK",
        "TEXT 3 5 \"RECT traces the blueprint frame\" fg=WHITE bg=BLACK",
        "TEXT 3 7 \"Canvas fill keeps the starfield dots\" fg=LIGHT_GREY bg=BLACK",
        "TEXT 3 9 \"Try another scene to swap palettes fast\" fg=LIGHT_GREEN bg=BLACK",
        "PRESENT",
    ),
    "sunset": (
        "# Warm gradient blocks",
        "DEBUG Painting sunset layers",
        "CANVAS fg=RED bg=RED",
        "RECT 0 0 80 8 fg=RED bg=RED fill=1",
        "RECT 0 8 80 8 fg=LIGHT_RED bg=LIGHT_RED fill=1",
        "RECT 0 16 80 9 fg=YELLOW bg=YELLOW fill=1",
        "LINE 0 8 79 8 fg=LIGHT_YELLOW bg=RED",
        "LINE 0 16 79 16 fg=LIGHT_YELLOW bg=LIGHT_RED",
        "TEXT 6 5 \"Layered RECT blocks blend colors\" fg=WHITE bg=RED",
        "TEXT 6 13 \"LINE draws horizon separators\" fg=BLACK bg=LIGHT_RED",
        "TEXT 6 20 \"PRESENT unhide keeps VGA focus\" fg=BLACK bg=YELLOW",
        "PRESENT",
    ),
    "badge": (
        "# Title badge",
        "DEBUG Drawing badge overlay",
        "CANVAS fg=BLUE bg=BLUE",
        "RECT 10 4 60 6 fg=LIGHT_CYAN bg=BLUE fill=0",
        "RECT 14 6 52 3 fg=LIGHT_BLUE bg=LIGHT_BLUE fill=1",
        "TEXT 17 7 \"ExoDraw quick badge\" fg=WHITE bg=LIGHT_BLUE",
        "TEXT 12 12 \"Mix CANVAS, RECT, LINE, TEXT commands\" fg=YELLOW bg=BLUE",
        "LINE 10 14 69 14 fg=LIGHT_GREY bg=BLUE",
        "TEXT 20 16 \"Use PRESENT to commit the buffer\" fg=WHITE bg=BLUE",
        "PRESENT",
    ),
}

EXTRA_SHAPES_SCRIPT = (
    "# Advanced shape demos",
    "DEBUG Drawing circles, ellipses, and polygons",
    "CANVAS fg=BLACK bg=BLACK clear=1",
    "POLY 6 4 20 3 34 6 30 14 12 15 fg=LIGHT_GREEN bg=BLACK fill=1",
    "POLY 45 5 58 5 64 10 56 15 44 12 fg=YELLOW bg=BLACK fill=0",
    "CIRCLE 22 18 5 fg=LIGHT_CYAN bg=BLACK fill=1",
    "CIRCLE 60 18 4 fg=LIGHT_MAGENTA bg=BLACK fill=0",
    "ELLIPSE 16 9 6 3 fg=LIGHT_RED bg=BLACK fill=0",
    "ELLIPSE 62 9 8 4 fg=LIGHT_BLUE bg=BLACK fill=1",
    "TEXT 6 20 \"Extras: polygon fill, circles, ellipses\" fg=WHITE bg=BLACK",
    "TEXT 6 22 \"Press e to return to the menu\" fg=LIGHT_GREY bg=BLACK",
    "PRESENT",
)

DEMO_ENTRIES = (
    {
        "key": "default",
        "title": "Hello splash",
        "category": "VGA basics",
        "script": DEFAULT_SCRIPT,
    },
    {
        "key": "grid",
        "title": "Blueprint grid",
        "category": "VGA basics",
        "script": SHOWCASE_SCRIPTS["grid"],
    },
    {
        "key": "sunset",
        "title": "Sunset layers",
        "category": "VGA basics",
        "script": SHOWCASE_SCRIPTS["sunset"],
    },
    {
        "key": "badge",
        "title": "Badge overlay",
        "category": "VGA basics",
        "script": SHOWCASE_SCRIPTS["badge"],
    },
    {
        "key": "extras",
        "title": "Shape extras",
        "category": "Drawing extras",
        "script": EXTRA_SHAPES_SCRIPT,
    },
    {
        "key": "pixel",
        "title": "Pixel gradient",
        "category": "Pixel surface",
        "script": None,
        "runner": None,
    },
)


def safe_get(mapping, key, default=None):
    """Return mapping[key] or attribute without assuming the mapping type."""

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


class PixelSurface:
    """A lightweight pixel buffer that renders colored blocks to the console.

    The VGA ExoDraw stack uses character cells.  To offer a more OS-like pixel
    view without dropping the VGA demos, this helper paints ANSI backgrounds
    directly to the console.  Resolution and refresh rate are configurable so
    the demo can simulate higher fidelity output while running inside QEMU.
    """

    def __init__(self, console):
        self.console = console if isinstance(console, dict) else None
        self.width = 48
        self.height = 24
        self.refresh_hz = 30
        self._refresh_interval_ms = max(1, 1000 // self.refresh_hz)
        self._supports_color = self._detect_color_support()
        self._writer = safe_get(self.console, "write") if isinstance(self.console, dict) else None
        self._clearer = safe_get(self.console, "clear") if isinstance(self.console, dict) else None

    def _detect_color_support(self):
        flag = safe_get(self.console, "ansi", True) if isinstance(self.console, dict) else True
        return bool(flag)

    def _write(self, text, end=""):
        if callable(self._writer):
            try:
                self._writer(str(text) + end)
                return
            except Exception:
                pass
        print(text, end=end)

    def _clear_screen(self):
        if callable(self._clearer):
            try:
                self._clearer()
                return
            except Exception:
                pass
        self._write("\x1b[2J\x1b[H")

    def set_resolution(self, width, height):
        self.width = max(16, min(int(width), 120))
        self.height = max(9, min(int(height), 80))
        log("Pixel surface resolution set to " + str(self.width) + "x" + str(self.height))

    def set_refresh_rate(self, hz):
        try:
            numeric = int(hz)
        except Exception:
            numeric = int(self.refresh_hz)
        if numeric <= 0:
            numeric = 1
        if numeric > 240:
            numeric = 240
        self.refresh_hz = numeric
        self._refresh_interval_ms = max(1, 1000 // numeric)
        log("Pixel surface refresh set to " + str(self.refresh_hz) + "Hz")

    def _pixel_block(self, r, g, b):
        if self._supports_color:
            return "\x1b[48;2;" + str(r) + ";" + str(g) + ";" + str(b) + "m  \x1b[0m"
        brightness = (int(r) + int(g) + int(b)) // 3
        charset = " .:-=+*#%@"
        index = int(brightness * (len(charset) - 1) / 255)
        return charset[index] + charset[index]

    def draw_frame(self, frame_index):
        rows = []
        for y in range(self.height):
            row = []
            for x in range(self.width):
                r = (x * 4 + frame_index * 9) % 256
                g = (y * 7 + frame_index * 5) % 256
                b = ((x + y) * 3 + frame_index * 11) % 256
                row.append(self._pixel_block(r, g, b))
            rows.append("".join(row))

        self._clear_screen()
        self._write("\n".join(rows) + "\n")
        footer = "Resolution: " + str(self.width) + "x" + str(self.height) + " | Refresh: " + str(self.refresh_hz) + "Hz\n"
        footer += "Press 'e' to exit, 'r' to resize, 'f' to tweak refresh."
        self._write(footer + "\n")

    def wait_for_refresh(self):
        try:
            import time

            sleeper_ms = getattr(time, "sleep_ms", None)
            if callable(sleeper_ms):
                sleeper_ms(self._refresh_interval_ms)
                return
            sleeper = getattr(time, "sleep", None)
            if callable(sleeper):
                seconds = self._refresh_interval_ms // 1000
                sleeper(seconds)
        except Exception:
            pass


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

    def reset(self):
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
                parts = token.split("=", 1)
                key = parts[0]
                value = parts[1] if len(parts) > 1 else ""
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
        except Exception:
            raise ValueError(label + " expects integer")

    def _ensure_session(self):
        if not self.session_active:
            self.vga.start()
            self.session_active = True
            log("started default ExoDraw canvas")

    def _draw_text(self, x, y, text, fg, bg):
        if y < 0 or y >= self.height:
            return
        offset = 0
        for char in text:
            xpos = x + offset
            if xpos < 0 or xpos >= self.width:
                continue
            self.vga.pixel(x=xpos, y=y, char=char, fg=fg, bg=bg)
            offset += 1

    def _draw_circle(self, cx, cy, radius, char, fg, bg, fill):
        circle_fn = getattr(self.vga, "circle", None)
        if callable(circle_fn):
            circle_fn(cx, cy, radius, char=char, fg=fg, bg=bg, fill=fill)
            return

        x = radius
        y = 0
        decision = 1 - radius

        def _span(span_y, span_x):
            self.vga.line(x0=cx - span_x, y0=span_y, x1=cx + span_x, y1=span_y, char=char, fg=fg, bg=bg)

        while x >= y:
            if fill:
                _span(cy + y, x)
                _span(cy - y, x)
                _span(cy + x, y)
                _span(cy - x, y)
            else:
                self.vga.pixel(x=cx + x, y=cy + y, char=char, fg=fg, bg=bg)
                self.vga.pixel(x=cx - x, y=cy + y, char=char, fg=fg, bg=bg)
                self.vga.pixel(x=cx + x, y=cy - y, char=char, fg=fg, bg=bg)
                self.vga.pixel(x=cx - x, y=cy - y, char=char, fg=fg, bg=bg)
                self.vga.pixel(x=cx + y, y=cy + x, char=char, fg=fg, bg=bg)
                self.vga.pixel(x=cx - y, y=cy + x, char=char, fg=fg, bg=bg)
                self.vga.pixel(x=cx + y, y=cy - x, char=char, fg=fg, bg=bg)
                self.vga.pixel(x=cx - y, y=cy - x, char=char, fg=fg, bg=bg)

            y += 1
            if decision <= 0:
                decision += 2 * y + 1
            else:
                x -= 1
                decision += 2 * y - 2 * x + 1

    def _draw_ellipse(self, cx, cy, rx, ry, char, fg, bg, fill):
        ellipse_fn = getattr(self.vga, "ellipse", None)
        if callable(ellipse_fn):
            ellipse_fn(cx, cy, rx, ry, char=char, fg=fg, bg=bg, fill=fill)
            return

        if rx < 0 or ry < 0:
            return

        def _isqrt(value):
            if value <= 0:
                return 0
            x = value
            while True:
                y = (x + value // x) // 2
                if y >= x:
                    return x
                x = y

        if rx == 0 and ry == 0:
            self.vga.pixel(x=cx, y=cy, char=char, fg=fg, bg=bg)
            return

        if rx == 0:
            for y in range(cy - ry, cy + ry + 1):
                self.vga.pixel(x=cx, y=y, char=char, fg=fg, bg=bg)
            return

        if ry == 0:
            if fill:
                self.vga.line(x0=cx - rx, y0=cy, x1=cx + rx, y1=cy, char=char, fg=fg, bg=bg)
            else:
                self.vga.pixel(x=cx - rx, y=cy, char=char, fg=fg, bg=bg)
                self.vga.pixel(x=cx + rx, y=cy, char=char, fg=fg, bg=bg)
            return

        rx2 = rx * rx
        ry2 = ry * ry
        rx2_ry2 = rx2 * ry2

        for dy in range(-ry, ry + 1):
            remaining = rx2_ry2 - (dy * dy) * rx2
            if remaining < 0:
                continue

            span = _isqrt(remaining // ry2)
            y = cy + dy

            if fill:
                self.vga.line(x0=cx - span, y0=y, x1=cx + span, y1=y, char=char, fg=fg, bg=bg)
            else:
                self.vga.pixel(x=cx - span, y=y, char=char, fg=fg, bg=bg)
                if span:
                    self.vga.pixel(x=cx + span, y=y, char=char, fg=fg, bg=bg)

    def _draw_polygon(self, points, char, fg, bg, fill):
        polygon_fn = getattr(self.vga, "polygon", None)
        if callable(polygon_fn):
            polygon_fn(points, char=char, fg=fg, bg=bg, fill=fill)
            return

        total = len(points)
        for idx in range(total):
            x0, y0 = points[idx]
            x1, y1 = points[(idx + 1) % total]
            self.vga.line(x0=x0, y0=y0, x1=x1, y1=y1, char=char, fg=fg, bg=bg)

        if not fill:
            return

        ys = [p[1] for p in points]
        min_y = int(min(ys))
        max_y = int(max(ys))

        for y in range(min_y, max_y + 1):
            intersections = []
            for idx in range(total):
                x0, y0 = points[idx]
                x1, y1 = points[(idx + 1) % total]
                if y0 == y1:
                    continue
                if y < min(y0, y1) or y >= max(y0, y1):
                    continue
                proportion = (y - y0) / float(y1 - y0)
                x_cross = x0 + proportion * (x1 - x0)
                intersections.append(int(round(x_cross)))

            intersections.sort()
            spans = len(intersections)
            step = 0
            while step + 1 < spans:
                x_start = intersections[step]
                x_end = intersections[step + 1]
                self.vga.line(x0=x_start, y0=y, x1=x_end, y1=y, char=char, fg=fg, bg=bg)
                step += 2

    def _handle_debug(self, args, opts, line_no):
        message = " ".join(args) if args else "line " + str(line_no)
        log("DEBUG line " + str(line_no) + ": " + message)

    def _handle_canvas(self, args, opts, line_no):
        fg = self._color_value(opts.get("fg"), "WHITE")
        bg = self._color_value(opts.get("bg"), "BLACK")
        fill = self._char_value(opts.get("fill"), PIXEL_CHAR)
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
        char = self._char_value(opts.get("char"), PIXEL_CHAR)
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
        char = self._char_value(opts.get("char"), PIXEL_CHAR)
        fg = self._color_value(opts.get("fg"), "LIGHT_GREY")
        bg = self._color_value(opts.get("bg"), "BLACK")
        self._ensure_session()
        self.vga.line(x0=x0, y0=y0, x1=x1, y1=y1, char=char, fg=fg, bg=bg)
        log("LINE line " + str(line_no) + " from (" + str(x0) + "," + str(y0) + ") to (" + str(x1) + "," + str(y1) + ")")

    def _handle_circle(self, args, opts, line_no):
        if len(args) < 3:
            raise ValueError("CIRCLE requires cx cy r")
        cx = self._to_int(args[0], "cx")
        cy = self._to_int(args[1], "cy")
        radius = self._to_int(args[2], "r")
        char = self._char_value(opts.get("char"), PIXEL_CHAR)
        fg = self._color_value(opts.get("fg"), "LIGHT_CYAN")
        bg = self._color_value(opts.get("bg"), "BLACK")
        fill = self._flag(opts.get("fill"), False)
        self._ensure_session()
        self._draw_circle(cx, cy, radius, char, fg, bg, fill)
        log("CIRCLE line " + str(line_no) + " center (" + str(cx) + "," + str(cy) + ") r=" + str(radius) + " fill=" + str(fill))

    def _handle_ellipse(self, args, opts, line_no):
        if len(args) < 4:
            raise ValueError("ELLIPSE requires cx cy rx ry")
        cx = self._to_int(args[0], "cx")
        cy = self._to_int(args[1], "cy")
        rx = self._to_int(args[2], "rx")
        ry = self._to_int(args[3], "ry")
        char = self._char_value(opts.get("char"), PIXEL_CHAR)
        fg = self._color_value(opts.get("fg"), "LIGHT_MAGENTA")
        bg = self._color_value(opts.get("bg"), "BLACK")
        fill = self._flag(opts.get("fill"), False)
        self._ensure_session()
        self._draw_ellipse(cx, cy, rx, ry, char, fg, bg, fill)
        log("ELLIPSE line " + str(line_no) + " center (" + str(cx) + "," + str(cy) + ") radii=" + str(rx) + "x" + str(ry) + " fill=" + str(fill))

    def _handle_polygon(self, args, opts, line_no):
        if len(args) < 6 or len(args) % 2 != 0:
            raise ValueError("POLY requires paired x y coordinates")
        points = []
        idx = 0
        total = len(args)
        while idx + 1 < total:
            points.append((self._to_int(args[idx], "x"), self._to_int(args[idx + 1], "y")))
            idx += 2
        char = self._char_value(opts.get("char"), PIXEL_CHAR)
        fg = self._color_value(opts.get("fg"), "LIGHT_GREEN")
        bg = self._color_value(opts.get("bg"), "BLACK")
        fill = self._flag(opts.get("fill"), False)
        self._ensure_session()
        self._draw_polygon(points, char, fg, bg, fill)
        log("POLY line " + str(line_no) + " points=" + str(len(points)) + " fill=" + str(fill))

    def _handle_text(self, args, opts, line_no):
        if len(args) < 3:
            raise ValueError("TEXT requires x y text")
        x = self._to_int(args[0], "x")
        y = self._to_int(args[1], "y")
        text_parts = []
        idx = 2
        total = len(args)
        while idx < total:
            text_parts.append(args[idx])
            idx += 1
        text = " ".join(text_parts)
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
            "CIRCLE": self._handle_circle,
            "ELLIPSE": self._handle_ellipse,
            "POLY": self._handle_polygon,
            "TEXT": self._handle_text,
            "PRESENT": self._handle_present,
        }

        line_no = 0
        for raw in lines:
            line_no += 1
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
            arg_tokens = []
            index = 0
            total_tokens = len(tokens)
            while index < total_tokens:
                if index > 0:
                    arg_tokens.append(tokens[index])
                index += 1
            parsed = self._split_args_opts(arg_tokens)
            args = parsed[0]
            opts = parsed[1]
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


def freeze_vga_console():
    try:
        runstatectl = load_module("runstatectl")
    except Exception as exc:
        log("runstatectl unavailable: " + str(exc))
        return False

    setter = getattr(runstatectl, "set_vga_output", None)
    if not callable(setter):
        log("runstatectl.set_vga_output() missing; cannot preserve ExoDraw frame")
        return False

    try:
        setter(False)
        log("VGA console output disabled to preserve ExoDraw frame")
        return True
    except Exception as exc:
        log("Failed to disable VGA console output: " + str(exc))
        return False


def _draw_menu_vga(entries, index, interpreter):
    colors = interpreter.colors if interpreter.colors is not None else {}
    blue = colors.get("BLUE", 1)
    light_blue = colors.get("LIGHT_BLUE", blue)
    light_cyan = colors.get("LIGHT_CYAN", blue)
    white = colors.get("WHITE", 15)
    yellow = colors.get("YELLOW", white)

    interpreter.reset()
    interpreter.vga.start(fg=white, bg=blue, char=" ", clear=True)
    interpreter.session_active = True

    interpreter.vga.rect(x=0, y=0, w=interpreter.width, h=interpreter.height, char="#", fg=light_blue, bg=blue, fill=False)
    interpreter.vga.line(x0=1, y0=3, x1=interpreter.width - 2, y1=3, char="-", fg=light_cyan, bg=blue)
    interpreter._draw_text(3, 2, "ExoDraw VGA demo menu", yellow, blue)
    interpreter._draw_text(3, interpreter.height - 3, "Use arrows/W-S + Enter. 'e' exits.", white, blue)

    start_y = 5
    idx = 0
    total = len(entries)
    while idx < total:
        entry = entries[idx]
        prefix = "->" if idx == index else "  "
        fg = yellow if idx == index else white
        bg = light_blue if idx == index else blue
        text = prefix + " " + entry["title"] + " [" + entry["category"] + "]"
        interpreter._draw_text(4, start_y + idx * 2, text, fg, bg)
        idx += 1

    interpreter.vga.present(True)
    interpreter.presented = True


def render_menu(entries, index, interpreter):
    log("=== ExoDraw VGA demo menu ===")
    try:
        total = len(entries)
        idx = 0
        while idx < total:
            entry = entries[idx]
            prefix = "->" if idx == index else "  "
            log(prefix + " " + entry["title"] + " [" + entry["category"] + "]")
            idx += 1
    except Exception as exc:
        log("render_menu error: " + repr(exc))
        raise

    try:
        _draw_menu_vga(entries, index, interpreter)
        log("Rendered VGA menu frame")
    except Exception as exc:
        log("render_menu VGA error: " + repr(exc) + " type=" + str(type(exc)))
    log("Use arrow keys/W-S and Enter to run. Press 'e' to exit menu.")


def load_keyboard():
    try:
        module = load_module("keyinput")
    except Exception as exc:
        log("keyinput unavailable: " + str(exc))
        return None

    keyboard_env = safe_get(env, "keyboard")
    if isinstance(keyboard_env, dict):
        return keyboard_env
    return module


def _read_nav_keyboard(keyboard):
    reader = safe_get(keyboard, "read_code")
    if not callable(reader):
        return None
    code = reader()
    if code in (ord("e"), ord("E")):
        return "exit"
    if code in (10, 13):
        return "enter"
    if code in (ord("w"), ord("W")):
        return "up"
    if code in (ord("s"), ord("S")):
        return "down"
    if code == 27:
        second = reader()
        if second == 91:
            third = reader()
            if third == 65:
                return "up"
            if third == 66:
                return "down"
    return None


def _text_reader():
    reader = safe_get(env, "input")
    builtins_obj = None
    try:
        builtins_obj = globals().get("__builtins__")
    except Exception:
        builtins_obj = None
    if reader is None and builtins_obj is not None:
        reader = safe_get(builtins_obj, "input")
    return reader if callable(reader) else None


def _prompt_resolution(reader, current_width, current_height):
    if reader is None:
        return current_width, current_height
    try:
        response = reader("Enter resolution as WIDTHxHEIGHT (blank keeps " + str(current_width) + "x" + str(current_height) + "): ")
    except Exception:
        return current_width, current_height
    if response is None:
        return current_width, current_height
    text = str(response).strip().lower()
    if not text:
        return current_width, current_height
    if "x" not in text:
        log("Resolution format must be WIDTHxHEIGHT; keeping " + str(current_width) + "x" + str(current_height))
        return current_width, current_height
    parts = text.split("x", 1)
    try:
        width = int(parts[0])
        height = int(parts[1])
        return width, height
    except Exception:
        log("Invalid resolution entry '" + text + "'")
        return current_width, current_height


def _prompt_refresh(reader, current_hz):
    if reader is None:
        return current_hz
    try:
        response = reader("Enter refresh rate in Hz (blank keeps " + str(current_hz) + "): ")
    except Exception:
        return current_hz
    if response is None:
        return current_hz
    text = str(response).strip().lower()
    if not text:
        return current_hz
    try:
        hz = int(text)
        return hz
    except Exception:
        log("Invalid refresh entry '" + text + "'")
        return current_hz


def _read_key_char(keyboard):
    reader = safe_get(keyboard, "read_char")
    if not callable(reader):
        return None
    code = reader()
    if code is None:
        return None
    try:
        return chr(code) if isinstance(code, int) else str(code)
    except Exception:
        return None


def _run_pixel_demo(entry, keyboard, reader):
    try:
        console = safe_get(env, "console")
        surface = PixelSurface(console)

        width, height = _prompt_resolution(reader, surface.width, surface.height)
        surface.set_resolution(width, height)
        hz = _prompt_refresh(reader, surface.refresh_hz)
        surface.set_refresh_rate(hz)

        log(
            "Starting pixel demo at "
            + str(surface.width)
            + "x"
            + str(surface.height)
            + " "
            + str(surface.refresh_hz)
            + "Hz"
        )

        frame = 0
        max_frames = 12
        while frame < max_frames:
            surface.draw_frame(frame)
            frame += 1
            surface.wait_for_refresh()
            key = _read_key_char(keyboard)
            if key is None and reader is not None:
                try:
                    text = reader("")
                except Exception:
                    text = None
                key = str(text)[0] if text else None
            if key is None:
                continue
            lowered = str(key).lower()
            if lowered == "e":
                log("Pixel demo exited by user")
                return
            if lowered == "r":
                width, height = _prompt_resolution(reader, surface.width, surface.height)
                surface.set_resolution(width, height)
                continue
            if lowered == "f":
                hz = _prompt_refresh(reader, surface.refresh_hz)
                surface.set_refresh_rate(hz)
                continue

        log("Pixel demo finished after " + str(max_frames) + " frames")
    except Exception as exc:
        log("Pixel demo error: " + repr(exc))
        log("Returning to menu after pixel demo failure")


for _entry in DEMO_ENTRIES:
    if _entry.get("key") == "pixel":
        _entry["runner"] = _run_pixel_demo


def _run_demo(entry, interpreter, keyboard, reader):
    runner = entry.get("runner")
    if callable(runner):
        runner(entry, keyboard, reader)
        return

    interpreter.reset()
    interpreter.run(entry["script"])
    log("Demo '" + entry["title"] + "' finished. Press 'e' to return or Enter to replay.")

    while True:
        if keyboard is not None:
            nav = _read_nav_keyboard(keyboard)
        else:
            nav = None
        if nav is None and reader is not None:
            try:
                response = reader("Press Enter to replay or e to exit: ")
            except Exception:
                response = "e"
            if response is None:
                response = ""
            nav = "exit" if str(response).strip().lower().startswith("e") else "enter"

        if nav is None:
            continue
        if nav == "exit":
            return
        if nav == "enter":
            interpreter.reset()
            interpreter.run(entry["script"])
            log("Replayed '" + entry["title"] + "'. Press 'e' to return or Enter to replay.")


def _menu_loop(entries, interpreter, keyboard, reader):
    index = 0
    try:
        render_menu(entries, index, interpreter)
    except Exception as exc:
        log("Menu render failed: " + repr(exc))
        raise
    while True:
        nav = _read_nav_keyboard(keyboard) if keyboard is not None else None
        if nav is None and reader is not None:
            prompt = "Select demo number (1-" + str(len(entries)) + ") or e to exit: "
            try:
                selection = reader(prompt)
            except Exception as exc:
                log("Input failed: " + str(exc))
                return
            if selection is None:
                selection = ""
            text = str(selection).strip().lower()
            if not text:
                nav = "exit"
            elif text == "e":
                nav = "exit"
            else:
                try:
                    idx = int(text) - 1
                    if 0 <= idx < len(entries):
                        index = idx
                        nav = "enter"
                    else:
                        log("Pick between 1 and " + str(len(entries)))
                        render_menu(entries, index, interpreter)
                        continue
                except Exception:
                    log("Unknown selection '" + text + "'")
                    render_menu(entries, index, interpreter)
                    continue

        if nav is None:
            continue
        if nav == "exit":
            log("Interactive session complete")
            return
        if nav == "up":
            index = (index - 1) % len(entries)
            render_menu(entries, index, interpreter)
            continue
        if nav == "down":
            index = (index + 1) % len(entries)
            render_menu(entries, index, interpreter)
            continue
        if nav == "enter":
            entry = entries[index]
            log("Rendering demo '" + entry["key"] + "'")
            _run_demo(entry, interpreter, keyboard, reader)
            render_menu(entries, index, interpreter)


def interactive_showcase(interpreter):
    keyboard = load_keyboard()
    reader = _text_reader()
    if keyboard is None and reader is None:
        log("Interactive input unavailable: missing keyboard and input() support")
        log("Running fallback 'grid' showcase")
        interpreter.reset()
        interpreter.run(SHOWCASE_SCRIPTS["grid"])
        return

    _menu_loop(DEMO_ENTRIES, interpreter, keyboard, reader)


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
    interactive_showcase(interpreter)
    if not freeze_vga_console():
        log("ExoDraw UI demo complete (console not frozen)")
    else:
        log("ExoDraw UI demo complete")


main()
