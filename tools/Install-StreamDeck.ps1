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
$receiptName = 'install-receipt.json'
$transactionFiles = @($owned) + $receiptName
function HashOrNull([string]$path) { if (Test-Path -LiteralPath $path -PathType Leaf) { (Get-FileHash -LiteralPath $path).Hash } else { $null } }
function Restore-Files([string]$from, $state) {
    # Fixed owned paths only; validate every backup before the first mutation.
    foreach ($name in $transactionFiles) {
        $entry = @($state.files | Where-Object { $_.name -ceq $name })
        if ($entry.Count -ne 1) { throw "Invalid backup inventory for $name" }
        if ($entry[0].existed -and (HashOrNull (Join-Path $from $name)) -cne $entry[0].sha256) { throw "Backup hash differs: $name" }
    }
    foreach ($name in $transactionFiles) {
        $entry = @($state.files | Where-Object { $_.name -ceq $name })[0]
        $destination = Join-Path $Target $name
        if ($entry.existed) { Copy-Item -LiteralPath (Join-Path $from $name) -Destination $destination -Force }
        elseif (Test-Path -LiteralPath $destination) { Remove-Item -LiteralPath $destination -Force }
        if ((HashOrNull $destination) -cne $entry.sha256) { throw "Restoration did not verify: $name" }
    }
}
if (Get-Process | Where-Object { $_.Name -like 'FZeroSNESRecomp*' }) { throw 'F-Zero is running; close it first. Nothing changed.' }
if (-not (Test-Path -LiteralPath (Join-Path $Target $launcher))) { throw "Not the F-Zero Stream Deck folder: $Target" }
$backups = Join-Path $Target 'deployment-backups'

if ($Rollback) {
    if ($Rollback -notmatch '^\d{4}-\d{2}-\d{2}-[a-z0-9][a-z0-9-]*$') { throw 'Invalid backup name.' }
    $from = Join-Path $backups $Rollback
    if (-not (Test-Path -LiteralPath $from)) { throw "No backup $from" }
    $statePath = Join-Path $from 'backup-state.json'
    if (Test-Path -LiteralPath $statePath) {
        $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
        if ($state.schemaVersion -ne 1) { throw 'Unknown backup schema.' }
        Restore-Files $from $state
    } else {
        # Historical backups did not retain their receipt. Withdraw the current
        # capability rather than attach it to the older runtime.
        if (Test-Path -LiteralPath (Join-Path $Target $receiptName)) {
            Copy-Item -LiteralPath (Join-Path $Target $receiptName) -Destination (Join-Path $from ('withdrawn-receipt-' + [guid]::NewGuid().ToString('N') + '.json'))
        }
        foreach ($name in $owned) {
            $source = Join-Path $from $name
            if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination (Join-Path $Target $name) -Force }
        }
        if (Test-Path -LiteralPath (Join-Path $Target $receiptName)) { Remove-Item -LiteralPath (Join-Path $Target $receiptName) }
    }
    "restored runtime and receipt state from $Rollback (settings were not changed)"
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
$nativePin = @(Get-Content -LiteralPath (Join-Path $repo 'lib/toolkit/MANIFEST.txt') |
    Where-Object { $_ -match '^[0-9A-Fa-f]{64}\s+native/WheelFfb\.dll\s*$' })
if ($nativePin.Count -ne 1 -or -not (Test-Path -LiteralPath $sources['WheelFfb.dll'])) {
    throw 'Missing or ambiguous native pin/build runtime. Nothing changed.'
}
$nativeHash = ($nativePin[0] -split '\s+')[0]
if ((Get-FileHash -LiteralPath $sources['WheelFfb.dll']).Hash -ine $nativeHash) {
    throw 'Built WheelFfb.dll differs from the source pin; rebuild before installing. Nothing changed.'
}

