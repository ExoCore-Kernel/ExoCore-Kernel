#mpyexo
import vga
print("VGA demo started")
vga.enable(True)
print("Both outputs")
vga.serial(False)
print("VGA only")
vga.serial(True)
vga.enable(False)
print("Serial only")
