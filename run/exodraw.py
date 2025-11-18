#mpyexo
"""ExoDraw: lightweight markup renderer for the text-mode framebuffer.

The language resembles HTML but stays tiny enough for the bundled
MicroPython runtime. Elements map directly to VGA drawing primitives so
UI definitions can remain data-driven and easy to tweak inside the ISO.
"""

try:
    import vga_draw
    from vga_draw import (
        start as _start,
        clear as _clear,
        pixel as _pixel,
        line as _line,
        rect as _rect,
        present as _present,
        WIDTH,
        HEIGHT,
        COLORS,
    )
except Exception as exc:  # pragma: no cover - runtime diagnostic
    raise ImportError("vga_draw module is required for ExoDraw: %s" % exc)


def _as_bool(value, default=False):
    if value is None:
        return default
    if isinstance(value, bool):
        return value
    try:
        text = str(value).strip().lower()
    except Exception:
        return default
    if text in ("1", "true", "yes", "on", "y"):
        return True
    if text in ("0", "false", "no", "off", "n"):
        return False
    return default


def _as_int(value, default=0):
    if value is None:
        return default
    if isinstance(value, int):
        return value
    try:
        return int(str(value).strip(), 0)
    except Exception:
        return default


def _as_color(value, default=0):
    if value is None:
        return default
    if isinstance(value, int):
        return value & 0xFF
    try:
        text = str(value).strip()
    except Exception:
        return default
    upper = text.upper()
    if upper in COLORS:
        return COLORS[upper]
    try:
        return int(text, 0) & 0xFF
    except Exception:
        return default


def _parse_attrs(raw):
    attrs = {}
    i = 0
    raw_len = len(raw)
    while i < raw_len:
        while i < raw_len and raw[i].isspace():
            i += 1
        if i >= raw_len:
            break
        key_start = i
        while i < raw_len and not raw[i].isspace() and raw[i] != '=':
            i += 1
        key = raw[key_start:i]
        while i < raw_len and raw[i].isspace():
            i += 1
        value = None
        if i < raw_len and raw[i] == '=':
            i += 1
            while i < raw_len and raw[i].isspace():
                i += 1
            if i < raw_len and raw[i] in ('"', "'"):
                quote = raw[i]
                i += 1
                val_start = i
                while i < raw_len and raw[i] != quote:
                    i += 1
                value = raw[val_start:i]
                if i < raw_len:
                    i += 1
            else:
                val_start = i
                while i < raw_len and not raw[i].isspace():
                    i += 1
                value = raw[val_start:i]
        if key:
            attrs[key] = value
    return attrs


def _parse_nodes(markup):
    pos = 0
    length = len(markup)

    def parse_block(stop_tag=None):
        nonlocal pos
        nodes = []
        text_buf = []

        def flush_text():
            if not text_buf:
                return
            text = "".join(text_buf)
            text_buf[:] = []
            if text.strip():
                nodes.append({"tag": "#text", "text": text})

        while pos < length:
            ch = markup[pos]
            if ch == '<':
                flush_text()
                pos += 1
                closing = False
                if pos < length and markup[pos] == '/':
                    closing = True
                    pos += 1
                tag_start = pos
                while pos < length and markup[pos] not in (' ', '\t', '\r', '\n', '>', '/'):
                    pos += 1
                tag = markup[tag_start:pos]
                attr_start = pos
                while pos < length and markup[pos] != '>':
                    pos += 1
                raw_attrs = markup[attr_start:pos]
                if pos < length:
                    pos += 1
                raw_attrs = raw_attrs.strip()
                self_close = raw_attrs.endswith('/')
                if self_close:
                    raw_attrs = raw_attrs[:-1]
                attrs = _parse_attrs(raw_attrs)

                if closing:
                    if stop_tag and tag == stop_tag:
                        return nodes
                    continue
                if self_close:
                    nodes.append({"tag": tag, "attrs": attrs, "children": []})
                    continue
                children = parse_block(tag)
                nodes.append({"tag": tag, "attrs": attrs, "children": children})
            else:
                text_buf.append(ch)
                pos += 1
        flush_text()
        return nodes

    return parse_block()


