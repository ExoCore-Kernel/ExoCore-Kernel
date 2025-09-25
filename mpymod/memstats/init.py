print("memstats module loaded")
from env import env
import memstats_native as _native

heap_free = _native.heap_free
register_app = _native.register_app
app_used = _native.app_used
save_block = _native.save_block
load_block = _native.load_block

env['memory'] = {
    'heap_free': heap_free,
    'register_app': register_app,
    'app_used': app_used,
    'save_block': save_block,
    'load_block': load_block,
}
