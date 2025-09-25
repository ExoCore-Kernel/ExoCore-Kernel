print("runstatectl module loaded")
from env import env
import runstatectl_native as _native

current_program = _native.current_program
set_current_program = _native.set_current_program
current_user_app = _native.current_user_app
set_current_user_app = _native.set_current_user_app
is_debug_mode = _native.is_debug_mode
set_debug_mode = _native.set_debug_mode
is_vga_output = _native.is_vga_output
set_vga_output = _native.set_vga_output

env['runstate'] = {
    'current_program': current_program,
    'set_current_program': set_current_program,
    'current_user_app': current_user_app,
    'set_current_user_app': set_current_user_app,
    'is_debug_mode': is_debug_mode,
    'set_debug_mode': set_debug_mode,
    'is_vga_output': is_vga_output,
    'set_vga_output': set_vga_output,
}
