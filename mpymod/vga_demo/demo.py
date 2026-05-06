print("vgademo module loaded")
import vga
import vga_draw
import mouse
import time

vga.enable(True)
print("vgademo enabled VGA output")

if mouse.init():
    print("mouse initialized")

width = vga_draw.WIDTH
height = vga_draw.HEIGHT
cursor_x = width // 2
cursor_y = height // 2

vga_draw.start(bg=vga_draw.VGA_BLUE, fg=vga_draw.VGA_WHITE, clear=True)

while True:
    packet = mouse.poll()
    if packet is not None:
        dx, dy, left, right, middle = packet
        cursor_x += int(dx)
        cursor_y += int(dy)
        if cursor_x < 0:
            cursor_x = 0
        elif cursor_x >= width:
            cursor_x = width - 1

        if cursor_y < 0:
            cursor_y = 0
        elif cursor_y >= height:
            cursor_y = height - 1

    vga_draw.clear(char=' ', fg=vga_draw.VGA_WHITE, bg=vga_draw.VGA_BLUE)
    vga_draw.rect(x=0, y=0, w=width, h=height, char='#', fg=vga_draw.VGA_LIGHT_GREY, bg=vga_draw.VGA_BLUE)
    vga_draw.pixel(x=cursor_x, y=cursor_y, char='X', fg=vga_draw.VGA_YELLOW, bg=vga_draw.VGA_RED)
    vga_draw.present(False)
    time.sleep_ms(16)
