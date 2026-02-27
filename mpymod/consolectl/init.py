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


def blit_pattern(frame, width, height, x=0, y=0):
    return _native.blit_pattern(int(frame), int(width), int(height), int(x), int(y))


env['console'] = {
    'write': write,
    'clear': clear,
    'set_attr': set_attr,
    'scroll': scroll,
    'backspace': backspace,
    'blit_pixels': blit_pixels,
    'blit_pattern': blit_pattern,
    'colors': COLORS,
    'ansi': False,
}
