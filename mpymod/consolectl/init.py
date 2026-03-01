print("consolectl module loaded")
from env import env
import consolectl_native as _native

COLORS = _native.colors()


def write(text):
    text = str(text)
    return _native.write(text)


def clear():
    _native.clear()


def set_attr(fg, bg):
    _native.set_attr(int(fg), int(bg))


def scroll(lines=1):
    _native.scroll(int(lines))


def backspace(count=1):
    _native.backspace(int(count))


def blit_pixels(buffer, width, height, x=0, y=0, stride=0):
    return _native.blit_pixels(buffer, int(width), int(height), int(x), int(y), int(stride))


def _clamp_color(value):
    try:
        numeric = int(value)
    except Exception:
        numeric = 0
    if numeric < 0:
        return 0
    if numeric > 255:
        return 255
    return numeric


def _clamp_dimension(value, low, high, default):
    try:
        numeric = int(value)
    except Exception:
        numeric = default
    if numeric < low:
        return low
    if numeric > high:
        return high
    return numeric


def fb_create(width=320, height=200):
    width = _clamp_dimension(width, 16, 640, 320)
    height = _clamp_dimension(height, 12, 480, 200)
    stride = width * 3
    buffer = bytearray(stride * height)
    return {
        'buffer': buffer,
        'width': width,
        'height': height,
        'stride': stride,
    }




def fb_create_bestfit(width=320, height=200, min_width=64, min_height=48):
    target_w = _clamp_dimension(width, 16, 640, 320)
    target_h = _clamp_dimension(height, 12, 480, 200)
    low_w = _clamp_dimension(min_width, 16, 640, 64)
    low_h = _clamp_dimension(min_height, 12, 480, 48)

    while target_w >= low_w and target_h >= low_h:
        try:
            return fb_create(target_w, target_h)
        except Exception:
            next_w = target_w // 2
            next_h = target_h // 2
            if next_w < low_w:
                next_w = low_w
            if next_h < low_h:
                next_h = low_h
            if next_w == target_w and next_h == target_h:
                break
            target_w = next_w
            target_h = next_h
    return fb_create(low_w, low_h)
def fb_resize(surface, width, height):
    created = fb_create(width, height)
    surface['buffer'] = created['buffer']
    surface['width'] = created['width']
    surface['height'] = created['height']
    surface['stride'] = created['stride']
    return surface


def fb_set_pixel(surface, x, y, r, g, b):
    width = int(surface['width'])
    height = int(surface['height'])
    try:
        x = int(x)
        y = int(y)
    except Exception:
        return False
    if x < 0 or y < 0 or x >= width or y >= height:
        return False
    offset = y * int(surface['stride']) + x * 3
    buffer = surface['buffer']
    buffer[offset] = _clamp_color(r)
    buffer[offset + 1] = _clamp_color(g)
    buffer[offset + 2] = _clamp_color(b)
    return True


def fb_clear(surface, r=0, g=0, b=0):
    rr = _clamp_color(r)
    gg = _clamp_color(g)
    bb = _clamp_color(b)
    buffer = surface['buffer']
    stride = int(surface['stride'])
    height = int(surface['height'])
    width = int(surface['width'])
    y = 0
    while y < height:
        offset = y * stride
        x = 0
        while x < width:
            buffer[offset] = rr
            buffer[offset + 1] = gg
            buffer[offset + 2] = bb
            offset += 3
            x += 1
        y += 1


def fb_fill_rect(surface, x, y, width, height, r, g, b):
    try:
        x0 = int(x)
        y0 = int(y)
        w = int(width)
        h = int(height)
    except Exception:
        return
    if w <= 0 or h <= 0:
        return

    max_w = int(surface['width'])
    max_h = int(surface['height'])
    x1 = x0 + w
    y1 = y0 + h
    if x0 < 0:
        x0 = 0
    if y0 < 0:
        y0 = 0
    if x1 > max_w:
        x1 = max_w
    if y1 > max_h:
        y1 = max_h

    rr = _clamp_color(r)
    gg = _clamp_color(g)
    bb = _clamp_color(b)
    buffer = surface['buffer']
    stride = int(surface['stride'])

    yy = y0
    while yy < y1:
        offset = yy * stride + x0 * 3
        xx = x0
        while xx < x1:
            buffer[offset] = rr
            buffer[offset + 1] = gg
            buffer[offset + 2] = bb
            offset += 3
            xx += 1
        yy += 1


def fb_present(surface, x=0, y=0):
    buffer = surface['buffer']
    width = int(surface['width'])
    height = int(surface['height'])
    stride = int(surface['stride'])
    try:
        return bool(blit_pixels(buffer, width, height, int(x), int(y), stride))
    except TypeError:
        return bool(blit_pixels(buffer, width, height))


def fb_sleep_hz(hz):
    try:
        numeric = int(hz)
    except Exception:
        numeric = 30
    if numeric < 1:
        numeric = 1
    if numeric > 360:
        numeric = 360
    delay_ms = 1000 // numeric
    if delay_ms < 1:
        delay_ms = 1

    try:
        import time
        sleeper_ms = getattr(time, 'sleep_ms', None)
        if callable(sleeper_ms):
            sleeper_ms(delay_ms)
            return
        sleeper = getattr(time, 'sleep', None)
        if callable(sleeper):
            sleeper(max(1, delay_ms // 1000))
            return
    except Exception:
        pass

    loops = delay_ms * 1200
    if loops < 30000:
        loops = 30000
    sink = 0
    idx = 0
    while idx < loops:
        sink = (sink + idx) & 0xFFFF
        idx += 1


env['console'] = {
    'write': write,
    'clear': clear,
    'set_attr': set_attr,
    'scroll': scroll,
    'backspace': backspace,
    'blit_pixels': blit_pixels,
    'colors': COLORS,
    'ansi': False,
    'fb_create': fb_create,
    'fb_create_bestfit': fb_create_bestfit,
    'fb_resize': fb_resize,
    'fb_set_pixel': fb_set_pixel,
    'fb_clear': fb_clear,
    'fb_fill_rect': fb_fill_rect,
    'fb_present': fb_present,
    'fb_sleep_hz': fb_sleep_hz,
}
