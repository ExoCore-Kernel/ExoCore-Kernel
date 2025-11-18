#mpyexo
"""Interactive ExoDraw demonstration for TinyScript and framebuffer checks."""

from exodraw import ExoDraw
from vga_draw import WIDTH, HEIGHT, COLORS, rect as vga_rect, VGA_ATTR
from keyinput import read_code

try:
    import tscript
except ImportError:  # pragma: no cover - runtime diagnostic
    tscript = None


HEADER_BG = COLORS.get("LIGHT_GREY", 7)
HEADER_FG = COLORS.get("BLACK", 0)
FOCUS_BG = COLORS.get("LIGHT_CYAN", 11)
FOCUS_FG = COLORS.get("BLACK", 0)
CANVAS_X = 2
CANVAS_Y = 12
CANVAS_W = WIDTH - 4
CANVAS_H = max(6, HEIGHT - CANVAS_Y - 2)


def _run_interpreter_probe():
    if tscript is None:
        return "TinyScript module missing"
    try:
        tscript.run("2 3 + dup * print")
        return "TinyScript OK (2 3 + dup * -> 25)"
    except Exception as exc:
        return "TinyScript error: %s" % exc


def _draw_framebuffer_probe():
    attr = VGA_ATTR(COLORS.get("LIGHT_GREEN", 10), COLORS.get("BLACK", 0))
    edge_attr = VGA_ATTR(COLORS.get("YELLOW", 14), COLORS.get("BLACK", 0))
    vga_rect(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, '\u2592', attr, True)
    vga_rect(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, '\u2593', edge_attr, False)

    gradient_fg = COLORS.get("LIGHT_RED", 12)
    gradient_bg = COLORS.get("BLUE", 1)
    for offset in range(0, CANVAS_W - 2, 3):
        ch = "*" if (offset // 3) % 2 == 0 else "+"
        vga_rect(CANVAS_X + 1 + offset, CANVAS_Y + 1, 1, CANVAS_H - 2, ch, VGA_ATTR(gradient_fg, gradient_bg), True)
    return "Framebuffer OK (canvas refreshed)"


def _build_markup(focus, statuses):
    interp_focus = "true" if focus == 0 else "false"
    fb_focus = "true" if focus == 1 else "false"
    interp_label = "Interpreter"
    fb_label = "Framebuffer"
    return f"""
<screen fg=\"WHITE\" bg=\"BLACK\" char=\" \">\n  <rect x=\"0\" y=\"0\" w=\"{WIDTH}\" h=\"1\" fg=\"{HEADER_FG}\" bg=\"{HEADER_BG}\" fill=\"true\" char=\" \"/>\n  <text x=\"2\" y=\"0\" fg=\"{HEADER_FG}\" bg=\"{HEADER_BG}\">ExoDraw UI harness</text>\n  <text x=\"2\" y=\"2\" fg=\"LIGHT_CYAN\">TAB or A/D to change focus, ENTER to run, Q to quit.</text>\n  <text x=\"2\" y=\"3\" fg=\"LIGHT_GREY\">Interpreter status: {statuses['interpreter']}</text>\n  <text x=\"2\" y=\"4\" fg=\"LIGHT_GREY\">Framebuffer status: {statuses['framebuffer']}</text>\n  <button x=\"2\" y=\"6\" w=\"24\" h=\"4\" fg=\"{FOCUS_FG}\" bg=\"{FOCUS_BG}\" focus=\"{interp_focus}\">{interp_label}</button>\n  <button x=\"28\" y=\"6\" w=\"24\" h=\"4\" fg=\"{FOCUS_FG}\" bg=\"{FOCUS_BG}\" focus=\"{fb_focus}\">{fb_label}</button>\n  <rect x=\"{CANVAS_X}\" y=\"{CANVAS_Y}\" w=\"{CANVAS_W}\" h=\"{CANVAS_H}\" fg=\"LIGHT_GREY\" bg=\"BLACK\" fill=\"false\" char=\" \"/>\n  <text x=\"{CANVAS_X + 2}\" y=\"{CANVAS_Y}\" fg=\"YELLOW\">Framebuffer canvas</text>\n</screen>\n"""


def _read_key():
    code = read_code()
    if code is None:
        return None
    try:
        return int(code)
    except Exception:
        return None


def main():
    drawer = ExoDraw()
    focus = 0
    statuses = {"interpreter": "Waiting", "framebuffer": "Idle"}
    canvas_ready = False

    while True:
        markup = _build_markup(focus, statuses)
        if canvas_ready:
            drawer.render(markup, post=_draw_framebuffer_probe)
        else:
            drawer.render(markup)
        key = _read_key()
        if key in (ord('q'), ord('Q')):
            break
        if key in (ord('\t'), ord('d'), ord('D'), ord('l'), ord('L')):
            focus = (focus + 1) % 2
            continue
        if key in (ord('a'), ord('A'), ord('h'), ord('H')):
            focus = (focus - 1) % 2
            continue
        if key in (10, 13):
            if focus == 0:
                statuses["interpreter"] = _run_interpreter_probe()
            else:
                statuses["framebuffer"] = _draw_framebuffer_probe()
                canvas_ready = True
            continue


if __name__ == "__main__":
    main()