# The configured header alone is insufficient: reconfiguration can leave an old
# executable behind. Require the same clean revision in the linked image too.
$description = (git -C $repo describe --always --dirty --abbrev=12).Trim()
$header = Join-Path $BuildDir 'generated/fzero_build.h'
if (!(Test-Path -LiteralPath $header) -or
    (Get-Content -LiteralPath $header -Raw) -notmatch ('(?m)^#define FZERO_SOURCE_REVISION "' + [regex]::Escape($description) + '"\r?$') -or
    ![Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($built)).Contains($description + [char]0)) {
    throw 'Build source stamp is stale or absent; reconfigure and rebuild from this clean commit. Nothing changed.'
}
$changes = @($owned | Where-Object { (HashOrNull $sources[$_]) -cne (HashOrNull (Join-Path $Target $_)) })
$oldReceiptPath = Join-Path $Target $receiptName
if (-not $changes.Count -and (Test-Path -LiteralPath $oldReceiptPath)) {
    try { $oldReceipt = Get-Content -LiteralPath $oldReceiptPath -Raw | ConvertFrom-Json } catch { $oldReceipt = $null }
    if ($oldReceipt.schemaVersion -eq 1 -and $oldReceipt.controlsProfileSchema -eq 1 -and
        $oldReceipt.controlsProfileAdapter -ceq 'fzero-raw-wheel-1' -and $oldReceipt.sourceCommit -ceq $commit -and
        $oldReceipt.launcherSha256 -ceq (HashOrNull $built) -and $oldReceipt.wheelFfbSha256 -ceq $nativeHash.ToUpperInvariant()) {
        'The installed files and controls receipt already match this build; nothing changed.'; return
    }
}

$stamp = (Get-Date -Format yyyy-MM-dd) + '-' + $Label
$backup = Join-Path $backups $stamp
if (Test-Path -LiteralPath $backup) { throw "Backup $stamp exists; choose another -Label. Nothing changed." }
New-Item -ItemType Directory -Path $backup | Out-Null
$lines = @("Backup before installing $Label from $commit ($(Get-Date -Format s)).")
$state = [ordered]@{ schemaVersion = 1; files = @($transactionFiles | ForEach-Object {
    $hash = HashOrNull (Join-Path $Target $_)
    [ordered]@{ name = $_; existed = ($null -ne $hash); sha256 = $hash }
}) }
foreach ($entry in $state.files) {
    if ($entry.existed) {
        Copy-Item -LiteralPath (Join-Path $Target $entry.name) -Destination (Join-Path $backup $entry.name)
        if ((HashOrNull (Join-Path $backup $entry.name)) -cne $entry.sha256) { throw 'Backup did not verify; nothing installed.' }
    }
}
$state | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $backup 'backup-state.json') -Encoding utf8
foreach ($name in 'config.ini', 'fzero-video.ini') { if (Test-Path -LiteralPath (Join-Path $Target $name)) { Copy-Item -LiteralPath (Join-Path $Target $name) -Destination (Join-Path $backup $name) } }
Set-Content -LiteralPath (Join-Path $backup 'README.txt') -Value ($lines + "Roll back: .\tools\Install-StreamDeck.ps1 -Rollback $stamp") -Encoding utf8

try {
foreach ($name in $changes) {
    Copy-Item -LiteralPath $sources[$name] -Destination (Join-Path $Target $name) -Force
    if ((Get-FileHash -LiteralPath $sources[$name]).Hash -ne (Get-FileHash -LiteralPath (Join-Path $Target $name)).Hash) { throw "Copy of $name did not verify; roll back with -Rollback $stamp" }
}
[ordered]@{
    schemaVersion = 1; controlsProfileSchema = 1; controlsProfileAdapter = 'fzero-raw-wheel-1'
    launcher = $launcher; launcherSha256 = (HashOrNull (Join-Path $Target $launcher)); wheelFfbSha256 = (HashOrNull (Join-Path $Target 'WheelFfb.dll'))
    label = $Label; sourceCommit = $commit; installedUtc = [DateTime]::UtcNow.ToString('o'); buildDir = $BuildDir; backup = $stamp
    files = @($owned | ForEach-Object { [ordered]@{ name = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $Target $_)).Hash } })
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $Target $receiptName) -Encoding utf8
} catch {
    $failure = $_
    Restore-Files $backup $state
    throw $failure
}
"installed $($changes -join ', ') from $commit; backup deployment-backups\$stamp; receipt install-receipt.json"
