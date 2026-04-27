param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$SourceLl,

    [Parameter(Mandatory = $true, Position = 1)]
    [string]$DestinationLl,

    [Parameter(Position = 2)]
    [string]$Plugin
)

$ErrorActionPreference = "Stop"

$llvmRoot = if ([string]::IsNullOrWhiteSpace($env:LLVM_DIR)) {
    "D:\clang+llvm-22.1.4-x86_64-pc-windows-msvc"
} else {
    $env:LLVM_DIR
}

if ([string]::IsNullOrWhiteSpace($Plugin)) {
    $Plugin = if ([string]::IsNullOrWhiteSpace($env:PLUGIN)) {
        Join-Path $PSScriptRoot "pass.\build\bin\RELWITHDEBINFO\InstrumentFunctions.dll"
    } else {
        $env:PLUGIN
    }
}

$opt = Join-Path $llvmRoot "bin\opt.exe"
if (-not (Test-Path -LiteralPath $opt)) {
    throw "opt.exe not found: $opt"
}

if (-not (Test-Path -LiteralPath $Plugin)) {
    throw "Pass plugin not found: $Plugin"
}

& $opt -load-pass-plugin $Plugin "--passes=cs4277-p1" $SourceLl -S -o $DestinationLl
exit $LASTEXITCODE
