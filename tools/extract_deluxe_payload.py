"""Recover the private BS Deluxe payload from a built executable that embeds it.

Builds compile the payload into the executable (tools/embed_payload.py). If the ignored
captures/bs-deluxe copy is lost, this finds the BSDELX1 payload inside an executable,
reads it to its exact end from the header and record table, and verifies it by applying
it to the stock ROM and checking the pinned USA Deluxe SHA-256. Nothing is written unless
that check passes.

    py -3 tools/extract_deluxe_payload.py <exe> [--stock fzero.sfc] [--out captures/bs-deluxe/mods/bs-deluxe.dat]
"""
import argparse
import hashlib
import struct
from pathlib import Path

from import_bs_deluxe import DELUXE_SHA256

ROOT = Path(__file__).resolve().parents[1]
MAGIC = b"BSDELX1\0"
HEADER = 8 + 8 + 32 + 32


def payload_at(blob, start):
    target_len, records = struct.unpack_from("<II", blob, start + 8)
    pos = start + HEADER
    for _ in range(records):
        offset, length = struct.unpack_from("<II", blob, pos)
        pos += 8 + length
        if pos > len(blob):
            raise ValueError("record table runs past the end of the file")
    return blob[start:pos], target_len


def apply(payload, stock, target_len):
    rom = bytearray(stock + bytes(max(0, target_len - len(stock))))[:target_len]
    _, records = struct.unpack_from("<II", payload, 8)
    pos = HEADER
    for _ in range(records):
        offset, length = struct.unpack_from("<II", payload, pos)
        rom[offset:offset + length] = payload[pos + 8:pos + 8 + length]
        pos += 8 + length
    return bytes(rom)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("exe", type=Path)
    p.add_argument("--stock", type=Path, default=ROOT / "fzero.sfc")
    p.add_argument("--out", type=Path, default=ROOT / "captures/bs-deluxe/mods/bs-deluxe.dat")
    a = p.parse_args()
    blob = a.exe.read_bytes()
    stock = a.stock.read_bytes()
    if len(stock) == 524800:  # copier header
        stock = stock[512:]
    start = blob.find(MAGIC)
    while start >= 0:
        try:
            payload, target_len = payload_at(blob, start)
            if hashlib.sha256(apply(payload, stock, target_len)).hexdigest() == DELUXE_SHA256:
                a.out.parent.mkdir(parents=True, exist_ok=True)
                a.out.write_bytes(payload)
                print(f"Recovered {len(payload)} bytes at offset {start}; Deluxe ROM SHA-256 verified -> {a.out}")
                return 0
        except (ValueError, struct.error):
            pass
        start = blob.find(MAGIC, start + 1)
    raise SystemExit("No verified BS Deluxe payload found in " + str(a.exe))


if __name__ == "__main__":
    raise SystemExit(main())
