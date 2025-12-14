#mpyexo
"""Pixel-focused init script for ExoCore.

This script boots straight into a pixel-surface demo so the kernel can
showcase RGB rendering without relying on the VGA character grid.  The
logic stays in plain ``.py`` files so the MicroPython loader accepts it
without compiled headers.
"""

from env import env, mpyrun


def _ensure_boot_globals():
    global name
    try:
        name
    except NameError:
        name = "exodraw"
    if "__name__" not in globals() or globals().get("__name__") is None:
        globals()["__name__"] = "exodraw"
    if globals().get("env") is None:
        globals()["env"] = env if "env" in globals() else {}
    return name


# Safeguard against NameError from missing globals in embedded MicroPython
name = _ensure_boot_globals()


LOG_PREFIX = "[pixel-demo] "
DEFAULT_SCRIPT = (
    "# ExoDraw pixel surface showcase",
    "PIXELCANVAS 80 60 refresh=30",
    "PIXELPATTERN plasma",
    "PIXELFRAMES 30",
)


class PixelMemoryManager:
    # MicroPython-friendly buffer manager for pixel rows.

    def __init__(self):
        self.width = 0
        self.height = 0
        self._row_cache = []
        self._frames_since_collect = 0
        self._collect_interval = 25
        try:
            import gc
            self._gc = gc
        except Exception:
            self._gc = None

    def configure(self, width, height):
        try:
            self.width = int(width)
        except Exception:
            pass
        try:
            self.height = int(height)
        except Exception:
            pass
        needed = self.width
        if needed < 0:
            needed = 0
        if needed == 0:
            self._row_cache = []
        elif len(self._row_cache) != needed:
            self._row_cache = [""] * needed

    def render_row(self, surface, frame_index, y):
        self.configure(surface.width, surface.height)
        width = surface.width
        if width <= 0:
            return ""
        row = self._row_cache
        idx = 0
        while idx < width:
            r, g, b = surface._pixel_color(frame_index, idx, y)
            row[idx] = "\x1b[48;2;" + str(r) + ";" + str(g) + ";" + str(b) + "m "
            idx += 1
        return "".join(row)

    def after_frame(self):
        self._frames_since_collect += 1
        if self._gc is None:
            return
        if self._frames_since_collect < self._collect_interval:
            return
        try:
            self._gc.collect()
        except Exception:
            pass
        self._frames_since_collect = 0


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


def log(message):
    text = LOG_PREFIX + str(message)
    try:
        console = safe_get(env, "console")
    except Exception:
        console = None
    writer = safe_get(console, "write") if isinstance(console, dict) else None
    if callable(writer):
        try:
            writer(text + "\n")
            return
        except Exception:
            pass
    print(text)


def load_module(name):
    module = mpyrun(name)
    if module is None:
        raise RuntimeError("module " + str(name) + " unavailable")
    log("loaded " + str(name) + " module")
    return module


