#!/usr/bin/env python3
"""
Wrap a raw FDT blob with a Samsung DTB v2 header.

Usage: wrap-samsung-dtb.py <original-wrapped-dtb> <raw-fdt> <output>

The original file supplies the header template; only the two size fields
(total size at offset 0x04, FDT size at offset 0x20) are updated to match
the new payload.
"""
import struct
import sys

def main():
    if len(sys.argv) != 4:
        print(__doc__)
        sys.exit(1)

    orig_path, fdt_path, out_path = sys.argv[1:]

    with open(orig_path, "rb") as f:
        orig = f.read()
    with open(fdt_path, "rb") as f:
        fdt = f.read()

    if orig[:4] != b"\xd7\xb7\xab\x1e":
        print("ERROR: original DTB doesn't have Samsung magic (d7b7ab1e)")
        sys.exit(1)
    if fdt[:4] != b"\xd0\x0d\xfe\xed":
        print("ERROR: input FDT doesn't have standard FDT magic (d00dfeed)")
        sys.exit(1)

    header = bytearray(orig[:64])
    total_size = 64 + len(fdt)

    # Update size fields
    struct.pack_into(">I", header, 0x04, total_size)
    struct.pack_into(">I", header, 0x20, len(fdt))
    struct.pack_into(">I", header, 0x24, 64)

    with open(out_path, "wb") as f:
        f.write(header)
        f.write(fdt)

    print(f"Wrote {out_path}: header 64 + FDT {len(fdt)} = {total_size} bytes")

if __name__ == "__main__":
    main()
