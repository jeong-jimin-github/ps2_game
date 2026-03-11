#!/bin/bash
# PS2 ISO image builder
# Usage: bash tools/make_iso.sh [output.iso]
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

ELF_NAME="game_engine.elf"
ELF_FILE="$PROJECT_DIR/$ELF_NAME"
STAGING="$PROJECT_DIR/.iso_staging"

GAME_ID_RAW="${PS2_GAME_ID:-SLUS_209.99}"
GAME_TITLE_RAW="${PS2_GAME_TITLE:-PS2 PLATFORMER}"
GAME_ID="$(echo "$GAME_ID_RAW" | tr '[:lower:]' '[:upper:]' | tr -cd 'A-Z0-9_.')"
GAME_TITLE_OPL="$(echo "$GAME_TITLE_RAW" | tr '[:lower:]' '[:upper:]' | tr -cs 'A-Z0-9' '_' | sed -e 's/^_\+//' -e 's/_\+$//')"

if ! [[ "$GAME_ID" =~ ^[A-Z]{4}_[0-9]{3}\.[0-9]{2}$ ]]; then
    echo "WARNING: PS2_GAME_ID '$GAME_ID_RAW' is invalid. Using default SLUS_209.99"
    GAME_ID="SLUS_209.99"
fi

if [ -z "$GAME_TITLE_OPL" ]; then
    GAME_TITLE_OPL="PS2_PLATFORMER"
fi

VOL_LABEL="${GAME_ID}_${GAME_TITLE_OPL}"
VOL_LABEL="$(printf '%.32s' "$VOL_LABEL")"
ISO_FILE="${1:-$PROJECT_DIR/${GAME_ID}.${GAME_TITLE_OPL}.iso}"

# --- check tools ---
ISOTOOL=""
PYTHON=""

# Check for Python (needed for UDF ISO builder)
for py in python3 python \
    "/c/Users/$USER/AppData/Local/Programs/Python/Python310/python.exe" \
    "/c/Users/$USER/AppData/Local/Programs/Python/Python311/python.exe" \
    "/c/Users/$USER/AppData/Local/Programs/Python/Python312/python.exe"; do
    if command -v "$py" &>/dev/null || [ -x "$py" ]; then
        if "$py" -c "import pycdlib" &>/dev/null 2>&1; then
            PYTHON="$py"
            break
        fi
    fi
done

if [ -n "$PYTHON" ]; then
    ISOTOOL="pycdlib"
elif command -v mkisofs &>/dev/null; then
    ISOTOOL="mkisofs"
elif command -v genisoimage &>/dev/null; then
    ISOTOOL="genisoimage"
elif command -v xorriso &>/dev/null; then
    ISOTOOL="xorriso"
else
    echo "ERROR: No ISO tool found."
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
ELF_UPPER="$GAME_ID"
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

# BGM audio only (exclude sfx_*.wav — SFX uses .adpcm files)
for f in "$PROJECT_DIR/assets"/bgm*.wav; do
    if [ -f "$f" ]; then
        BASE="$(basename "$f" | tr 'a-z' 'A-Z')"
        cp "$f" "$STAGING/$BASE"
    fi
done

# SFX ADPCM files (flat into root)
for f in "$PROJECT_DIR/assets"/*.adpcm; do
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

# Font files only (.fnt + font .ps2tex) — sprite textures are in SPRITES.PAK
# Do NOT copy all .ps2tex files; too many root entries breaks PS2 CDVDFSV
for f in "$PROJECT_DIR/assets"/*.fnt; do
    if [ -f "$f" ]; then
        BASE="$(basename "$f" | tr 'a-z' 'A-Z')"
        [ ! -f "$STAGING/$BASE" ] && cp "$f" "$STAGING/$BASE"
    fi
done
for f in "$PROJECT_DIR/assets"/font*.ps2tex; do
    if [ -f "$f" ]; then
        BASE="$(basename "$f" | tr 'a-z' 'A-Z')"
        [ ! -f "$STAGING/$BASE" ] && cp "$f" "$STAGING/$BASE"
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
# -pad           : pad to multiples of 150 sectors (required for real CD-R)
# --norock       : disable Rock Ridge extensions (PS2 CDVDFSV only reads ISO 9660,
#                  RR SUSP data bloats directory entries causing them to overflow
#                  into sector 2 which CDVDFSV cannot read)
# No -R/-r/-J    : pure ISO 9660 only (no Rock Ridge / Joliet extensions)
#                  PS2 CDVDFSV only reads ISO 9660 standard entries
if [ "$ISOTOOL" = "pycdlib" ]; then
    # Python pycdlib: ISO 9660 Level 2 + UDF (ESR Patcher compatible)
    echo "Using pycdlib (ISO 9660 + UDF)..."
    "$PYTHON" "$SCRIPT_DIR/make_udf_iso.py" "$STAGING" "$ISO_FILE" "$VOL_LABEL" "$VOL_LABEL"
elif [ "$ISOTOOL" = "xorriso" ]; then
    echo "WARNING: xorriso cannot produce UDF. ESR Patcher may not work."
    echo "         Install pycdlib: pip install pycdlib"
    xorriso -as mkisofs \
        --norock \
        -iso-level 2 \
        -D \
        -sysid "PLAYSTATION" \
        -V "$VOL_LABEL" \
        -A "$VOL_LABEL" \
        -pad \
        -o "$ISO_FILE" \
        "$STAGING" 2>&1
else
    "$ISOTOOL" \
        -iso-level 2 \
        -udf \
        -D \
        -sysid "PLAYSTATION" \
        -V "$VOL_LABEL" \
        -A "$VOL_LABEL" \
        -pad \
        -o "$ISO_FILE" \
        "$STAGING" 2>&1
fi

rm -rf "$STAGING"

echo ""
echo "=== ISO created: $ISO_FILE ==="
echo "Game ID      : $GAME_ID"
echo "Game Title   : $GAME_TITLE_OPL"
echo "Volume Label : $VOL_LABEL"
echo ""
echo "Test in PCSX2:  File -> Run ISO"
echo "Burn to CD-R:   imgburn / cdrecord at 4x-8x speed"
