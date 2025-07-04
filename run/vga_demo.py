#mpyexo
import vga
print("VGA demo started")
vga.enable(True)
print("This should appear on both VGA and serial")
vga.enable(False)
print("VGA disabled")
