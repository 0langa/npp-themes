[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('x64', 'x86', 'arm64')]
    [string]$Architecture,
    [string]$Version = '0.1.0'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$archive = Join-Path $repoRoot "artifacts\NppThemes-$Version-$Architecture.zip"
if (-not (Test-Path -LiteralPath $archive)) {
    throw "Package archive not found: $archive"
}

$testRoot = Join-Path $repoRoot "out\package-test\$Architecture"
$resolvedRepo = [System.IO.Path]::GetFullPath($repoRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar)
$resolvedTestRoot = [System.IO.Path]::GetFullPath($testRoot)
if (-not $resolvedTestRoot.StartsWith($resolvedRepo + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to use package test directory outside repository: $resolvedTestRoot"
}
if (Test-Path -LiteralPath $testRoot) {
    Remove-Item -LiteralPath $testRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $testRoot -Force | Out-Null

try {
    Expand-Archive -LiteralPath $archive -DestinationPath $testRoot
    $files = @(Get-ChildItem -LiteralPath $testRoot -File | Select-Object -ExpandProperty Name | Sort-Object)
    $expected = @('LICENSE', 'NppThemes.dll', 'README.md')
    if (Compare-Object $files $expected) {
        throw "Archive root differs from required files: $($files -join ', ')"
    }
    if (Get-ChildItem -LiteralPath $testRoot -Directory) {
        throw 'Archive must not contain nested directories'
    }

    $dll = Join-Path $testRoot 'NppThemes.dll'
    $stream = [System.IO.File]::OpenRead($dll)
    try {
        $reader = [System.IO.BinaryReader]::new($stream)
        if ($reader.ReadUInt16() -ne 0x5A4D) {
            throw 'DLL has invalid DOS signature'
        }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadUInt32()
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) {
            throw 'DLL has invalid PE signature'
        }
        $machine = $reader.ReadUInt16()
    } finally {
        $stream.Dispose()
    }

    $machines = @{ x86 = 0x014C; x64 = 0x8664; arm64 = 0xAA64 }
    if ($machine -ne $machines[$Architecture]) {
        throw ('DLL machine 0x{0:X4} does not match {1}' -f $machine, $Architecture)
    }
    $fileVersion = (Get-Item -LiteralPath $dll).VersionInfo.FileVersion
    if (-not $fileVersion.StartsWith($Version, [System.StringComparison]::Ordinal)) {
        throw "DLL version '$fileVersion' does not start with package version '$Version'"
    }
    $hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
    [pscustomobject]@{
        Archive = $archive
        Architecture = $Architecture
        Version = $Version
        SHA256 = $hash
        Result = 'Pass'
    }
} finally {
    if (Test-Path -LiteralPath $testRoot) {
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}
