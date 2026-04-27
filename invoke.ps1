param(
    [switch]$Debug,

    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Program,

    [Parameter(Mandatory = $true, Position = 1)]
    [string]$InputFile,

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ProgramArgs
)

$ErrorActionPreference = "Stop"

if ($InputFile -ne "input") {
    Copy-Item -LiteralPath $InputFile -Destination (Join-Path (Get-Location) "input") -Force
}

$programPath = (Resolve-Path -LiteralPath $Program).Path

if ($Debug) {
    $gdb = Get-Command "gdb.exe" -ErrorAction SilentlyContinue
    if (-not $gdb) {
        throw "gdb.exe not found in PATH"
    }

    $gdbPath = $gdb.Source
    & $gdbPath --args $programPath @ProgramArgs
} else {
    & $programPath @ProgramArgs
}

exit $LASTEXITCODE
