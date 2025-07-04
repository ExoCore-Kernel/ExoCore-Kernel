print("vgademo module loaded")
import vga
vga.enable(True)
print("mpymod demo both")
vga.serial(False)
print("mpymod demo vga only")
vga.serial(True)
vga.enable(False)
print("mpymod demo serial only")
