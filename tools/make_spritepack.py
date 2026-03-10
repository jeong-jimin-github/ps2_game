#!/usr/bin/env python3
"""Pack all .ps2tex files into a single SPRITES.PAK for PS2 CD loading.

Format:
  4 bytes: magic 0x4B505332 ("2SPK" LE)
  4 bytes: entry count (uint32 LE)
  Per entry (40 bytes each):
    32 bytes: filename (uppercase, null-padded)
    4 bytes:  data offset (from start of data section)
    4 bytes:  data size
  Data section: raw .ps2tex blobs concatenated
"""
import os, sys, struct, glob

MAGIC = 0x4B505332  # "2SPK" little-endian
ENTRY_NAME_SIZE = 32
ENTRY_SIZE = ENTRY_NAME_SIZE + 4 + 4  # 40 bytes

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input_dir> <output.pak>")
        sys.exit(1)

    input_dir = sys.argv[1]
    output_path = sys.argv[2]

    files = sorted(glob.glob(os.path.join(input_dir, "*.ps2tex")))
    if not files:
        print("No .ps2tex files found in", input_dir)
        sys.exit(1)

    entries = []
    all_data = bytearray()
    for f in files:
        name = os.path.basename(f).upper()
        data = open(f, "rb").read()
        entries.append((name, len(all_data), len(data)))
        all_data.extend(data)

    count = len(entries)

    with open(output_path, "wb") as out:
        # Header
        out.write(struct.pack("<II", MAGIC, count))
        # Index
        for name, offset, size in entries:
            name_bytes = name.encode("ascii")[:ENTRY_NAME_SIZE - 1]
            name_padded = name_bytes + b'\x00' * (ENTRY_NAME_SIZE - len(name_bytes))
            out.write(name_padded)
            out.write(struct.pack("<II", offset, size))
        # Data
        out.write(all_data)

    total_size = os.path.getsize(output_path)
    print(f"[OK] Packed {count} sprites into {output_path} ({total_size} bytes)")

if __name__ == "__main__":
    main()
