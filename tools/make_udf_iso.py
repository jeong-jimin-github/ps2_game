#!/usr/bin/env python3
"""Build a PS2-compatible ISO 9660 + UDF bridge image from a staging directory.

Usage: python make_udf_iso.py <staging_dir> <output.iso>

Creates an ISO with both ISO 9660 Level 2 and UDF descriptors,
which is required for FHDB/ESR Patcher compatibility.
"""
import os
import sys

import pycdlib


def main():
    if len(sys.argv) not in (3, 5):
        print(f"Usage: {sys.argv[0]} <staging_dir> <output.iso> [vol_ident app_ident]")
        sys.exit(1)

    staging_dir = sys.argv[1]
    iso_path = sys.argv[2]
    vol_ident = "PS2GAME"
    app_ident = "PS2GAME"

    if len(sys.argv) == 5:
        vol_ident = sys.argv[3][:32]
        app_ident = sys.argv[4][:128]

    if not os.path.isdir(staging_dir):
        print(f"ERROR: staging directory not found: {staging_dir}")
        sys.exit(1)

    iso = pycdlib.PyCdlib()
    iso.new(
        interchange_level=2,
        sys_ident="PLAYSTATION",
        vol_ident=vol_ident,
        app_ident_str=app_ident,
        udf="2.60",
    )

    for fname in sorted(os.listdir(staging_dir)):
        fpath = os.path.join(staging_dir, fname)
        if not os.path.isfile(fpath):
            continue

        upper = fname.upper()
        iso_name = "/" + upper + ";1"
        udf_name = "/" + upper

        fsize = os.path.getsize(fpath)
        iso.add_fp(
            open(fpath, "rb"),
            fsize,
            iso_path=iso_name,
            udf_path=udf_name,
        )
        print(f"  {upper} ({fsize} bytes)")

    if os.path.exists(iso_path):
        os.remove(iso_path)

    iso.write(iso_path)
    iso.close()
    print(f"\nISO written: {iso_path}")


if __name__ == "__main__":
    main()
