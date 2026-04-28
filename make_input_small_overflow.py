#!/usr/bin/env python3
import argparse
from pathlib import Path


parser = argparse.ArgumentParser()
parser.add_argument("-o", "--output", default="input")
args = parser.parse_args()

payload = b"A" * 32 + b"\x00"
Path(args.output).write_bytes(payload)
print(f"wrote {len(payload)} bytes to {Path(args.output).resolve()}")
