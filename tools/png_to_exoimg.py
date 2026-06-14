#!/usr/bin/env python3
from PIL import Image
import struct
import sys
FORMAT_RGBA8888 = 1
MAGIC = b"EXOIMG1\0"

def main():
    if len(sys.argv) != 3:
        print("usage: png_to_exoimg.py input.png output.exoimg", file=sys.stderr)
        sys.exit(1)
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    try:
        img = Image.open(input_path).convert("RGBA")
    except Exception as exc:
        print(f"png_to_exoimg: failed to open {input_path}: {exc}", file=sys.stderr)
        sys.exit(1)
    width, height = img.size
    if width <= 0 or height <= 0:
        print("png_to_exoimg: image dimensions must be nonzero", file=sys.stderr)
        sys.exit(1)
    pixels = img.tobytes()
    data_size = len(pixels)
    expected_size = width * height * 4
    if data_size != expected_size:
        raise RuntimeError("unexpected RGBA pixel size")
    header = struct.pack("<8sIIII", MAGIC, width, height, FORMAT_RGBA8888, data_size)
    with open(output_path, "wb") as f:
        f.write(header)
        f.write(pixels)
    print(f"wrote {output_path}: {width}x{height}, {data_size} bytes")

if __name__ == "__main__":
    main()
