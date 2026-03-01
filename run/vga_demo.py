#mpyexo
"""Animated VGA demo with centered title text and moving colored shapes."""

import vga
import vga_draw


def _draw_text_centered(text, y, fg, bg):
    start_x = (vga_draw.WIDTH - len(text)) // 2
    if start_x < 0:
        start_x = 0
    x = start_x
    for ch in text:
        vga_draw.pixel(x=x, y=y, char=ch, fg=fg, bg=bg)
        x += 1
    return start_x, start_x + len(text) - 1


def _draw_shape(shape):
    if shape['kind'] == 'rect':
        vga_draw.rect(
            x=shape['x'],
            y=shape['y'],
            w=shape['w'],
            h=shape['h'],
            char=shape['char'],
            fg=shape['color'],
            bg=vga_draw.VGA_BLACK,
            fill=True,
        )
    elif shape['kind'] == 'line':
        vga_draw.line(
            x0=shape['x'],
            y0=shape['y'],
            x1=shape['x'] + shape['w'],
            y1=shape['y'] + shape['h'],
            char=shape['char'],
            fg=shape['color'],
            bg=vga_draw.VGA_BLACK,
        )
    else:
        vga_draw.circle(
            cx=shape['x'],
            cy=shape['y'],
            radius=shape['r'],
            char=shape['char'],
            fg=shape['color'],
            bg=vga_draw.VGA_BLACK,
            fill=True,
        )


def _step_shape(shape, min_x, min_y, max_x, max_y, text_box):
    next_x = shape['x'] + shape['dx']
    next_y = shape['y'] + shape['dy']

    left = next_x
    right = next_x + shape.get('w', shape.get('r', 0))
    top = next_y
    bottom = next_y + shape.get('h', shape.get('r', 0))

    if left < min_x or right > max_x:
        shape['dx'] = -shape['dx']
        next_x = shape['x'] + shape['dx']

    if top < min_y or bottom > max_y:
        shape['dy'] = -shape['dy']
        next_y = shape['y'] + shape['dy']

    tx0, ty0, tx1, ty1 = text_box
    collides_text = not (right < tx0 or left > tx1 or bottom < ty0 or top > ty1)
    if collides_text:
        shape['dx'] = -shape['dx']
        shape['dy'] = -shape['dy']
        next_x = shape['x'] + shape['dx']
        next_y = shape['y'] + shape['dy']

    shape['x'] = next_x
    shape['y'] = next_y


def run_demo(frames=220):
    title = 'Welcome to ExoCore!'
    title_y = vga_draw.HEIGHT // 2

    vga.enable(True)
    vga_draw.start(fill=' ', fg=vga_draw.VGA_WHITE, bg=vga_draw.VGA_BLACK)

    title_x0, title_x1 = _draw_text_centered(title, title_y, vga_draw.VGA_WHITE, vga_draw.VGA_BLUE)

    text_box = (
        title_x0 - 2,
        title_y - 1,
        title_x1 + 2,
        title_y + 1,
    )

    shapes = [
        {'kind': 'rect', 'x': 2, 'y': 2, 'w': 6, 'h': 3, 'dx': 1, 'dy': 1, 'char': '#', 'color': vga_draw.VGA_LIGHT_RED},
        {'kind': 'circle', 'x': vga_draw.WIDTH - 6, 'y': 3, 'r': 2, 'dx': -1, 'dy': 1, 'char': '@', 'color': vga_draw.VGA_LIGHT_GREEN},
        {'kind': 'line', 'x': 3, 'y': vga_draw.HEIGHT - 5, 'w': 5, 'h': 0, 'dx': 1, 'dy': -1, 'char': '=', 'color': vga_draw.VGA_YELLOW},
        {'kind': 'rect', 'x': vga_draw.WIDTH - 10, 'y': vga_draw.HEIGHT - 6, 'w': 7, 'h': 2, 'dx': -1, 'dy': -1, 'char': '%', 'color': vga_draw.VGA_LIGHT_CYAN},
        {'kind': 'circle', 'x': vga_draw.WIDTH // 2, 'y': 2, 'r': 1, 'dx': 1, 'dy': 1, 'char': '*', 'color': vga_draw.VGA_LIGHT_MAGENTA},
    ]

    for _ in range(frames):
        vga_draw.clear(fill=' ', fg=vga_draw.VGA_DARK_GREY, bg=vga_draw.VGA_BLACK)

        # Title banner in the middle.
        vga_draw.rect(
            x=text_box[0],
            y=text_box[1],
            w=(text_box[2] - text_box[0]) + 1,
            h=(text_box[3] - text_box[1]) + 1,
            char=' ',
            fg=vga_draw.VGA_WHITE,
            bg=vga_draw.VGA_BLUE,
            fill=True,
        )
        _draw_text_centered(title, title_y, vga_draw.VGA_WHITE, vga_draw.VGA_BLUE)

        for shape in shapes:
            _step_shape(shape, 0, 0, vga_draw.WIDTH - 1, vga_draw.HEIGHT - 1, text_box)
            _draw_shape(shape)

        vga_draw.present(False)

        # Tiny busy wait to make movement observable in bare-metal MicroPython.
        delay = 0
        while delay < 13000:
            delay += 1

    vga_draw.present(True)
    print('High-resolution VGA demo completed')


run_demo()
