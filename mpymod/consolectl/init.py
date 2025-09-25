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


env['console'] = {
    'write': write,
    'clear': clear,
    'set_attr': set_attr,
    'scroll': scroll,
    'backspace': backspace,
    'colors': COLORS,
}
