#mpyexo
"""Minimal pixel init demo using consolectl framebuffer helpers."""

from env import env, mpyrun

LOG_PREFIX = "[pixel-demo] "
WIDTH = 320
HEIGHT = 200
FPS = 30
FRAMES = 120


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


def _safe_get(mapping, key, default=None):
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
    serial = _safe_get(env, "serial")
    writer = _safe_get(serial, "write") if isinstance(serial, dict) else None
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


def _color(frame, x, y):
    r = (x * 3 + frame * 7) % 256
    g = (y * 4 + frame * 5) % 256
    b = ((x + y) * 2 + frame * 11) % 256
    return r, g, b


def _draw_frame(console, surface, frame):
    fill_rect = console['fb_fill_rect']
    set_px = console['fb_set_pixel']

    fill_rect(surface, 0, 0, surface['width'], surface['height'], 0, 0, 0)

    y = 0
    while y < surface['height']:
        x = 0
        while x < surface['width']:
            r, g, b = _color(frame, x, y)
            set_px(surface, x, y, r, g, b)
            x += 4
        y += 2


def run_pixel_demo():
    load_module("consolectl")
    # consolectl_native is a built-in C module, so loading it via mpyrun()
    # can fail even though `import consolectl_native` works inside consolectl.

    console = _safe_get(env, "console")
    if not isinstance(console, dict):
        raise RuntimeError("console API unavailable")

    create = _safe_get(console, 'fb_create_bestfit')
    present = _safe_get(console, 'fb_present_fullscreen')
    if not callable(present):
        present = _safe_get(console, 'fb_present')
    sleep_hz = _safe_get(console, 'fb_sleep_hz')
    if not callable(create) or not callable(present) or not callable(sleep_hz):
        raise RuntimeError("framebuffer helper API unavailable")

    surface = create(WIDTH, HEIGHT, 64, 48)
    log("surface=" + str(surface['width']) + "x" + str(surface['height']))
    frame = 0
    while frame < FRAMES:
        _draw_frame(console, surface, frame)
        ok = present(surface)
        if frame == 0:
            log("fb_present frame0=" + str(bool(ok)))
        sleep_hz(FPS)
        frame += 1


def main():
    log("Starting pixel helper API demo")
    run_pixel_demo()
    log("Pixel init demo complete")


try:
    _ensure_boot_globals()
    main()
except Exception as exc:
    log("Pixel demo error: " + repr(exc))
