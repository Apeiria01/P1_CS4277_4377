#!/usr/bin/env python3
import argparse
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
LLC = Path("D:\\llvm-project\\b\\RelWithDebInfo\\bin\\llc.exe")
POWERSHELL = [
    "powershell.exe",
    "-NoProfile",
    "-ExecutionPolicy",
    "Bypass",
    "-File",
]


def find_source(test_dir, test_name):
    for ext in (".c", ".cpp", ".cc", ".cxx"):
        source = test_dir / f"{test_name}{ext}"
        if source.exists():
            return source
    raise FileNotFoundError(f"no source file found for {test_name} in {test_dir}")


def run(command, cwd, checked=True):
    print(">", " ".join(str(part) for part in command))
    subprocess.run(command, cwd=str(cwd), check=checked)


def copy_input(source, test_dir):
    source = Path(source)
    if not source.is_absolute():
        source = ROOT / source

    if not source.exists():
        raise FileNotFoundError(f"input file not found: {source}")

    destination = test_dir / "input"
    shutil.copyfile(source, destination)
    print(f"copied {source} -> {destination}")


def run_test(test_name, input_file, copy_input_file):
    test_dir = ROOT / test_name
    if not test_dir.is_dir():
        raise FileNotFoundError(f"test directory not found: {test_dir}")

    if not LLC.exists():
        raise FileNotFoundError(f"llc.exe not found: {LLC}")

    if copy_input_file:
        copy_input(input_file, test_dir)

    source = find_source(test_dir, test_name)
    ll_file = f"{test_name}.ll"
    instrumented_ll = f"{test_name}2.ll"
    asm_file_orig = f"{test_name}.s"
    exe_file_orig = f"{test_name}.exe"
    asm_file = f"{test_name}2.s"
    exe_file = f"{test_name}2.exe"

    run(
        [
            *POWERSHELL,
            str(ROOT / "make-llvm-ir.ps1"),
            f".\\{source.name}",
            ll_file,
        ],
        test_dir,
    )
    run(
        [
            *POWERSHELL,
            str(ROOT / "plug.ps1"),
            f".\\{ll_file}",
            f".\\{instrumented_ll}",
        ],
        test_dir,
    )
    run(
        [
            str(LLC),
            "-mtriple=x86_64-pc-windows-msvc",
            f"./{ll_file}",
        ],
        test_dir,
    )
    run(
        [
            *POWERSHELL,
            str(ROOT / "compile.ps1"),
            f".\\{asm_file_orig}",
            f"./{exe_file_orig}",
        ],
        test_dir,
    )
    run(
        [
            str(LLC),
            "-mtriple=x86_64-pc-windows-msvc",
            f"./{instrumented_ll}",
        ],
        test_dir,
    )
    run(
        [
            *POWERSHELL,
            str(ROOT / "compile.ps1"),
            f".\\{asm_file}",
            f"./{exe_file}",
        ],
        test_dir,
    )
    print("Running instrumented subject:")
    run(
        [
            str(test_dir / exe_file),
        ],
        test_dir,
        False
    )
    print("Running original subject:")
    run(
        [
            str(test_dir / exe_file_orig),
        ],
        test_dir,
        False
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("test", help="test directory name, for example Test_memmove")
    parser.add_argument(
        "--input",
        default="input",
        help="input file copied into the test directory, default: ./input",
    )
    parser.add_argument(
        "--no-copy-input",
        action="store_true",
        help="do not copy input into the test directory",
    )
    args = parser.parse_args()

    run_test(args.test, args.input, not args.no_copy_input)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as error:
        raise SystemExit(error.returncode)
    except Exception as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
