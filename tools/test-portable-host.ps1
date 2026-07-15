[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [ValidateSet('x64', 'x86')]
    [string]$Architecture = 'x64',
    [string]$NotepadPlusPlusRoot = 'C:\Program Files\Notepad++'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$hostRoot = Join-Path $repoRoot "out\test-host-$Architecture"
$pluginSource = Join-Path $repoRoot "out\build\$Architecture\bin\$Configuration\NppThemes.dll"
$notepadSource = Join-Path $NotepadPlusPlusRoot 'notepad++.exe'

if (-not (Test-Path -LiteralPath $pluginSource)) {
    throw "Plugin binary not found: $pluginSource"
}
if (-not (Test-Path -LiteralPath $notepadSource)) {
    throw "Notepad++ executable not found: $notepadSource"
}

$resolvedRepo = [System.IO.Path]::GetFullPath($repoRoot).TrimEnd([System.IO.Path]::DirectorySeparatorChar)
$resolvedHost = [System.IO.Path]::GetFullPath($hostRoot)
if (-not $resolvedHost.StartsWith($resolvedRepo + [System.IO.Path]::DirectorySeparatorChar,
        [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to use host test directory outside repository: $resolvedHost"
}
if (Test-Path -LiteralPath $hostRoot) {
    Remove-Item -LiteralPath $hostRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $hostRoot -Force | Out-Null
Get-ChildItem -LiteralPath $NotepadPlusPlusRoot -Force |
    Where-Object Name -ne 'plugins' |
    Copy-Item -Destination $hostRoot -Recurse -Force
New-Item -ItemType File -Path (Join-Path $hostRoot 'doLocalConf.xml') -Force | Out-Null

$pluginRoot = Join-Path $hostRoot 'plugins\NppThemes'
New-Item -ItemType Directory -Path $pluginRoot -Force | Out-Null
Copy-Item -LiteralPath $pluginSource -Destination (Join-Path $pluginRoot 'NppThemes.dll') -Force

$logPath = Join-Path $hostRoot 'plugins\Config\NppThemes\NppThemes.log'
$existingLogLength = if (Test-Path -LiteralPath $logPath) { (Get-Item -LiteralPath $logPath).Length } else { 0 }
$notepad = Join-Path $hostRoot 'notepad++.exe'
$process = Start-Process -FilePath $notepad -ArgumentList '-multiInst', '-nosession' -PassThru -WindowStyle Hidden

$ready = $false
$deadline = [DateTime]::UtcNow.AddSeconds(15)
while ([DateTime]::UtcNow -lt $deadline) {
    Start-Sleep -Milliseconds 200
    if (Test-Path -LiteralPath $logPath) {
        $log = Get-Content -Raw -LiteralPath $logPath
        if ((Get-Item -LiteralPath $logPath).Length -gt $existingLogLength -and $log.Contains('NppThemes ready')) {
            $ready = $true
            break
        }
    }
    if ($process.HasExited) {
        break
    }
}

if (-not $process.HasExited) {
    $null = $process.CloseMainWindow()
    if (-not $process.WaitForExit(5000)) {
        Stop-Process -Id $process.Id -Force
    }
}

if (-not $ready) {
    throw "NppThemes did not report ready state. Check $logPath"
}

[pscustomobject]@{
    Status = 'pass'
    HostVersion = (Get-Item -LiteralPath $notepad).VersionInfo.FileVersion
    Plugin = Join-Path $pluginRoot 'NppThemes.dll'
    Log = $logPath
}
