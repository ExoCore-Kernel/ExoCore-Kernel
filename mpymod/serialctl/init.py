print("serialctl module loaded")
from env import env
import serialctl_native as _native

write = _native.write
write_hex = _native.write_hex
write_dec = _native.write_dec
raw_write = _native.raw_write

env['serial'] = {
    'write': write,
    'write_hex': write_hex,
    'write_dec': write_dec,
    'raw_write': raw_write,
}
