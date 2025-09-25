print("keyinput module loaded")
from env import env
import keyinput_native as _native

read_char = _native.read_char
read_code = _native.read_code

env['keyboard'] = {
    'read_char': read_char,
    'read_code': read_code,
}
