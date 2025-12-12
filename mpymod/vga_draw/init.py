from env import env
from vga_draw_native import (
    start as _start,
    clear as _clear,
    pixel as _pixel,
    line as _line,
    rect as _rect,
    present as _present,
    show as _show,
    is_hidden as _is_hidden,
    WIDTH as WIDTH,
    HEIGHT as HEIGHT,
    VGA_BLACK,
    VGA_BLUE,
    VGA_GREEN,
    VGA_CYAN,
    VGA_RED,
    VGA_MAGENTA,
    VGA_BROWN,
    VGA_LIGHT_GREY,
    VGA_DARK_GREY,
    VGA_LIGHT_BLUE,
    VGA_LIGHT_GREEN,
    VGA_LIGHT_CYAN,
    VGA_LIGHT_RED,
    VGA_LIGHT_MAGENTA,
    VGA_YELLOW,
    VGA_WHITE,
)

start = _start
clear = _clear
pixel = _pixel
line = _line
rect = _rect
present = _present
show = _show
is_hidden = _is_hidden


def _clamp_color(value):
    try:
        numeric = int(value)
    except Exception:
        numeric = 15
    if numeric < 0:
        return 0
    if numeric > 15:
        return 15
    return numeric


def _safe_char(value, default):
    text = "" if value is None else str(value)
    if not text:
        return default
    return text[0]


def circle(cx, cy, radius, char="o", fg=VGA_WHITE, bg=VGA_BLACK, fill=False):
    """Draw a circle using the active drawing session.

    The circle uses an integer midpoint routine and works with both outline and
    filled variants. It relies on ``pixel``/``line`` to render into the current
    ExoDraw buffer so the caller must have already called ``start()``.
    """

    ch = _safe_char(char, "o")
    fg_color = _clamp_color(fg)
    bg_color = _clamp_color(bg)
    x = radius
    y = 0
    decision = 1 - radius

    def _horizontal_span(span_y, span_x):
        line(x0=cx - span_x, y0=span_y, x1=cx + span_x, y1=span_y, char=ch, fg=fg_color, bg=bg_color)

    while x >= y:
        if fill:
            _horizontal_span(cy + y, x)
            _horizontal_span(cy - y, x)
            _horizontal_span(cy + x, y)
            _horizontal_span(cy - x, y)
        else:
            pixel(x=cx + x, y=cy + y, char=ch, fg=fg_color, bg=bg_color)
            pixel(x=cx - x, y=cy + y, char=ch, fg=fg_color, bg=bg_color)
            pixel(x=cx + x, y=cy - y, char=ch, fg=fg_color, bg=bg_color)
            pixel(x=cx - x, y=cy - y, char=ch, fg=fg_color, bg=bg_color)
            pixel(x=cx + y, y=cy + x, char=ch, fg=fg_color, bg=bg_color)
            pixel(x=cx - y, y=cy + x, char=ch, fg=fg_color, bg=bg_color)
            pixel(x=cx + y, y=cy - x, char=ch, fg=fg_color, bg=bg_color)
            pixel(x=cx - y, y=cy - x, char=ch, fg=fg_color, bg=bg_color)

        y += 1
        if decision <= 0:
            decision = decision + 2 * y + 1
        else:
            x -= 1
            decision = decision + 2 * y - 2 * x + 1


