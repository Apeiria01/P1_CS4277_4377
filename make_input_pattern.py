#!/usr/bin/env python3
import argparse
from pathlib import Path


parser = argparse.ArgumentParser()
parser.add_argument("-o", "--output", default="input")
parser.add_argument("-n", "--length", type=int, default=128)
args = parser.parse_args()

payload = b"".join(f"{i:08X}".encode("ascii") for i in range((args.length + 7) // 8))
payload = payload[: args.length] + b"\x00"
Path(args.output).write_bytes(payload)
print(f"wrote {len(payload)} bytes to {Path(args.output).resolve()}")
