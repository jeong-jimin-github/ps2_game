#!/bin/bash
# PS2 ISO image builder
# Usage: bash tools/make_iso.sh [output.iso]
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

ELF_NAME="game_engine.elf"
ELF_FILE="$PROJECT_DIR/$ELF_NAME"
ISO_FILE="${1:-$PROJECT_DIR/game.iso}"
STAGING="$PROJECT_DIR/.iso_staging"

# --- check tools ---
ISOTOOL=""
if command -v mkisofs &>/dev/null; then
    ISOTOOL="mkisofs"
elif command -v genisoimage &>/dev/null; then
    ISOTOOL="genisoimage"
elif command -v xorriso &>/dev/null; then
    ISOTOOL="xorriso"
else
    echo "ERROR: mkisofs, genisoimage, or xorriso not found."
    echo ""
    echo "Install on MSYS2:  pacman -S xorriso"
    echo "Install on Debian: sudo apt install genisoimage"
    echo "Install on macOS:  brew install cdrtools"
    exit 1
fi

if [ ! -f "$ELF_FILE" ]; then
    echo "ERROR: $ELF_FILE not found. Run 'make' first."
    exit 1
fi

# --- prepare staging dir (FLAT: everything in root) ---
rm -rf "$STAGING"
mkdir -p "$STAGING"

# SYSTEM.CNF (CR+LF line endings required by PS2 BIOS)
ELF_UPPER="$(echo "$ELF_NAME" | tr 'a-z' 'A-Z')"
printf "BOOT2 = cdrom0:\\\\%s;1\r\nVER = 1.00\r\nVMODE = NTSC\r\n" "$ELF_UPPER" \
    > "$STAGING/SYSTEM.CNF"

# ELF (uppercase filename to match SYSTEM.CNF)
cp "$ELF_FILE" "$STAGING/$ELF_UPPER"

# IOP modules (uppercase)
for irx in audsrv.irx freesd.irx; do
    if [ -f "$PROJECT_DIR/$irx" ]; then
        UPPER="$(echo "$irx" | tr 'a-z' 'A-Z')"
        cp "$PROJECT_DIR/$irx" "$STAGING/$UPPER"
    fi
done

# Sprite pack (single file containing all .ps2tex)
if [ -f "$PROJECT_DIR/sprites.pak" ]; then
    cp "$PROJECT_DIR/sprites.pak" "$STAGING/SPRITES.PAK"
else
    echo "WARNING: sprites.pak not found. Run 'make assets' first."
fi

# BGM audio (flat into root)
for f in "$PROJECT_DIR/assets"/*.wav; do
    if [ -f "$f" ]; then
        BASE="$(basename "$f" | tr 'a-z' 'A-Z')"
        cp "$f" "$STAGING/$BASE"
    fi
done

# Levels (flat into root)
for f in "$PROJECT_DIR/levels"/*.txt; do
    if [ -f "$f" ]; then
        BASE="$(basename "$f" | tr 'a-z' 'A-Z')"
        cp "$f" "$STAGING/$BASE"
    fi
done

echo "=== Staging contents ==="
find "$STAGING" -type f | sort | while read -r f; do
    echo "  ${f#$STAGING/}"
done
echo ""

# Remove existing ISO to avoid device-busy errors
if [ -f "$ISO_FILE" ]; then
    rm -f "$ISO_FILE"
fi

# --- build ISO ---
# -iso-level 2  : allow filenames up to 31 chars (required for PS2TEX names)
# -D             : allow deep directory nesting without relocation
# -no-pad        : don't pad to 300 sectors (saves CD space)
# No -R/-r/-J    : pure ISO 9660 only (no Rock Ridge / Joliet extensions)
#                  PS2 CDVDFSV only reads ISO 9660 standard entries
if [ "$ISOTOOL" = "xorriso" ]; then
    xorriso -as mkisofs \
        -iso-level 2 \
        -D \
        -sysid "PLAYSTATION" \
        -V "PS2GAME" \
        -A "PS2GAME" \
        -no-pad \
        -o "$ISO_FILE" \
        "$STAGING" 2>&1
else
    "$ISOTOOL" \
        -iso-level 2 \
        -D \
        -sysid "PLAYSTATION" \
        -V "PS2GAME" \
        -A "PS2GAME" \
        -no-pad \
        -o "$ISO_FILE" \
        "$STAGING" 2>&1
fi

rm -rf "$STAGING"

echo ""
echo "=== ISO created: $ISO_FILE ==="
echo ""
echo "Test in PCSX2:  File -> Run ISO"
echo "Burn to CD-R:   imgburn / cdrecord at 4x-8x speed"
