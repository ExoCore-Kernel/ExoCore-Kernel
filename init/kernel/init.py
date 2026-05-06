#mpyexo
"""Boot demo: 1920x1080 framebuffer with a movable mouse cursor."""

from env import env, mpyrun

LOG_PREFIX = "[welcome-demo] "
TARGET_WIDTH = 160
TARGET_HEIGHT = 100
MIN_WIDTH = 160
MIN_HEIGHT = 100
FPS = 60
FRAMES = 0


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
        pass
    try:
        return getattr(mapping, key)
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


def _load_console_api():
    mod = mpyrun("consolectl")
    if mod is None:
        raise RuntimeError("consolectl module unavailable")

    console = _safe_get(env, "console")
    if not isinstance(console, dict):
        raise RuntimeError("console API unavailable")

    create = _safe_get(console, "fb_create_bestfit")
    fill_rect = _safe_get(console, "fb_fill_rect")
    present = _safe_get(console, "fb_present_fullscreen")
    if not callable(present):
        present = _safe_get(console, "fb_present")
    sleep_hz = _safe_get(console, "fb_sleep_hz")

    if not callable(create) or not callable(fill_rect) or not callable(present) or not callable(sleep_hz):
        raise RuntimeError("framebuffer helper API unavailable")

    info = _safe_get(console, "framebuffer_info")
    return create, fill_rect, present, sleep_hz, info


def _load_mouse_api():
    mod = mpyrun("mouse")
    init_fn = _safe_get(mod, "init")
    poll_fn = _safe_get(mod, "poll")

    if not callable(init_fn) or not callable(poll_fn):
        import mouse_native
        init_fn = _safe_get(mouse_native, "init")
        poll_fn = _safe_get(mouse_native, "poll")

    if not callable(init_fn) or not callable(poll_fn):
        raise RuntimeError("mouse API unavailable")
    return init_fn, poll_fn


def _clamp(value, low, high):
    if value < low:
        return low
    if value > high:
        return high
    return value


def run_welcome_demo():
    create, fill_rect, present, sleep_hz, framebuffer_info = _load_console_api()
    mouse_init, mouse_poll = _load_mouse_api()

    surface = create(TARGET_WIDTH, TARGET_HEIGHT, MIN_WIDTH, MIN_HEIGHT)
    width = int(surface["width"])
    height = int(surface["height"])

    if not mouse_init():
        raise RuntimeError("mouse initialization failed")

    cursor_x = width // 2
    cursor_y = height // 2
    cursor_size = 12

    fb = framebuffer_info() if callable(framebuffer_info) else {}
    fb_w = int(_safe_get(fb, "width", 0) or 0)
    fb_h = int(_safe_get(fb, "height", 0) or 0)
    log("surface=" + str(width) + "x" + str(height))
    log("display=" + str(fb_w) + "x" + str(fb_h))
    log("mouse-ready=True")

    frame = 0
    while FRAMES <= 0 or frame < FRAMES:
        packet = mouse_poll()
        if packet is not None:
            cursor_x += int(packet[0])
            cursor_y += int(packet[1])
            cursor_x = _clamp(cursor_x, 0, width - 1)
            cursor_y = _clamp(cursor_y, 0, height - 1)

        fill_rect(surface, 0, 0, width, height, 0, 0, 0)
        fill_rect(surface, cursor_x, cursor_y, cursor_size, 2, 255, 255, 255)
        fill_rect(surface, cursor_x, cursor_y, 2, cursor_size, 255, 255, 255)

        ok = present(surface)
        if frame == 0:
            log("fb_present frame0=" + str(bool(ok)))
            log("cursor_start=" + str(cursor_x) + "," + str(cursor_y))

        sleep_hz(FPS)
        frame += 1


try:
    run_welcome_demo()
except Exception as exc:
    log("Welcome demo error: " + repr(exc))
