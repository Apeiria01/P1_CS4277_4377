param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Source,

    [Parameter(Position = 1)]
    [string]$Output
)

$ErrorActionPreference = "Stop"

$llvmRoot = if ([string]::IsNullOrWhiteSpace($env:LLVM_DIR)) {
    "D:\clang+llvm-22.1.4-x86_64-pc-windows-msvc"
} else {
    $env:LLVM_DIR
}

$clang = Join-Path $llvmRoot "bin\clang.exe"
if (-not (Test-Path -LiteralPath $clang)) {
    throw "clang.exe not found: $clang"
}

$clangArgs = @(
    "-target", "x86_64-pc-windows-msvc",
    "-emit-llvm",
    "-S",
    "-fno-stack-protector",
    "-g",
    $Source
)

if (-not [string]::IsNullOrWhiteSpace($Output)) {
    $clangArgs += @("-o", $Output)
}

& $clang @clangArgs
exit $LASTEXITCODE
