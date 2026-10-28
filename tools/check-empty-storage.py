#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Check logical enrollment state, not physical erasure of flash metadata.

Firmware 3014 retains INFO identity bytes after acknowledged CLEAR, even when
the fresh COUNT is zero. Those bytes cannot establish an active enrollment.
This checker only reads a previously saved, independent post-reset snapshot.
"""
import argparse
from pathlib import Path
import sys

try:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path)
    parser.add_argument("--clear-ack", type=Path)
    args = parser.parse_args()
    root = args.snapshot
    if args.clear_ack and args.clear_ack.read_bytes() != b"\x40\x00":
        raise ValueError("The saved HP CLEAR acknowledgment is not 40 00.")
    if (root / "count.bin").read_bytes() != b"\x40\x00":
        raise ValueError("COUNT is not zero; empty storage was not confirmed.")
    retained = []
    for slot in range(10):
        path = root / f"snapshot-slot-{slot}.bin"
        if path.exists():
            record = path.read_bytes()
            if (len(record) == 64 and record[:2] == b"\x40\x00" and
                    record[3:] not in (b"\xff" * 61, b"\x00" * 61)):
                retained.append(slot)
    print("ZERO ACTIVE ENROLLMENTS CONFIRMED: fresh COUNT is 40 00.")
    if args.clear_ack:
        print("The preceding HP CLEAR was acknowledged with 40 00.")
    if retained:
        print("Retained inactive INFO metadata at slots:", *retained)
    print("This does not prove physical erasure of flash or recovery of templates.")
except (OSError, ValueError) as error:
    print(f"CLEAR VERIFICATION FAILED: {error}", file=sys.stderr)
    raise SystemExit(1)
