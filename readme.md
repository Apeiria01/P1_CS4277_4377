Team member: Yean Luo

# Project 2 Implementing Stack Canaries in an LLVM IR Pass for Windows x64

## Implementation Setting
System: Windows x64
LLVM: LLVM release package at [https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.4/clang+llvm-22.1.4-x86_64-pc-windows-msvc.tar.xz](https://github.com/llvm/llvm-project/releases/download/llvmorg-22.1.4/clang+llvm-22.1.4-x86_64-pc-windows-msvc.tar.xz) for compiling subjects.
C/C++ compiler: MSVC vc 145 with Visual Studio 2026 for compiling IR pass plugin.

# Deliverable
- IR Pass plugin in `./pass`, with binary at Release section.
- Test Scripts. Run `python make_input_[xx].py` to generate malicious stack overflow input and then run `python run_test.py Test_[Test Name]` to build and run tests.
- Report: here.
- Presentation Video: [https://youtu.be/ZMzHj2VqHiQ](https://youtu.be/ZMzHj2VqHiQ)

## Build Instructions
Run `cmake -B build -G "Visual Studio 18 2026" ` in "pass" directory to generate VS solution file to compile IR Pass (`InstrumentFunctions.dll`).
For tests, run `python run_test.py Test_[Test Name]` to build and run tests. It would compile the Test associated and generate `Test_[Test Name]2.exe` as a instrumented version and `Test_[Test Name].exe` not protected.
For running IR pass in command line, run:
```powershell
.\make-llvm-ir.ps1 .\TestPrintCanary.cpp TestPrintCanary.ll
.\plug.ps1 .\TestPrintCanary.ll .\TestPrintCanary2.ll
D:\llvm-project\b\RelWithDebInfo\bin\llc.exe -mtriple=x86_64-pc-windows-msvc ./TestPrintCanary2.ll
.\compile.ps1 .\TestPrintCanary2.s ./TestPrintCanary2.exe
```
While there are some hard coded path, change `D:\llvm-project\b\RelWithDebInfo\bin\llc.exe` to your `llc` path in your LLVM installation. For `make-llvm-ir.ps1`, `plug.ps1` and `compile.ps1`, change the `$llvmRoot` literal to your LLVM installation path. Before running plug, change `$Plugin` to the path to `InstrumentFunctions.dll`.

## Project Summary

This project implements a simplified stack-canary defense by hand, matching Project 2 of the CS4377 final project options. The defense is implemented as an LLVM IR module pass. The pass instruments functions so that a random process-wide guard value is generated at program startup, copied into each instrumented function's stack frame, and checked before the function leaves. If a stack overwrite corrupts the local canary value, the instrumented program reports the corruption and traps before returning through a potentially overwritten control-flow target.

The implementation began from the LLVM pass framework used in CS8395 for a Linux x86 stack-canary assignment. That original framework was designed for a Linux x86 environment and for the CS8395 test cases. For this project, the target was changed to Windows x64 by compiling LLVM IR through:

```powershell
llc.exe -mtriple=x86_64-pc-windows-msvc
```

The main technical finding of the project is that an LLVM IR implementation that works on Linux x86 does not automatically provide a correct stack-canary layout on Windows x64. At the IR level, the pass inserts a canary `alloca`, stores the global guard into it, and checks the value before function exits. 
  
However, any backend may reorder frame objects during code generation. In the test of vulnerable functions on Windows x64, the generated stack layout placed the canary outside the intended position between the vulnerable local buffer and the saved return/control data. As a result, an overflow could corrupt data without touching canary.

## Threat Model

The target attack class is stack-based memory corruption in C and C++ programs. The expected vulnerable pattern is a function with a fixed-size local stack buffer and an unchecked or incorrectly bounded copy operation, such as:

- `strcpy(buffer, input)`
- `strncpy(buffer, input, len)` with attacker-controlled `len`
- `memcpy(buffer, input, len)` with attacker-controlled `len`
- `memmove(buffer, input, len)` with attacker-controlled `len`
- `sprintf(buffer, "%s", input)`
- a manually written loop that copies attacker-controlled bytes into a fixed-size buffer

The attacker's goal is to provide input that writes beyond the local buffer and corrupts nearby stack state. In the strongest version of the attack, the overflow reaches a saved return address or other control-flow-sensitive value and redirects execution to an attacker-chosen function. The test programs include a `hijacked` function so that control-flow redirection can be observed clearly during testing.

The intended protection goal is to detect stack corruption before the corrupted function returns. This project does not attempt to stop all memory corruption. It focuses on detecting overwrites that cross from a vulnerable local buffer toward protected function-control data.

## Starting Point and Porting Goal

The starting point was the CS8395 LLVM pass framework for implementing stack canaries on Linux x86. 
I extended my own implementation of that CS8395 one, to achieve:

1. Create a random canary at program startup.
2. Store the canary in a global guard variable.
3. Insert a local stack slot for the canary in each function prologue.
4. Copy the global guard into the local stack slot.
5. Before each function exit point, compare the stack canary with the global guard.
6. If the comparison fails, print a diagnostic and terminate.

That approach would pass the Kali Linux x86 tests because the resulting generated stack layout places the canary in the intended protective position for those test cases.

For this CS4377 project, the same conceptual defense was ported to Windows x64. The target triple was changed to:

```text
llc.exe -mtriple=x86_64-pc-windows-msvc
```



## Implementation

The pass is implemented as an LLVM module pass. Its main implementation file is:

```text
pass/lib/InstrumentFunctions.cpp
```

The pass performs instrumentation at the LLVM IR level before lowering to Windows x64 assembly.



### Per-Function Instrumentation

For each defined function, the pass skips declarations and the canary initialization function itself. It then inserts a function-local canary slot and store the global canary in it.
```LLVM
%5 = alloca [12 x i8], align 1
  %6 = alloca i64, align 8
  %"__cs4277_canary_?vulnerable@@YAHPEBD_K@Z" = alloca i64, align 8
  %canary.guard = load volatile i64, ptr @__cs4277_canary_guard, align 8
  store volatile i64 %canary.guard, ptr %"__cs4277_canary_?vulnerable@@YAHPEBD_K@Z", align 8
  %canary.guard.ptr = inttoptr i64 %canary.guard to ptr
  call void @llvm.stackprotector(ptr %canary.guard.ptr, ptr %"__cs4277_canary_?vulnerable@@YAHPEBD_K@Z")
```

### Canary Alloc Control
The pass used `Intrinsic::stackprotector` to emit ir like:
```LLVM
call void @llvm.stackprotector(ptr %canary.guard.ptr, ptr %"__cs4277_canary_?vulnerable@@YAHPEBD_K@Z")
``` 
It mark a frame slot to preserve some order so that it would be allocated exactly between the local buffer and the saved return/control data.


### Exit-Point Checks

The pass collects function terminators and inserts checks before them. The check reloads the local canary and the global guard:

```llvm
%canary.load = load volatile i64, ptr %"__cs4277_canary_?vulnerable@@YAHPEBD_K@Z", align 8, !dbg !78
  %canary.expected = load volatile i64, ptr @__cs4277_canary_guard, align 8, !dbg !78
  %canary.corrupt = icmp ne i64 %canary.load, %canary.expected, !dbg !78
```

If the comparison fails, the pass calls `printf` with a message naming the function and then calls `llvm.trap`:

```text
Stack canary corrupted before leaving <function>
```

The intended behavior is that a corrupted function cannot return normally after a canary overwrite.

## Build and Test Pipeline / Results

The project uses the following workflow:

1. Generate a selected test input file.
2. Compile the C or C++ test program to LLVM IR.
3. Run the LLVM pass plugin over the IR.
4. Lower both the original and instrumented IR files to Windows x64 assembly using `llc.exe`.
5. Compile the assembly to Windows executables.
6. Compare behavior between the original and instrumented programs.

The test suite includes vulnerable programs for:

- `Test_strcpy`
- `Test_strncpy`
- `Test_memcpy`
- `Test_memmove`
- `Test_sprintf`
- `Test_loop`

Each test program uses a fixed-size stack buffer and attacker-controlled input read from an `input` file.

Run `.\run_test.py [Test Name]` to run the build and test.

The input file is generated by small Python helper scripts so that the same vulnerable program can be tested against benign input, ordinary stack overflows, and control-flow hijacking payloads. These scripts write bytes to `input` by default and can also write to a selected output path with `-o` / `--output`.

- `make_input_good.py` generates a short benign string (`hello`) that should not overflow the local stack buffer.
- `make_input_offbyone.py` generates a boundary-style input that reaches the end of the 12-byte buffer and adds one more byte.
- `make_input_small_overflow.py`, `make_input_medium_overflow.py`, and `make_input_large_overflow.py` generate progressively larger overflow payloads. These are used to check whether the instrumented binary detects stack corruption before returning.
- `make_input_pattern.py` generates a recognizable ASCII pattern of configurable length. This is useful when inspecting crashes or generated stack layouts because overwritten bytes can be matched back to input offsets.
- `make_input_touch_return.py` derives a payload length from generated assembly and creates an input large enough to reach the return-address area for a selected function.
- `make_input_controlflow_hijack_instrumented.py` generates a control-flow hijacking payload for the instrumented binary. It takes the target function address with `--addr`, writes that address repeatedly in little-endian form after a configurable prefix, and can redirect execution to another function such as `hijacked` when the vulnerable program's stack layout allows the overwrite.

In our experiments, the instrumented binary correctly triggered failure when stack is corrupted while the original binary (compiled with -fno-stack-protector) does not.




## Limitations

The most important limitation is stack-slot placement. The pass inserts a canary alloca, but the Windows x64 backend decides where that frame object appears relative to buffers, spills, saved registers, shadow space, and unwind-related layout constraints. This can break the core canary invariant.

The randomness source is also simplified. `llvm.readcyclecounter` provides runtime variation, but it is not a cryptographically strong random source. A production canary should use operating-system randomness or a hardened runtime-provided guard.

The pass instruments all defined functions in the module, including helper functions emitted from headers. This is useful for broad coverage, but it can add unnecessary overhead and make generated IR noisier than needed. A more selective implementation would instrument only functions with vulnerable stack objects.




## Conclusion

This project successfully implements the main mechanics of a stack-canary defense in an LLVM IR pass: startup guard initialization, per-function stack canary allocation, exit checks, and trap-based failure handling. 

