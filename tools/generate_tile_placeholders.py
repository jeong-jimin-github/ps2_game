#!/usr/bin/env python3
"""Generate placeholder tile PNG images for new block types.

Creates simple 32x32 pixel tile PNGs in assets/src/ so the build
pipeline (png_to_ps2tex.py) can convert them to .ps2tex format.
Replace these with real artwork later.

Usage:
    python tools/generate_tile_placeholders.py
    python tools/generate_tile_placeholders.py --size 32
"""

import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("[ERR] Pillow required: pip install pillow", file=sys.stderr)
    raise SystemExit(1)

# Output directory
OUT_DIR = Path(__file__).resolve().parent.parent / "assets" / "src"

# Tile definitions: (filename, bg_color, symbol, symbol_color, outline_color)
TILES = [
    ("tile_coinblock",  (200, 160,  40, 255), "?", (255, 220,  60), ( 80,  50,  10)),
    ("tile_emptyblock", (120, 100,  60, 255), " ", (120, 100,  60), ( 60,  50,  30)),
    ("tile_coin",       (  0,   0,   0,   0), "O", (255, 215,   0), (200, 165,   0)),
    ("tile_mushroom",   (  0,   0,   0,   0), "M", (220,  40,  40), (180,  20,  20)),
    ("tile_1up",        (  0,   0,   0,   0), "1", ( 40, 200,  60), ( 20, 160,  40)),
]


def draw_block_tile(size: int, bg: tuple, symbol: str, sym_color: tuple, outline: tuple) -> Image.Image:
    """Draw a simple block tile with border and centered symbol."""
    img = Image.new("RGBA", (size, size), bg)
    draw = ImageDraw.Draw(img)

    # Outer border (solid block look)
    if bg[3] > 0:
        draw.rectangle([0, 0, size - 1, size - 1], outline=outline, width=2)
        # Inner highlight
        draw.line([(2, 2), (size - 3, 2)], fill=(255, 255, 255, 100), width=1)
        draw.line([(2, 2), (2, size - 3)], fill=(255, 255, 255, 80), width=1)

    # Symbol
    if symbol.strip():
        # Use default font, centered
        bbox = draw.textbbox((0, 0), symbol)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        tx = (size - tw) // 2
        ty = (size - th) // 2 - 1
        draw.text((tx, ty), symbol, fill=sym_color)

    return img


def draw_coin_tile(size: int) -> Image.Image:
    """Draw a coin as a simple circle."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    margin = size // 6
    draw.ellipse(
        [margin, margin, size - margin - 1, size - margin - 1],
        fill=(255, 215, 0, 255),
        outline=(200, 165, 0, 255),
        width=2,
    )
    # Inner shine
    cx, cy = size // 2, size // 2
    r = size // 6
    draw.ellipse([cx - r, cy - r - 2, cx + r, cy + r - 2],
                 fill=(255, 240, 120, 180))
    return img


def draw_mushroom_tile(size: int) -> Image.Image:
    """Draw a mushroom shape."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Cap (red dome)
    cap_top = size // 6
    cap_bot = size * 5 // 8
    draw.ellipse([4, cap_top, size - 5, cap_bot + 2], fill=(220, 40, 40, 255),
                 outline=(160, 20, 20, 255), width=2)
    # White spots
    draw.ellipse([size // 3 - 2, cap_top + 3, size // 3 + 4, cap_top + 9],
                 fill=(255, 255, 255, 255))
    draw.ellipse([size * 2 // 3 - 4, cap_top + 5, size * 2 // 3 + 2, cap_top + 11],
                 fill=(255, 255, 255, 255))
    # Stem (beige)
    stem_l = size // 3
    stem_r = size * 2 // 3
    draw.rectangle([stem_l, cap_bot - 2, stem_r, size - 4],
                   fill=(240, 220, 180, 255), outline=(200, 180, 140, 255), width=1)
    return img


def draw_1up_tile(size: int) -> Image.Image:
    """Draw a 1-up mushroom (green)."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    cap_top = size // 6
    cap_bot = size * 5 // 8
    draw.ellipse([4, cap_top, size - 5, cap_bot + 2], fill=(40, 200, 60, 255),
                 outline=(20, 160, 40, 255), width=2)
    draw.ellipse([size // 3 - 2, cap_top + 3, size // 3 + 4, cap_top + 9],
                 fill=(255, 255, 255, 255))
    draw.ellipse([size * 2 // 3 - 4, cap_top + 5, size * 2 // 3 + 2, cap_top + 11],
                 fill=(255, 255, 255, 255))
    # "1" text on cap
    bbox = draw.textbbox((0, 0), "1")
    tw = bbox[2] - bbox[0]
    draw.text(((size - tw) // 2, cap_top + 2), "1", fill=(255, 255, 255, 220))
    stem_l = size // 3
    stem_r = size * 2 // 3
    draw.rectangle([stem_l, cap_bot - 2, stem_r, size - 4],
                   fill=(240, 220, 180, 255), outline=(200, 180, 140, 255), width=1)
    return img


def main() -> int:
    size = 32
    if len(sys.argv) >= 3 and sys.argv[1] == "--size":
        size = int(sys.argv[2])

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    generated = []

    # Coin block (? block)
    name, bg, sym, sym_c, outline = TILES[0]
    img = draw_block_tile(size, bg, sym, sym_c, outline)
    out = OUT_DIR / f"{name}.png"
    img.save(out)
    generated.append(out)

    # Empty block (used ? block)
    name, bg, sym, sym_c, outline = TILES[1]
    img = draw_block_tile(size, bg, " ", sym_c, outline)
    out = OUT_DIR / f"{name}.png"
    img.save(out)
    generated.append(out)

    # Coin
    img = draw_coin_tile(size)
    out = OUT_DIR / "tile_coin.png"
    img.save(out)
    generated.append(out)

    # Mushroom
    img = draw_mushroom_tile(size)
    out = OUT_DIR / "tile_mushroom.png"
    img.save(out)
    generated.append(out)

    # 1-up
    img = draw_1up_tile(size)
    out = OUT_DIR / "tile_1up.png"
    img.save(out)
    generated.append(out)

    for p in generated:
        print(f"[OK] {p}")

    print(f"\nGenerated {len(generated)} placeholder tiles in {OUT_DIR}")
    print("Run 'bash build.sh' to convert to .ps2tex and pack into sprites.pak")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
