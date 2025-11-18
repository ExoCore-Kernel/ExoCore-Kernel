# ExoDraw Markup Guide

ExoDraw is a lightweight, HTML-inspired markup used to paint text-mode UIs on the VGA framebuffer from raw MicroPython `.py` modules. The parser lives in `mpymod/exodraw/init.py` and renders directly through `vga_draw`, so everything runs inside the kernel's MicroPython runtime without compiled `.mpy` files.

## Quick start

```python
from exodraw import ExoDraw

markup = """
<screen fg="WHITE" bg="BLACK" char=" ">
  <rect x="0" y="0" w="80" h="1" fg="BLACK" bg="LIGHT_GREY" fill="true"/>
  <text x="2" y="0" fg="BLACK" bg="LIGHT_GREY">ExoDraw header</text>
  <text x="2" y="2">Hello from ExoDraw!</text>
  <button x="2" y="4" w="18" h="3" fg="LIGHT_CYAN" bg="BLUE" focus="true">Run check</button>
</screen>
"""

ExoDraw().render(markup)
```

* Put the script in `run/` or a MicroPython module directory, keeping the raw `.py` extension.
* Run the kernel, enter the boot shell, and execute `exodraw` to open the bundled demo, or import your script directly.

## Core elements

| Tag      | Purpose                                                                        |
|----------|--------------------------------------------------------------------------------|
| `screen` | Root surface; set defaults like `fg`, `bg`, and the fill `char` used by `vga_draw.start` |
| `rect`   | Filled or outlined rectangle; `box` is an alias calling the same renderer      |
| `line`   | Straight line drawn with a single character                                   |
| `text`   | Inline text; you can also nest raw text inside other elements                  |
| `button` | Basic framed box with a centered label; swaps foreground/background when `focus="true"` |

### Common attributes

All tags support color overrides with `fg` and `bg`. Colors accept:

* Named VGA entries from `vga_draw.COLORS` (e.g., `LIGHT_CYAN`, `YELLOW`, `BLACK`).
* Numeric values parsed with `int(..., 0)` so hex (`0x0F`) and decimal work.

Booleans recognize `true/false`, `yes/no`, `on/off`, `1/0`, or `y/n` (case-insensitive). Integers accept decimal or `0x`/`0b` prefixed values.

### Geometry

* Coordinates are zero-based: `(0,0)` is the top-left pixel/character cell.
* `rect`/`box`: `x`, `y`, `w`, `h`, `char`, and `fill` (defaults to outline). Width/height default to the full screen width and one row, respectively.
* `line`: `x0`, `y0`, `x1`, `y1`, `char` (defaults to full block `\u2588`).
* `text`: `x`, `y`, `value` (optional). If `value` is omitted, nested text nodes are concatenated.
* `button`: `x`, `y`, `w`, `h`, `label` (or nested text), and `focus` to invert colors.

## Styling and layout flow

ExoDraw does not auto-layout; every element uses absolute coordinates. A parent's `fg`/`bg` propagate to children unless overridden. The renderer calls `vga_draw.present(True)` after finishing all nodes, so a single render paints the whole frame.

Use a `screen` tag to set global defaults and optionally change the background fill character:

```xml
<screen fg="LIGHT_GREY" bg="BLACK" char=".">
  <text x="2" y="1" fg="YELLOW">Status</text>
</screen>
```

## Input handling and composition

The parser only draws; it does not process input. Pair it with `keyinput.read_code` or your own routines:

```python
from exodraw import ExoDraw
from keyinput import read_code

drawer = ExoDraw()
focus = 0

while True:
    markup = f"""
<screen fg=\"WHITE\" bg=\"BLACK\">
  <button x=\"2\" y=\"2\" w=\"16\" h=\"3\" focus=\"{focus==0}\">Interpreter</button>
  <button x=\"20\" y=\"2\" w=\"16\" h=\"3\" focus=\"{focus==1}\">Framebuffer</button>
</screen>
"""
    drawer.render(markup)
    key = read_code()
    if key in (ord('q'), ord('Q')):
        break
    if key in (ord('\t'), ord('d'), ord('D')):
        focus = (focus + 1) % 2
    elif key in (ord('a'), ord('A')):
        focus = (focus - 1) % 2
```

Combine this loop with TinyScript or framebuffer routines to respond when the user presses Enter, just like the bundled demo in `mpymod/exodraw/ui.py`.

## Tips for authors

* Keep markup small and cache strings; MicroPython has limited heap.
* Render once per logical frame—batch changes into a single markup string where possible.
* Use `char` on `screen` or `rect` to swap fill glyphs for shading (`\u2591`, `\u2592`, `\u2593`).
* Stay within `WIDTH` and `HEIGHT` from `vga_draw` to avoid drawing off-screen.
* Ship raw `.py` modules; the kernel skips compiled `.mpy` files because the magic header cannot be detected reliably in this build.