class ExoDraw:
    """Parse ExoDraw markup and render it through the VGA draw module."""

    def __init__(self, width=None, height=None):
        self.width = width or WIDTH
        self.height = height or HEIGHT

    def parse(self, markup):
        return _parse_nodes(markup)

    def render(self, markup, base_fg="WHITE", base_bg="BLACK", clear=True, post=None):
        nodes = self.parse(markup)
        fg = _as_color(base_fg, COLORS.get("WHITE", 15))
        bg = _as_color(base_bg, COLORS.get("BLACK", 0))
        root_char = ' '

        for node in nodes:
            if node.get("tag") == "screen":
                attrs = node.get("attrs", {})
                fg = _as_color(attrs.get("fg"), fg)
                bg = _as_color(attrs.get("bg"), bg)
                if attrs.get("char"):
                    root_char = str(attrs.get("char"))[:1]
                break

        _start(fg=fg, bg=bg, char=root_char, clear=clear)
        style = {"fg": fg, "bg": bg}
        self._render_nodes(nodes, style)
        if callable(post):
            post()
        _present(True)

    def _render_nodes(self, nodes, style):
        for node in nodes:
            tag = node.get("tag")
            if not tag:
                continue
            if tag == "#text":
                continue
            handler = getattr(self, "_render_" + tag, None)
            if handler:
                handler(node, style)
            else:
                self._render_generic(node, style)

    def _child_style(self, node, style):
        attrs = node.get("attrs", {})
        fg = _as_color(attrs.get("fg"), style.get("fg", COLORS.get("WHITE", 15)))
        bg = _as_color(attrs.get("bg"), style.get("bg", COLORS.get("BLACK", 0)))
        return {"fg": fg, "bg": bg}

    def _render_generic(self, node, style):
        child_style = self._child_style(node, style)
        self._render_nodes(node.get("children", []), child_style)

    def _render_screen(self, node, style):
        child_style = self._child_style(node, style)
        self._render_nodes(node.get("children", []), child_style)

    def _render_rect(self, node, style):
        attrs = node.get("attrs", {})
        child_style = self._child_style(node, style)
        x = _as_int(attrs.get("x"), 0)
        y = _as_int(attrs.get("y"), 0)
        w = _as_int(attrs.get("w"), self.width)
        h = _as_int(attrs.get("h"), 1)
        ch = str(attrs.get("char", ' '))[:1]
        fill = _as_bool(attrs.get("fill"), False)
        _rect(x, y, w, h, ch, vga_draw.VGA_ATTR(child_style["fg"], child_style["bg"]), fill)

    def _render_box(self, node, style):
        self._render_rect(node, style)

    def _render_line(self, node, style):
        attrs = node.get("attrs", {})
        child_style = self._child_style(node, style)
        x0 = _as_int(attrs.get("x0"), 0)
        y0 = _as_int(attrs.get("y0"), 0)
        x1 = _as_int(attrs.get("x1"), x0)
        y1 = _as_int(attrs.get("y1"), y0)
        ch = str(attrs.get("char", '\u2588'))[:1]
        _line(x0, y0, x1, y1, ch, vga_draw.VGA_ATTR(child_style["fg"], child_style["bg"]))

    def _render_text(self, node, style):
        attrs = node.get("attrs", {})
        child_style = self._child_style(node, style)
        x = _as_int(attrs.get("x"), 0)
        y = _as_int(attrs.get("y"), 0)
        text = attrs.get("value")
        if text is None:
            text = self._collect_text(node)
        self._draw_text(x, y, text, child_style)

    def _render_button(self, node, style):
        attrs = node.get("attrs", {})
        child_style = self._child_style(node, style)
        x = _as_int(attrs.get("x"), 0)
        y = _as_int(attrs.get("y"), 0)
        w = _as_int(attrs.get("w"), 10)
        h = _as_int(attrs.get("h"), 3)
        label = attrs.get("label") or self._collect_text(node) or ""
        focus = _as_bool(attrs.get("focus"), False)

        bg = child_style["bg"]
        fg = child_style["fg"]
        if focus:
            fg, bg = bg, fg
        _rect(x, y, w, h, ' ', vga_draw.VGA_ATTR(fg, bg), True)
        if w >= 2 and h >= 2:
            _rect(x, y, w, h, '\u2592' if focus else '\u2591', vga_draw.VGA_ATTR(fg, bg), False)
        text_x = x + max(1, (w - len(label)) // 2)
        text_y = y + h // 2
        self._draw_text(text_x, text_y, label, {"fg": fg, "bg": bg})

    def _collect_text(self, node):
        parts = []
        for child in node.get("children", []):
            if child.get("tag") == "#text":
                parts.append(child.get("text", ""))
        return "".join(parts).strip()

    def _draw_text(self, x, y, text, style):
        if text is None:
            return
        fg = style.get("fg", COLORS.get("WHITE", 15))
        bg = style.get("bg", COLORS.get("BLACK", 0))
        col = x
        row = y
        for ch in str(text):
            if ch == '\n':
                row += 1
                col = x
                continue
            _pixel(col, row, ch, vga_draw.VGA_ATTR(fg, bg))
            col += 1

