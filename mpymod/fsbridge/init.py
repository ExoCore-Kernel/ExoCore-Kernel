print("fsbridge module loaded")
from env import env
import fsbridge_native as _native

read = _native.read
write = _native.write
capacity = _native.capacity
is_mounted = _native.is_mounted

env['fs'] = {
    'read': read,
    'write': write,
    'capacity': capacity,
    'is_mounted': is_mounted,
}