def ellipse(cx, cy, rx, ry, char="e", fg=VGA_WHITE, bg=VGA_BLACK, fill=False):
    """Draw an ellipse using a midpoint approximation."""

    if rx < 0 or ry < 0:
        return
    ch = _safe_char(char, "e")
    fg_color = _clamp_color(fg)
    bg_color = _clamp_color(bg)

    def _isqrt(value):
        if value <= 0:
            return 0
        x = value
        while True:
            y = (x + value // x) // 2
            if y >= x:
                return x
            x = y

    if rx == 0 and ry == 0:
        pixel(x=cx, y=cy, char=ch, fg=fg_color, bg=bg_color)
        return

    if rx == 0:
        for y in range(cy - ry, cy + ry + 1):
            pixel(x=cx, y=y, char=ch, fg=fg_color, bg=bg_color)
        return

    if ry == 0:
        if fill:
            line(x0=cx - rx, y0=cy, x1=cx + rx, y1=cy, char=ch, fg=fg_color, bg=bg_color)
        else:
            pixel(x=cx - rx, y=cy, char=ch, fg=fg_color, bg=bg_color)
            pixel(x=cx + rx, y=cy, char=ch, fg=fg_color, bg=bg_color)
        return

    rx2 = rx * rx
    ry2 = ry * ry
    rx2_ry2 = rx2 * ry2

    for dy in range(-ry, ry + 1):
        remaining = rx2_ry2 - (dy * dy) * rx2
        if remaining < 0:
            continue

        span = _isqrt(remaining // ry2)
        y = cy + dy

        if fill:
            line(x0=cx - span, y0=y, x1=cx + span, y1=y, char=ch, fg=fg_color, bg=bg_color)
        else:
            pixel(x=cx - span, y=y, char=ch, fg=fg_color, bg=bg_color)
            if span:
                pixel(x=cx + span, y=y, char=ch, fg=fg_color, bg=bg_color)


def polygon(points, char="*", fg=VGA_WHITE, bg=VGA_BLACK, fill=False):
    """Draw a polygon from a list of ``(x, y)`` tuples."""

    if not points or len(points) < 3:
        return
    ch = _safe_char(char, "*")
    fg_color = _clamp_color(fg)
    bg_color = _clamp_color(bg)

    total = len(points)
    for idx in range(total):
        x0, y0 = points[idx]
        x1, y1 = points[(idx + 1) % total]
        line(x0=int(x0), y0=int(y0), x1=int(x1), y1=int(y1), char=ch, fg=fg_color, bg=bg_color)

    if not fill:
        return

    ys = [p[1] for p in points]
    min_y = int(min(ys))
    max_y = int(max(ys))

    for y in range(min_y, max_y + 1):
        intersections = []
        for idx in range(total):
            x0, y0 = points[idx]
            x1, y1 = points[(idx + 1) % total]
            if y0 == y1:
                continue
            if y < min(y0, y1) or y >= max(y0, y1):
                continue
            proportion = (y - y0) / float(y1 - y0)
            x_cross = x0 + proportion * (x1 - x0)
            intersections.append(int(round(x_cross)))

        intersections.sort()
        spans = len(intersections)
        step = 0
        while step + 1 < spans:
            x_start = intersections[step]
            x_end = intersections[step + 1]
            line(x0=x_start, y0=y, x1=x_end, y1=y, char=ch, fg=fg_color, bg=bg_color)
            step += 2

COLORS = {
    'BLACK': VGA_BLACK,
    'BLUE': VGA_BLUE,
    'GREEN': VGA_GREEN,
    'CYAN': VGA_CYAN,
    'RED': VGA_RED,
    'MAGENTA': VGA_MAGENTA,
    'BROWN': VGA_BROWN,
    'LIGHT_GREY': VGA_LIGHT_GREY,
    'DARK_GREY': VGA_DARK_GREY,
    'LIGHT_BLUE': VGA_LIGHT_BLUE,
    'LIGHT_GREEN': VGA_LIGHT_GREEN,
    'LIGHT_CYAN': VGA_LIGHT_CYAN,
    'LIGHT_RED': VGA_LIGHT_RED,
    'LIGHT_MAGENTA': VGA_LIGHT_MAGENTA,
    'YELLOW': VGA_YELLOW,
    'WHITE': VGA_WHITE,
}

env['vga_draw'] = {
    'width': WIDTH,
    'height': HEIGHT,
    'colors': COLORS,
}
