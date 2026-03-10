#!/usr/bin/env python3
"""Convert PNG files to simple PS2 texture binary (P2TX + RGBA32 pixels)."""

import struct
import sys
from collections import deque
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:
    print("[ERR] Pillow is required: pip install pillow", file=sys.stderr)
    raise SystemExit(1) from exc

MAGIC = b"P2TX"


def _rgb_close(c0: tuple[int, int, int], c1: tuple[int, int, int], tol: int) -> bool:
    return (
        abs(c0[0] - c1[0]) <= tol
        and abs(c0[1] - c1[1]) <= tol
        and abs(c0[2] - c1[2]) <= tol
    )


def apply_border_colorkey(img: Image.Image, tolerance: int = 10) -> Image.Image:
    """If alpha is fully opaque, remove border-connected background color.

    This treats the dominant corner color as background and flood-fills from
    image borders only, so interior dark details are preserved.
    """
    width, height = img.size
    if width <= 0 or height <= 0:
        return img

    data = bytearray(img.tobytes("raw", "RGBA"))

    # Even when alpha already exists, keep border colorkey enabled.
    # Some source sprites contain mixed alpha + opaque matte background.

    corners = [
        (data[0], data[1], data[2]),
        (data[(width - 1) * 4 + 0], data[(width - 1) * 4 + 1], data[(width - 1) * 4 + 2]),
        (data[((height - 1) * width) * 4 + 0], data[((height - 1) * width) * 4 + 1], data[((height - 1) * width) * 4 + 2]),
        (data[((height * width) - 1) * 4 + 0], data[((height * width) - 1) * 4 + 1], data[((height * width) - 1) * 4 + 2]),
    ]

    # Pick most frequent corner color as key color.
    key_color = max(set(corners), key=corners.count)
    key_luma = (key_color[0] + key_color[1] + key_color[2]) // 3

    visited = bytearray(width * height)
    q: deque[tuple[int, int]] = deque()

    def push(x: int, y: int) -> None:
        if x < 0 or y < 0 or x >= width or y >= height:
            return
        idx = y * width + x
        if visited[idx]:
            return
        visited[idx] = 1
        base = idx * 4
        pix = (data[base + 0], data[base + 1], data[base + 2])
        pix_luma = (pix[0] + pix[1] + pix[2]) // 3

        # For dark matte backgrounds, also treat nearby dark pixels as background.
        near_key = _rgb_close(pix, key_color, tolerance)
        dark_bg = (key_luma <= 40 and pix_luma <= 48)
        if near_key or dark_bg:
            q.append((x, y))

    # Seed flood fill from full image border.
    for x in range(width):
        push(x, 0)
        push(x, height - 1)
    for y in range(height):
        push(0, y)
        push(width - 1, y)

    while q:
        x, y = q.popleft()
        idx = y * width + x
        base = idx * 4
        # Make keyed background fully transparent.
        data[base + 3] = 0

        push(x - 1, y)
        push(x + 1, y)
        push(x, y - 1)
        push(x, y + 1)

    return Image.frombytes("RGBA", (width, height), bytes(data))


def convert_one(in_path: Path, out_path: Path) -> int:
    if not in_path.exists():
        print(f"[ERR] input not found: {in_path}", file=sys.stderr)
        return 1

    img = Image.open(in_path).convert("RGBA")
    img = apply_border_colorkey(img, tolerance=28)
    orig_width, orig_height = img.size

    if orig_width <= 0 or orig_height <= 0:
        print("[ERR] invalid image size", file=sys.stderr)
        return 1

    if orig_width > 1024 or orig_height > 1024:
        print("[ERR] image too large for this simple pipeline (max 1024x1024)", file=sys.stderr)
        return 1

    # Keep full source frame to preserve consistent per-state alignment.
    offset_x, offset_y = 0, 0
    width, height = orig_width, orig_height

    # Remove matte/background bleeding from transparent PNG edges.
    # For fully transparent pixels, RGB is forced to 0 so hidden background color does not leak.
    rgba = img.tobytes("raw", "RGBA")
    pixels = bytearray(len(rgba))
    for i in range(0, len(rgba), 4):
        r = rgba[i + 0]
        g = rgba[i + 1]
        b = rgba[i + 2]
        alpha = rgba[i + 3]
        alpha = (alpha * 128 + 127) // 255
        if alpha < 2:
            pixels[i + 0] = 0
            pixels[i + 1] = 0
            pixels[i + 2] = 0
            pixels[i + 3] = 0
        else:
            pixels[i + 0] = r
            pixels[i + 1] = g
            pixels[i + 2] = b
            pixels[i + 3] = alpha
    raw_rgba = bytes(pixels)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as f:
        f.write(struct.pack("<4sHHhh", MAGIC, width, height, offset_x, offset_y))
        f.write(raw_rgba)

    print(f"[OK] converted {in_path} -> {out_path} ({width}x{height} offset={offset_x},{offset_y})")
    return 0


def convert_all(src_dir: Path, out_dir: Path) -> int:
    png_files = sorted(src_dir.rglob("*.png"))

    if not src_dir.exists():
        print(f"[ERR] source directory not found: {src_dir}", file=sys.stderr)
        return 1

    if not png_files:
        print(f"[ERR] no png files found under: {src_dir}", file=sys.stderr)
        return 1

    fail_count = 0
    for in_path in png_files:
        rel = in_path.relative_to(src_dir)
        out_path = out_dir / rel.with_suffix(".ps2tex")
        rc = convert_one(in_path, out_path)
        if rc != 0:
            fail_count += 1

    if fail_count:
        print(f"[ERR] {fail_count} file(s) failed", file=sys.stderr)
        return 1

    print(f"[OK] converted {len(png_files)} file(s) from {src_dir} to {out_dir}")
    return 0


def main() -> int:
    if len(sys.argv) == 3:
        in_path = Path(sys.argv[1])
        out_path = Path(sys.argv[2])
        return convert_one(in_path, out_path)

    if len(sys.argv) == 4 and sys.argv[1] == "--all":
        src_dir = Path(sys.argv[2])
        out_dir = Path(sys.argv[3])
        return convert_all(src_dir, out_dir)

    print("usage:", file=sys.stderr)
    print("  png_to_ps2tex.py <input.png> <output.ps2tex>", file=sys.stderr)
    print("  png_to_ps2tex.py --all <input_dir> <output_dir>", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
