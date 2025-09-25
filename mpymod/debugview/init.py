print("debugview module loaded")
from env import env
import debugview_native as _native

flush = _native.flush
dump_console = _native.dump_console
save_file = _native.save_file
buffer = _native.buffer
is_available = _native.is_available

env['debuglog'] = {
    'flush': flush,
    'dump_console': dump_console,
    'save_file': save_file,
    'buffer': buffer,
    'is_available': is_available,
}
