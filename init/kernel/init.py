#mpyexo
"""Boot welcome demo: centered text with colorful moving shapes."""

from env import env, mpyrun

LOG_PREFIX = "[welcome-demo] "
TARGET_WIDTH = 320
TARGET_HEIGHT = 200
MIN_WIDTH = 160
MIN_HEIGHT = 100
FPS = 30
FRAMES = 240
WELCOME_TEXT = "Welcome to ExoCore!"
PIXEL_SIZE = 2
CHAR_SPACING = 1

GLYPHS = {
    ' ': ["00000", "00000", "00000", "00000", "00000", "00000", "00000"],
    '!': ["00100", "00100", "00100", "00100", "00100", "00000", "00100"],
    'W': ["10001", "10001", "10001", "10101", "10101", "10101", "01010"],
    'E': ["11111", "10000", "10000", "11110", "10000", "10000", "11111"],
    'C': ["01111", "10000", "10000", "10000", "10000", "10000", "01111"],
    'e': ["00000", "01110", "10001", "11111", "10000", "10001", "01110"],
    'l': ["00110", "00100", "00100", "00100", "00100", "00100", "00111"],
    'c': ["00000", "01110", "10001", "10000", "10000", "10001", "01110"],
    'o': ["00000", "01110", "10001", "10001", "10001", "10001", "01110"],
    'm': ["00000", "11010", "10101", "10101", "10101", "10101", "10101"],
    't': ["00100", "00100", "11111", "00100", "00100", "00100", "00011"],
    'x': ["00000", "10001", "01010", "00100", "00100", "01010", "10001"],
    'r': ["00000", "10110", "11001", "10000", "10000", "10000", "10000"],
}


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

    return create, fill_rect, present, sleep_hz


def _draw_char(surface, fill_rect, ch, x, y, color):
    glyph = GLYPHS.get(ch)
    if glyph is None:
        glyph = GLYPHS["e"]

    yy = 0
    while yy < 7:
        row = glyph[yy]
        xx = 0
        while xx < 5:
            if row[xx] == "1":
                fill_rect(surface, x + xx * PIXEL_SIZE, y + yy * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, color[0], color[1], color[2])
            xx += 1
        yy += 1


def _draw_text_centered(surface, fill_rect, text, color, box_y, box_h):
    char_w = 5 * PIXEL_SIZE
    spacing = CHAR_SPACING * PIXEL_SIZE
    text_w = len(text) * char_w + (len(text) - 1) * spacing
    text_h = 7 * PIXEL_SIZE

    start_x = (surface["width"] - text_w) // 2
    start_y = box_y + (box_h - text_h) // 2

    i = 0
    while i < len(text):
        char_x = start_x + i * (char_w + spacing)
        _draw_char(surface, fill_rect, text[i], char_x, start_y, color)
        i += 1


def _draw_shape(surface, fill_rect, shape):
    fill_rect(surface, shape["x"], shape["y"], shape["w"], shape["h"], shape["color"][0], shape["color"][1], shape["color"][2])


def _update_shape(shape, max_w, max_h, avoid_box):
    shape["x"] += shape["dx"]
    shape["y"] += shape["dy"]

    if shape["x"] < 0:
        shape["x"] = 0
        shape["dx"] = -shape["dx"]
    if shape["y"] < 0:
        shape["y"] = 0
        shape["dy"] = -shape["dy"]

    if shape["x"] + shape["w"] > max_w:
        shape["x"] = max_w - shape["w"]
        shape["dx"] = -shape["dx"]
    if shape["y"] + shape["h"] > max_h:
        shape["y"] = max_h - shape["h"]
        shape["dy"] = -shape["dy"]

    x0, y0, x1, y1 = avoid_box
    overlap = not (
        (shape["x"] + shape["w"]) < x0 or
        shape["x"] > x1 or
        (shape["y"] + shape["h"]) < y0 or
        shape["y"] > y1
    )
    if overlap:
        shape["x"] -= shape["dx"]
        shape["y"] -= shape["dy"]
        shape["dx"] = -shape["dx"]
        shape["dy"] = -shape["dy"]


def run_welcome_demo():
    create, fill_rect, present, sleep_hz = _load_console_api()
    surface = create(TARGET_WIDTH, TARGET_HEIGHT, MIN_WIDTH, MIN_HEIGHT)

    width = int(surface["width"])
    height = int(surface["height"])
    log("surface=" + str(width) + "x" + str(height))

    title_box_h = 26
    title_box_y = (height - title_box_h) // 2
    title_box_w = width - 40
    title_box_x = (width - title_box_w) // 2

    avoid_box = (
        title_box_x - 6,
        title_box_y - 6,
        title_box_x + title_box_w + 6,
        title_box_y + title_box_h + 6,
    )

    shapes = [
        {"x": 6, "y": 8, "w": 40, "h": 24, "dx": 2, "dy": 1, "color": (255, 50, 50)},
        {"x": width - 52, "y": 12, "w": 32, "h": 16, "dx": -1, "dy": 2, "color": (60, 220, 70)},
        {"x": 12, "y": height - 30, "w": 28, "h": 18, "dx": 2, "dy": -2, "color": (255, 220, 40)},
        {"x": width - 64, "y": height - 34, "w": 46, "h": 22, "dx": -2, "dy": -1, "color": (70, 180, 255)},
        {"x": width // 2 - 8, "y": 4, "w": 16, "h": 16, "dx": 1, "dy": 1, "color": (240, 80, 255)},
    ]

    frame = 0
    while frame < FRAMES:
        fill_rect(surface, 0, 0, width, height, 10, 14, 25)

        dot = 0
        while dot < width:
            fill_rect(surface, dot, (frame + dot) % height, 1, 1, 25, 40, 70)
            dot += 8

        fill_rect(surface, title_box_x, title_box_y, title_box_w, title_box_h, 20, 30, 120)
        fill_rect(surface, title_box_x + 2, title_box_y + 2, title_box_w - 4, title_box_h - 4, 30, 45, 170)
        _draw_text_centered(surface, fill_rect, WELCOME_TEXT, (255, 255, 255), title_box_y, title_box_h)

        for shape in shapes:
            _update_shape(shape, width, height, avoid_box)
            _draw_shape(surface, fill_rect, shape)

        ok = present(surface)
        if frame == 0:
            log("fb_present frame0=" + str(bool(ok)))
        sleep_hz(FPS)
        frame += 1

    log("animation complete for text: " + WELCOME_TEXT)


try:
    run_welcome_demo()
except Exception as exc:
    log("Welcome demo error: " + repr(exc))
