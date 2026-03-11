#!/usr/bin/env python3
"""Convert a TTF/OTF font to a PS2-compatible bitmap font atlas (.ps2tex + .fnt).

Generates:
  - <output>.ps2tex : RGBA32 texture atlas (power-of-2 dimensions for PS2 GS)
  - <output>.fnt    : Binary glyph metrics table

.fnt format:
  4 bytes: magic 'FNT1'
  2 bytes: atlas_width  (u16 LE)
  2 bytes: atlas_height (u16 LE)
  2 bytes: cell_width   (u16 LE)
  2 bytes: cell_height  (u16 LE)
  4 bytes: glyph_count  (u32 LE)
  Per glyph (12 bytes):
    4 bytes: unicode codepoint (u32 LE)
    2 bytes: atlas_x (u16 LE)
    2 bytes: atlas_y (u16 LE)
    1 byte:  advance_width (u8)
    1 byte:  bearing_x (s8)
    1 byte:  bearing_y (s8)
    1 byte:  glyph_width (u8)

Usage:
  python tools/ttf_to_ps2font.py <font.ttf> <output_basename> [--size 20]
"""

import math
import struct
import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("[ERR] Pillow required: pip install pillow", file=sys.stderr)
    raise SystemExit(1)

MAGIC_P2TX = b"P2TX"
MAGIC_FNT = b"FNT1"


def next_pow2(v):
    if v <= 0:
        return 1
    return 1 << math.ceil(math.log2(v))


def get_charset(extra=""):
    chars = set()
    for cp in range(32, 127):
        chars.add(cp)
    if extra:
        for ch in extra:
            chars.add(ord(ch))
    return sorted(chars)


def render_font_atlas(font_path, size, codepoints):
    font = ImageFont.truetype(font_path, size)

    cell_h = size + 4
    cell_w = 0
    raw_metrics = []

    for cp in codepoints:
        ch = chr(cp)
        bbox = font.getbbox(ch)
        if bbox is None:
            raw_metrics.append((cp, 0, 0, max(size // 4, 1), 0))
            continue
        left, top, right, bottom = bbox
        gw = right - left
        gh = bottom - top
        advance = max(gw, 1) + 1
        if advance > cell_w:
            cell_w = advance
        if gh + 4 > cell_h:
            cell_h = gh + 4
        raw_metrics.append((cp, left, top, advance, gw))

    cell_w = max(cell_w, size // 2)
    cell_w = (cell_w + 3) & ~3
    cell_h = (cell_h + 3) & ~3

    # Pick columns so atlas width stays reasonable
    cols = max(1, 256 // cell_w)
    rows = (len(codepoints) + cols - 1) // cols

    # Force power-of-2 dimensions (required by PS2 GS hardware)
    atlas_w = next_pow2(cols * cell_w)
    atlas_h = next_pow2(rows * cell_h)

    # Re-derive cols from final atlas_w
    cols = atlas_w // cell_w

    img = Image.new("RGBA", (atlas_w, atlas_h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    glyph_metrics = []
    for i, (cp, left, top, advance, gw) in enumerate(raw_metrics):
        col = i % cols
        row = i // cols
        ax = col * cell_w
        ay = row * cell_h

        ch = chr(cp)
        bbox = font.getbbox(ch)
        if bbox is None:
            glyph_metrics.append((cp, ax, ay, advance, 0, 0, max(gw, 1)))
            continue

        left, top, right, bottom = bbox
        draw.text((ax - left, ay - top), ch, font=font, fill=(255, 255, 255, 255))
        glyph_metrics.append((cp, ax, ay, advance, left, top, right - left))

    return img, glyph_metrics, atlas_w, atlas_h, cell_w, cell_h


def write_ps2tex(path, img):
    w, h = img.size
    rgba = img.tobytes("raw", "RGBA")

    pixels = bytearray(len(rgba))
    for i in range(0, len(rgba), 4):
        a = rgba[i + 3]
        a128 = (a * 128 + 127) // 255
        if a128 < 2:
            pixels[i:i + 4] = b'\x00\x00\x00\x00'
        else:
            pixels[i]     = rgba[i]
            pixels[i + 1] = rgba[i + 1]
            pixels[i + 2] = rgba[i + 2]
            pixels[i + 3] = a128

    with path.open("wb") as f:
        f.write(struct.pack("<4sHHhh", MAGIC_P2TX, w, h, 0, 0))
        f.write(bytes(pixels))


def write_fnt(path, atlas_w, atlas_h, cell_w, cell_h, glyph_metrics):
    with path.open("wb") as f:
        f.write(MAGIC_FNT)
        f.write(struct.pack("<HHHHI",
                            atlas_w, atlas_h, cell_w, cell_h,
                            len(glyph_metrics)))
        for cp, ax, ay, advance, bearing_x, bearing_y, gw in glyph_metrics:
            adv = max(0, min(255, advance))
            bx = max(-128, min(127, bearing_x))
            by = max(-128, min(127, bearing_y))
            f.write(struct.pack("<IHHBbbB",
                                cp, ax, ay,
                                adv, bx, by, min(255, gw)))


def main():
    import argparse
    parser = argparse.ArgumentParser(description="Convert TTF to PS2 bitmap font")
    parser.add_argument("font", help="Path to .ttf/.otf file")
    parser.add_argument("output", help="Output basename (e.g. assets/font_ui)")
    parser.add_argument("--size", type=int, default=20, help="Font pixel size (default: 20)")
    parser.add_argument("--chars", default="", help="Additional characters to include")
    args = parser.parse_args()

    if not Path(args.font).exists():
        print(f"[ERR] Font file not found: {args.font}", file=sys.stderr)
        return 1

    codepoints = get_charset(args.chars)
    print(f"Rendering {len(codepoints)} glyphs from {args.font} at size {args.size}...")

    img, metrics, aw, ah, cw, ch = render_font_atlas(args.font, args.size, codepoints)
    print(f"Atlas: {aw}x{ah} (cell {cw}x{ch}, power-of-2)")

    out_base = Path(args.output)
    out_base.parent.mkdir(parents=True, exist_ok=True)

    tex_path = out_base.with_suffix(".ps2tex")
    fnt_path = out_base.with_suffix(".fnt")

    write_ps2tex(tex_path, img)
    write_fnt(fnt_path, aw, ah, cw, ch, metrics)

    print(f"[OK] {tex_path} ({aw}x{ah}, {aw * ah * 4} bytes RGBA)")
    print(f"[OK] {fnt_path} ({len(metrics)} glyphs)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
