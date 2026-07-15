[CmdletBinding()]
param(
    [ValidateSet('x64', 'x86', 'arm64')]
    [string]$Architecture = 'x64',
    [string]$Configuration = 'Release',
    [string]$Version = '0.1.0'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$binary = Join-Path $repoRoot "out\build\$Architecture\bin\$Configuration\NppThemes.dll"
if (-not (Test-Path -LiteralPath $binary)) {
    throw "Plugin binary not found: $binary"
}

$artifactRoot = Join-Path $repoRoot "artifacts\NppThemes-$Version-$Architecture"
$archive = "$artifactRoot.zip"
$resolvedRepo = [System.IO.Path]::GetFullPath($repoRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar)
$resolvedArtifact = [System.IO.Path]::GetFullPath($artifactRoot)
if (-not $resolvedArtifact.StartsWith($resolvedRepo + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to stage package outside repository: $resolvedArtifact"
}
if (Test-Path -LiteralPath $artifactRoot) {
    Remove-Item -LiteralPath $artifactRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $artifactRoot -Force | Out-Null
Copy-Item -LiteralPath $binary -Destination (Join-Path $artifactRoot 'NppThemes.dll') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'README.md') -Destination (Join-Path $artifactRoot 'README.md') -Force
Copy-Item -LiteralPath (Join-Path $repoRoot 'LICENSE') -Destination (Join-Path $artifactRoot 'LICENSE') -Force

if (Test-Path -LiteralPath $archive) {
    Remove-Item -LiteralPath $archive -Force
}
Compress-Archive -Path (Join-Path $artifactRoot '*') -DestinationPath $archive -CompressionLevel Optimal
$hash = Get-FileHash -LiteralPath $archive -Algorithm SHA256
Remove-Item -LiteralPath $artifactRoot -Recurse -Force

[pscustomobject]@{
    Archive = $archive
    SHA256 = $hash.Hash
}
