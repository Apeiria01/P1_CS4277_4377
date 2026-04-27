param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Source,

    [Parameter(Position = 1)]
    [string]$Output,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ClangArgs
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

$sourceExt = [System.IO.Path]::GetExtension($Source).ToLowerInvariant()
$usesClangCodegen = $sourceExt -in @(".c", ".cc", ".cpp", ".cxx", ".c++", ".i", ".ii")

$clangArgsList = @(
    "-target", "x86_64-pc-windows-msvc"
)

if ($usesClangCodegen) {
    $clangArgsList += "-fno-stack-protector"
}

$clangArgsList += @(
    "-g",
    $Source
)

if (-not [string]::IsNullOrWhiteSpace($Output)) {
    $clangArgsList += @("-o", $Output)
}

if ($ClangArgs) {
    $clangArgsList += $ClangArgs
}

& $clang @clangArgsList
exit $LASTEXITCODE
