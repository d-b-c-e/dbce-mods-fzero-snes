<#
.SYNOPSIS
    Installs a built F-Zero launcher into the Stream Deck / BigBox copy, with a backup and a receipt; or rolls back.

.DESCRIPTION
    The Stream Deck key runs Launch-FZeroRecomp.bat, which starts FZeroSNESRecomp-wheel-launcher.exe from the
    LaunchBox Racing folder. This copies the build's FZeroSNESRecomp.exe there under that name, and WheelFfb.dll when it
    differs. The files it replaces go to deployment-backups\<date>-<label>\ first, with a README naming the source commit
    and hashes; install-receipt.json records what was installed. config.ini, fzero-video.ini, keybinds.ini, saves,
    the ROM and every other file are never touched. Refuses while the game is running.

    The build must be the configured integration build (build-merge: BS Deluxe embedded). Build it with
    `cmd /c "call vcvars64.bat && cmake --build build-merge"` and run `ctest --test-dir build-merge` first.

.EXAMPLE
    .\tools\Install-StreamDeck.ps1 -Label ground-filter
    .\tools\Install-StreamDeck.ps1 -Rollback 2026-10-07-ground-filter
#>
[CmdletBinding()]
param(
    [string]$BuildDir = (Join-Path (Split-Path $PSScriptRoot -Parent) 'build-merge'),
    [string]$Target = 'E:\Source\toolkits\launchbox\Launchbox-Racing\Games\Windows\F-Zero (SNES Recomp)',
    [ValidatePattern('^[a-z0-9][a-z0-9-]*$')][string]$Label,
    [string]$Rollback
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$launcher = 'FZeroSNESRecomp-wheel-launcher.exe'
$owned = @($launcher, 'WheelFfb.dll')
if (Get-Process | Where-Object { $_.Name -like 'FZeroSNESRecomp*' }) { throw 'F-Zero is running; close it first. Nothing changed.' }
if (-not (Test-Path -LiteralPath (Join-Path $Target $launcher))) { throw "Not the F-Zero Stream Deck folder: $Target" }
$backups = Join-Path $Target 'deployment-backups'

if ($Rollback) {
    $from = Join-Path $backups $Rollback
    if (-not (Test-Path -LiteralPath $from)) { throw "No backup $from" }
    foreach ($name in $owned) {
        $source = Join-Path $from $name
        if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination (Join-Path $Target $name) -Force; "restored $name from $Rollback" }
    }
    return
}

if (-not $Label) { throw 'Name this install with -Label (lower-case words and hyphens).' }
$built = Join-Path $BuildDir 'FZeroSNESRecomp.exe'
if (-not (Test-Path -LiteralPath $built)) { throw "No build at $built" }
$cache = Join-Path $BuildDir 'CMakeCache.txt'
if (-not (Test-Path -LiteralPath $cache) -or -not (Select-String -LiteralPath $cache -Pattern '^FZERO_DELUXE_DATA_FILE:FILEPATH=.+' -Quiet)) { throw 'This build directory does not embed BS Deluxe; the installed launcher does. Nothing changed.' }
$commit = (git -C $repo rev-parse HEAD).Trim()
$dirty = @(git -C $repo status --porcelain --untracked-files=no)
if ($dirty.Count) { throw 'Commit the source first: the receipt names the commit the build came from.' }
$sources = @{ $launcher = $built; 'WheelFfb.dll' = (Join-Path $BuildDir 'WheelFfb.dll') }

$changes = @($owned | Where-Object { (Test-Path -LiteralPath $sources[$_]) -and (Get-FileHash -LiteralPath $sources[$_]).Hash -ne (Get-FileHash -LiteralPath (Join-Path $Target $_)).Hash })
if (-not $changes.Count) { 'The installed files already match this build; nothing changed.'; return }

$stamp = (Get-Date -Format yyyy-MM-dd) + '-' + $Label
$backup = Join-Path $backups $stamp
if (Test-Path -LiteralPath $backup) { throw "Backup $stamp exists; choose another -Label. Nothing changed." }
New-Item -ItemType Directory -Path $backup | Out-Null
$lines = @("Backup before installing $Label from $commit ($(Get-Date -Format s)).")
foreach ($name in $changes) {
    Copy-Item -LiteralPath (Join-Path $Target $name) -Destination (Join-Path $backup $name)
    $lines += "$name previous SHA-256 $((Get-FileHash -LiteralPath (Join-Path $backup $name)).Hash)"
}
foreach ($name in 'config.ini', 'fzero-video.ini') { if (Test-Path -LiteralPath (Join-Path $Target $name)) { Copy-Item -LiteralPath (Join-Path $Target $name) -Destination (Join-Path $backup $name) } }
Set-Content -LiteralPath (Join-Path $backup 'README.txt') -Value ($lines + "Roll back: .\tools\Install-StreamDeck.ps1 -Rollback $stamp") -Encoding utf8

foreach ($name in $changes) {
    Copy-Item -LiteralPath $sources[$name] -Destination (Join-Path $Target $name) -Force
    if ((Get-FileHash -LiteralPath $sources[$name]).Hash -ne (Get-FileHash -LiteralPath (Join-Path $Target $name)).Hash) { throw "Copy of $name did not verify; roll back with -Rollback $stamp" }
}
[ordered]@{
    label = $Label; sourceCommit = $commit; installedUtc = [DateTime]::UtcNow.ToString('o'); buildDir = $BuildDir; backup = $stamp
    files = @($owned | ForEach-Object { [ordered]@{ name = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $Target $_)).Hash } })
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $Target 'install-receipt.json') -Encoding utf8
"installed $($changes -join ', ') from $commit; backup deployment-backups\$stamp; receipt install-receipt.json"
