print("ExoCore init starting")
print("MicroPython environment ready")

try:
    import vga
    print("Testing VGA module...")
    vga.enable(True)
    print("VGA module call succeeded")
except Exception as e:
    print("VGA module test failed:", e)