class PixelSurface:
    """Pixel-accurate surface that paints RGB pixels instead of ASCII cells."""

    def __init__(self, console):
        try:
            self.console = console if isinstance(console, dict) else {}
            log("PixelSurface init stage: console prepared")
            self.width = 80
            self.height = 60
            log("PixelSurface init stage: dimensions set")
            self.refresh_hz = 30
            self._refresh_interval_ms = 33
            self._writer = safe_get(self.console, "write")
            log("PixelSurface init stage: writer fetched")
            self._clearer = safe_get(self.console, "clear")
            self._blitter = safe_get(self.console, "blit_pixels")
            self._supports_color = bool(safe_get(self.console, "ansi", True))
            log("PixelSurface init stage: console helpers ready")
            self._frame_index = 0
            self._pattern = "gradient"
            self._memory = PixelMemoryManager()
            self._memory.configure(self.width, self.height)
            log("PixelSurface init stage: ready without frame buffer")
        except Exception as exc:
            log("PixelSurface init failure: " + repr(exc))
            try:
                details = getattr(exc, "args", None)
                log("PixelSurface exc args: " + str(details))
            except Exception:
                pass
            try:
                import sys

                sys.print_exception(exc)
            except Exception:
                pass
            raise

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
        try:
            parsed_width = int(width)
        except Exception:
            parsed_width = self.width
        try:
            parsed_height = int(height)
        except Exception:
            parsed_height = self.height

        if parsed_width < 16:
            parsed_width = 16
        if parsed_width > 200:
            parsed_width = 200
        if parsed_height < 12:
            parsed_height = 12
        if parsed_height > 150:
            parsed_height = 150

        self.width = parsed_width
        self.height = parsed_height
        self._memory.configure(self.width, self.height)
        log("Pixel surface resolution set to " + str(self.width) + "x" + str(self.height))

    def set_refresh_rate(self, hz):
        try:
            numeric = int(hz)
        except Exception:
            numeric = int(self.refresh_hz)
        if numeric <= 0:
            numeric = 1
        if numeric > 360:
            numeric = 360
        self.refresh_hz = numeric
        try:
            interval_ms = 1000 / float(numeric)
        except Exception:
            interval_ms = 1000 // numeric
        self._refresh_interval_ms = max(1, int(interval_ms))
        log("Pixel surface refresh set to " + str(self.refresh_hz) + "Hz")

    def set_pattern(self, pattern):
        text = str(pattern).strip().lower() if pattern is not None else ""
        if text in ("", "gradient"):
            self._pattern = "gradient"
        elif text in ("plasma", "wave"):
            self._pattern = "plasma"
        else:
            self._pattern = "grid"
        log("Pixel surface pattern set to " + self._pattern)

    def _pixel_color(self, frame_index, x, y):
        if self._pattern == "gradient":
            r = (x * 5 + frame_index * 7) % 256
            g = (y * 6 + frame_index * 5) % 256
            b = ((x + y) * 3 + frame_index * 11) % 256
        elif self._pattern == "plasma":
            half_w = self.width // 2
            half_h = self.height // 2
            dx = x - half_w
            dy = y - half_h
            r = (frame_index * 9 + x * y) % 256
            g = (128 + (dx * dx + dy * dy) // 3 + frame_index * 3) % 256
            b = (frame_index * 5 + x * 2 + y * 3) % 256
        else:
            toggle = ((x // 8) + (y // 8) + frame_index // 2) % 2
            color = 220 if toggle else 35
            r = g = b = color
        return r, g, b

    def draw_frame(self, frame_index):
        self._frame_index = frame_index

    def _present_with_blitter(self):
        return False

    def _present_ansi(self):
        if not self._supports_color:
            return False
        self._clear_screen()
        frame = self._frame_index
        for y in range(self.height):
            row_text = self._memory.render_row(self, frame, y)
            self._write(row_text + "\x1b[0m\n")
        footer = (
            "Pixel mode "
            + str(self.width)
            + "x"
            + str(self.height)
            + " @ "
            + str(self.refresh_hz)
            + "Hz ("
            + self._pattern
            + ")"
        )
        self._write(footer + "\n")
        self._memory.after_frame()
        return True

    def present(self):
        if self._present_with_blitter():
            return
        self._present_ansi()

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
                if seconds <= 0:
                    seconds = 1
                sleeper(seconds)
        except Exception:
            pass


class PixelScene:
    """Parse and execute pixel-mode scripts embedded in ExoDraw demos."""

    def __init__(self, lines):
        self.lines = lines or DEFAULT_SCRIPT
        self.width = 80
        self.height = 60
        self.refresh = 30
        self.frames = 30
        self.pattern = "gradient"

    def _parse_tokens(self, line):
        tokens = []
        current = []
        for char in line:
            if char.isspace():
                if current:
                    tokens.append("".join(current))
                    current = []
            else:
                current.append(char)
        if current:
            tokens.append("".join(current))
        return tokens

    def parse(self):
        for raw in self.lines:
            text = str(raw).strip()
            if not text or text.startswith("#"):
                continue
            tokens = self._parse_tokens(text)
            if not tokens:
                continue
            command = tokens[0].upper()
            if command == "PIXELCANVAS" and len(tokens) >= 3:
                try:
                    self.width = int(tokens[1])
                    self.height = int(tokens[2])
                except Exception:
                    continue
                idx = 3
                total = len(tokens)
                while idx < total:
                    token = tokens[idx]
                    if token.startswith("refresh="):
                        try:
                            self.refresh = int(token.split("=", 1)[1])
                        except Exception:
                            pass
                    idx += 1
            elif command == "PIXELREFRESH" and len(tokens) >= 2:
                try:
                    self.refresh = int(tokens[1])
                except Exception:
                    pass
            elif command == "PIXELFRAMES" and len(tokens) >= 2:
                try:
                    self.frames = max(1, int(tokens[1]))
                except Exception:
                    pass
            elif command == "PIXELPATTERN" and len(tokens) >= 2:
                self.pattern = tokens[1]

    def run(self, surface, keyboard=None, reader=None):
        log(
            "PixelScene run starting width="
            + str(self.width)
            + " height="
            + str(self.height)
            + " refresh="
            + str(self.refresh)
            + " frames="
            + str(self.frames)
        )
        surface.set_resolution(self.width, self.height)
        surface.set_refresh_rate(self.refresh)
        surface.set_pattern(self.pattern)

        frame = 0
        try:
            log("PixelScene entering frame loop")
            while frame < self.frames:
                log("Rendering frame " + str(frame))
                surface.draw_frame(frame)
                surface.present()
                surface.wait_for_refresh()
                frame += 1
        except Exception as exc:
            log("Pixel frame error: " + repr(exc))
            try:
                import sys

                sys.print_exception(exc)
            except Exception:
                pass
            raise
        log("Pixel scene rendered " + str(self.frames) + " frames")


def run_pixel_showcase():
    try:
        log("Preparing console for pixel mode")
        console = safe_get(env, "console")
        clearer = safe_get(console, "clear")
        if callable(clearer):
            try:
                clearer()
            except Exception:
                pass
        log("Creating pixel surface")
        surface = PixelSurface(console)
        log("Parsing pixel script")
        scene = PixelScene(DEFAULT_SCRIPT)
        scene.parse()
        log(
            "Starting pixel demo at "
            + str(scene.width)
            + "x"
            + str(scene.height)
            + " "
            + str(scene.refresh)
            + "Hz pattern="
            + scene.pattern
        )
        surface.set_resolution(scene.width, scene.height)
        try:
            surface.set_refresh_rate(scene.refresh)
        except NameError as name_exc:
            log("NameError while setting refresh; keeping default: " + repr(name_exc))
        try:
            surface.set_pattern(scene.pattern)
        except NameError as name_exc:
            log("NameError while setting pattern; using gradient: " + repr(name_exc))
            surface.set_pattern("gradient")

        try:
            frame = 0
            log("PixelScene entering direct frame loop")
            while frame < scene.frames:
                log("Rendering frame " + str(frame))
                surface.draw_frame(frame)
                surface.present()
                surface.wait_for_refresh()
                frame += 1
        except NameError as name_exc:
            log("NameError inside pixel loop; showing static gradient: " + repr(name_exc))
            try:
                surface.set_pattern("gradient")
                surface.draw_frame(0)
                surface.present()
            except Exception:
                pass
            return
        except Exception as loop_exc:
            log("Direct pixel loop error: " + repr(loop_exc))
            try:
                import sys

                sys.print_exception(loop_exc)
            except Exception:
                pass
            raise
    except Exception as exc:
        log("Pixel demo error: " + repr(exc))
        try:
            log("Pixel demo error type: " + str(getattr(type(exc), "__name__", "?")))
            log("Pixel demo error args: " + str(getattr(exc, "args", None)))
            log("Pixel demo globals: " + str(list(globals().keys())))
        except Exception:
            pass
        try:
            import sys

            sys.print_exception(exc)
            log("Exception above printed via sys.print_exception")
        except Exception as log_exc:
            log("Failed to print exception: " + repr(log_exc))
        log("Falling back to static gradient")
        try:
            fallback_surface = PixelSurface(safe_get(env, "console"))
            fallback_surface.draw_frame(0)
            fallback_surface.present()
        except Exception as fallback_exc:
            log("Fallback gradient failed as well: " + repr(fallback_exc))
            try:
                import sys

                sys.print_exception(fallback_exc)
            except Exception:
                pass


def main():
    log("Starting pixel-only init demo")
    run_pixel_showcase()
    log("Pixel init demo complete")


try:
    _ensure_boot_globals()
    main()
except NameError as boot_exc:
    log("Recovered from NameError during ExoDraw boot: " + repr(boot_exc))
    _ensure_boot_globals()
    try:
        main()
    except Exception as retry_exc:
        log("Retrying ExoDraw boot failed: " + repr(retry_exc))
