print("vga module loaded")
from env import env

def enable(flag):
    env['vga_enabled'] = bool(flag)

env['vga'] = True
