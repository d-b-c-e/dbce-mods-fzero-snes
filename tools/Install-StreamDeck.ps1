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

    -PrivateSharedInput additionally owns the private shim DLL, header, PIN and provenance from the existing
    vendor inventory. It declares ordinary Shared recording only after their hashes verify; it never selects
    the Shared backend or changes force settings. Shared recording still requires ForceFeedback Enabled=0.

.EXAMPLE
    .\tools\Install-StreamDeck.ps1 -Label ground-filter
    .\tools\Install-StreamDeck.ps1 -Rollback 2026-10-07-ground-filter
#>
[CmdletBinding()]
param(
    [string]$BuildDir,
    [string]$Target = 'E:\Source\toolkits\launchbox\Launchbox-Racing\Games\Windows\F-Zero (SNES Recomp)',
    [ValidatePattern('^[a-z0-9][a-z0-9-]*$')][string]$Label,
    [string]$Rollback,
    [switch]$PrivateSharedInput
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
if (!$BuildDir) { $BuildDir = Join-Path $repo 'build-merge' }
$launcher = 'FZeroSNESRecomp-wheel-launcher.exe'
$sharedOwned = @('dbce_shim.dll', 'shared-input/dbce_shim.h', 'shared-input/PIN.json', 'shared-input/PROVENANCE.md', 'shared-input/dbce_shim_extras.h')
$owned = @($launcher, 'WheelFfb.dll', 'tools/Recording.ps1') + @(if ($PrivateSharedInput) { $sharedOwned })
$receiptName = 'install-receipt.json'
$transactionFiles = @($owned) + $receiptName
$restorableFiles = @($launcher, 'WheelFfb.dll', 'tools/Recording.ps1', $receiptName) + $sharedOwned
function HashOrNull([string]$path) { if (Test-Path -LiteralPath $path -PathType Leaf) { (Get-FileHash -LiteralPath $path).Hash } else { $null } }
function Recording-Capability([bool]$PrivateShared, [string]$LinkedText) {
    if ($PrivateShared -and (!$LinkedText.Contains('--isolate-owner-state') -or !$LinkedText.Contains('[fzero-isolation]'))) {
        throw 'Built Shared recording lacks ordinary owner-state isolation. Reconfigure and rebuild from the clean source before installing.'
    }
    [ordered]@{ schema='fzero.session-capability@1'; protocol='fzero.session-control@1'; companion='tools/Recording.ps1'
        record=$true; replay=$true; stopShortcut='Ctrl+F11'; automatedRecord=$PrivateShared
        recordBackends=@('Native') + @(if ($PrivateShared) { 'Shared' })
        automatedRecordBackends=@(if ($PrivateShared) { 'Shared' }) }
}
function Assert-SharedOwnershipTransition([bool]$Requested, $PreviousReceipt, $IncomingPrivatePaths=$null) {
    if (!$PreviousReceipt) { return }
    $privatePaths = @('dbce_shim.dll', 'shared-input/dbce_shim.h', 'shared-input/PIN.json', 'shared-input/PROVENANCE.md', 'shared-input/dbce_shim_extras.h')
    $ownedPrivate = @($PreviousReceipt.files | Where-Object { $_.name -and $_.name.Replace('\','/') -in $privatePaths })
    if ($Requested) {
        if ($null -eq $IncomingPrivatePaths) { return } # source pin is checked later
        $withdrawn = @($ownedPrivate | Where-Object { $_.name.Replace('\','/') -cnotin $IncomingPrivatePaths })
        $lostExtras = $PreviousReceipt.privateSharedInput.extrasAbi -eq 1 -and 'shared-input/dbce_shim_extras.h' -cnotin $IncomingPrivatePaths
        if ($withdrawn.Count -or $lostExtras) {
            throw 'The previous private Shared install owns files absent from this source pin. Keep the extras-capable private pin or roll back that Shared install first; a legacy downgrade cannot withdraw ownership while leaving files behind. Nothing changed.'
        }
        return
    }
    if ($null -ne $PreviousReceipt.privateSharedInput -or $ownedPrivate.Count) {
        throw 'The previous install owns private Shared files. Keep -PrivateSharedInput or roll back that Shared install first; Native installation cannot withdraw ownership while leaving its files behind. Nothing changed.'
    }
}
function Private-SharedSources([string]$SourceRoot, [string]$BuildRoot) {
    # Same private-only files and metadata as stage_unified.private_shared_files;
    # the ordinary installer retains its own Deluxe/source identity checks.
    $vendor = Join-Path $SourceRoot 'vendor/dbce-shim'
    $pinPath = Join-Path $vendor 'PIN.json'
    if (!(Test-Path -LiteralPath $pinPath -PathType Leaf) -or (Get-Item -LiteralPath $pinPath).Length -gt 65536) { throw 'Missing private Shared source pin.' }
    $pin = Get-Content -LiteralPath $pinPath -Raw | ConvertFrom-Json
    $keys = @($pin.files.PSObject.Properties.Name)
    $extras = 'dbce_shim_extras.h' -cin $keys
    if ($pin.schema -cne 'dbce.fzero-shared-input-pin' -or $pin.version -ne 1 -or $pin.abi -ne 2 -or
        $pin.distribution -cne 'private-only' -or $pin.sourceRevision -cnotmatch '^[0-9a-f]{40}$' -or
        $keys.Count -ne $(if ($extras) { 3 } else { 2 }) -or 'dbce_shim.dll' -cnotin $keys -or 'dbce_shim.h' -cnotin $keys -or
        $(if ($extras) { $pin.extrasAbi -ne 1 } else { $null -ne $pin.extrasAbi })) { throw 'Invalid private Shared source pin.' }
    $files = @{'dbce_shim.dll'=(Join-Path $BuildRoot 'dbce_shim.dll'); 'shared-input/dbce_shim.h'=(Join-Path $vendor 'dbce_shim.h')
        'shared-input/PIN.json'=$pinPath; 'shared-input/PROVENANCE.md'=(Join-Path $vendor 'PROVENANCE.md')}
    if ($extras) { $files['shared-input/dbce_shim_extras.h'] = Join-Path $vendor 'dbce_shim_extras.h' }
    foreach ($name in $keys) {
        $key = if ($name -ceq 'dbce_shim.dll') { $name } else { 'shared-input/' + $name }
        $expected = $pin.files.$name
        if ($expected -cnotmatch '^[0-9a-f]{64}$' -or (HashOrNull $files[$key]) -ine $expected) { throw "Private Shared source/build pin differs: $name" }
    }
    foreach ($path in $files.Values) {
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing private Shared source: $path" }
        $current = [IO.Path]::GetFullPath($path)
        while ($current) {
            if ((Get-Item -Force -LiteralPath $current).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Private Shared sources cannot traverse links.' }
            $current = [IO.Path]::GetDirectoryName($current)
        }
    }
    $metadata=[ordered]@{ abi=2; distribution='private-only'; sourceRevision=$pin.sourceRevision; runtimeSha256=$pin.files.'dbce_shim.dll' }
    if ($extras) { $metadata.extrasAbi=1 }
    @{ files=$files; metadata=$metadata }
}
function Restore-Files([string]$from, $state) {
    # Fixed owned paths only; validate every backup before the first mutation.
    # Earlier backups predate the companion. Restore only their inventory;
    # their old receipt cannot authorize a leftover companion for execution.
    $restoreFiles = @($state.files | ForEach-Object { $_.name })
    if (@($restoreFiles | Select-Object -Unique).Count -ne $restoreFiles.Count -or
        @($restoreFiles | Where-Object { $_ -cnotin $restorableFiles }).Count -or
        @($launcher,'WheelFfb.dll',$receiptName | Where-Object { $_ -cnotin $restoreFiles }).Count) { throw 'Invalid backup ownership inventory.' }
    foreach ($name in $restoreFiles) {
        $entry = @($state.files | Where-Object { $_.name -ceq $name })
        if ($entry.Count -ne 1) { throw "Invalid backup inventory for $name" }
        if ($entry[0].existed -and (HashOrNull (Join-Path $from $name)) -cne $entry[0].sha256) { throw "Backup hash differs: $name" }
    }
    foreach ($name in $restoreFiles) {
        $entry = @($state.files | Where-Object { $_.name -ceq $name })[0]
        $destination = Join-Path $Target $name
        if ($entry.existed) {
            [void][IO.Directory]::CreateDirectory((Split-Path -Parent $destination))
            Copy-Item -LiteralPath (Join-Path $from $name) -Destination $destination -Force
        }
        elseif (Test-Path -LiteralPath $destination) { Remove-Item -LiteralPath $destination -Force }
        if ((HashOrNull $destination) -cne $entry.sha256) { throw "Restoration did not verify: $name" }
    }
}
if (Get-Process | Where-Object { $_.Name -like 'FZeroSNESRecomp*' }) { throw 'F-Zero is running; close it first. Nothing changed.' }
if (-not (Test-Path -LiteralPath (Join-Path $Target $launcher))) { throw "Not the F-Zero Stream Deck folder: $Target" }
$checkedPaths = if ($Rollback) { $restorableFiles } else { $transactionFiles }
foreach ($name in @($checkedPaths) + 'deployment-backups') {
    $current = [IO.Path]::GetFullPath((Join-Path $Target $name))
    while ($current) {
        if ((Test-Path -LiteralPath $current) -and ((Get-Item -Force -LiteralPath $current).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Owned install/backup paths must not traverse links.' }
        $current = [IO.Path]::GetDirectoryName($current)
    }
}
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
$oldReceiptPath = Join-Path $Target $receiptName
$oldReceipt = if (Test-Path -LiteralPath $oldReceiptPath -PathType Leaf) { Get-Content -LiteralPath $oldReceiptPath -Raw | ConvertFrom-Json } else { $null }
Assert-SharedOwnershipTransition ([bool]$PrivateSharedInput) $oldReceipt
$built = Join-Path $BuildDir 'FZeroSNESRecomp.exe'
if (-not (Test-Path -LiteralPath $built)) { throw "No build at $built" }
$cache = Join-Path $BuildDir 'CMakeCache.txt'
if (-not (Test-Path -LiteralPath $cache) -or -not (Select-String -LiteralPath $cache -Pattern '^FZERO_DELUXE_DATA_FILE:FILEPATH=.+' -Quiet)) { throw 'This build directory does not embed BS Deluxe; the installed launcher does. Nothing changed.' }
$commit = (git -C $repo rev-parse HEAD).Trim()
$dirty = @(git -C $repo status --porcelain --untracked-files=no)
if ($dirty.Count) { throw 'Commit the source first: the receipt names the commit the build came from.' }
$sources = @{ $launcher = $built; 'WheelFfb.dll' = (Join-Path $BuildDir 'WheelFfb.dll'); 'tools/Recording.ps1' = (Join-Path $repo 'tools/Recording.ps1') }
$shared = $null
if ($PrivateSharedInput) {
    $shared = Private-SharedSources $repo $BuildDir
    Assert-SharedOwnershipTransition $true $oldReceipt @($shared.files.Keys)
    foreach ($name in $shared.files.Keys) { $sources[$name] = $shared.files[$name] }
    # Legacy four-file pins remain readable; a new extras pin owns five files.
    $owned = @($launcher, 'WheelFfb.dll', 'tools/Recording.ps1') + @($shared.files.Keys)
    $transactionFiles = @($owned) + $receiptName
}
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
$linkedText = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($built))
if (!(Test-Path -LiteralPath $header) -or
    (Get-Content -LiteralPath $header -Raw) -notmatch ('(?m)^#define FZERO_SOURCE_REVISION "' + [regex]::Escape($description) + '"\r?$') -or
    !$linkedText.Contains($description + [char]0) -or !$linkedText.Contains('[fzero-controls]') -or
    !$linkedText.Contains('fzero.session-status@1') -or ($PrivateSharedInput -and !$linkedText.Contains('[fzero-shim]'))) {
    throw 'Build source stamp is stale or absent; reconfigure and rebuild from this clean commit. Nothing changed.'
}
$sessionCapability = Recording-Capability ([bool]$PrivateSharedInput) $linkedText
$changes = @($owned | Where-Object { (HashOrNull $sources[$_]) -cne (HashOrNull (Join-Path $Target $_)) })
if (-not $changes.Count -and (Test-Path -LiteralPath $oldReceiptPath)) {
    $expectedBackends = if ($PrivateSharedInput) { 'Native,Shared' } else { 'Native' }
    $sharedMatches = if ($PrivateSharedInput) {
        $oldReceipt.privateSharedInput.abi -eq 2 -and $oldReceipt.privateSharedInput.distribution -ceq 'private-only' -and
        $oldReceipt.privateSharedInput.sourceRevision -ceq $shared.metadata.sourceRevision -and
        $oldReceipt.privateSharedInput.runtimeSha256 -ceq $shared.metadata.runtimeSha256 -and
        $oldReceipt.privateSharedInput.extrasAbi -eq $shared.metadata.extrasAbi
    } else { !$oldReceipt.privateSharedInput }
    $inventoryMatches = @($oldReceipt.files).Count -eq $owned.Count
    foreach ($name in $owned) {
        $entries = @($oldReceipt.files | Where-Object { $_.name -ceq $name })
        if ($entries.Count -ne 1 -or $entries[0].sha256 -cne (HashOrNull $sources[$name])) { $inventoryMatches = $false }
    }
    if ($oldReceipt.schemaVersion -eq 1 -and $oldReceipt.controlsProfileSchema -eq 1 -and
        $oldReceipt.controlsProfileAdapter -ceq 'fzero-raw-wheel-1' -and $oldReceipt.sourceCommit -ceq $commit -and
        $oldReceipt.launcherSha256 -ceq (HashOrNull $built) -and $oldReceipt.wheelFfbSha256 -ceq $nativeHash.ToUpperInvariant() -and
        $oldReceipt.session.schema -ceq 'fzero.session-capability@1' -and $oldReceipt.session.protocol -ceq 'fzero.session-control@1' -and
        $oldReceipt.session.automatedRecord -eq $sessionCapability.automatedRecord -and
        (@($oldReceipt.session.automatedRecordBackends) -join ',') -ceq (@($sessionCapability.automatedRecordBackends) -join ',') -and
        $inventoryMatches -and $sharedMatches -and (@($oldReceipt.session.recordBackends) -join ',') -ceq $expectedBackends) {
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
        [void][IO.Directory]::CreateDirectory((Split-Path -Parent (Join-Path $backup $entry.name)))
        Copy-Item -LiteralPath (Join-Path $Target $entry.name) -Destination (Join-Path $backup $entry.name)
        if ((HashOrNull (Join-Path $backup $entry.name)) -cne $entry.sha256) { throw 'Backup did not verify; nothing installed.' }
    }
}
$state | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $backup 'backup-state.json') -Encoding utf8
foreach ($name in 'config.ini', 'fzero-video.ini') { if (Test-Path -LiteralPath (Join-Path $Target $name)) { Copy-Item -LiteralPath (Join-Path $Target $name) -Destination (Join-Path $backup $name) } }
Set-Content -LiteralPath (Join-Path $backup 'README.txt') -Value ($lines + "Roll back: .\tools\Install-StreamDeck.ps1 -Rollback $stamp") -Encoding utf8

try {
foreach ($name in $changes) {
    [void][IO.Directory]::CreateDirectory((Split-Path -Parent (Join-Path $Target $name)))
    Copy-Item -LiteralPath $sources[$name] -Destination (Join-Path $Target $name) -Force
    if ((Get-FileHash -LiteralPath $sources[$name]).Hash -ne (Get-FileHash -LiteralPath (Join-Path $Target $name)).Hash) { throw "Copy of $name did not verify; roll back with -Rollback $stamp" }
}
$receipt = [ordered]@{
    schemaVersion = 1; controlsProfileSchema = 1; controlsProfileAdapter = 'fzero-raw-wheel-1'
    launcher = $launcher; launcherSha256 = (HashOrNull (Join-Path $Target $launcher)); wheelFfbSha256 = (HashOrNull (Join-Path $Target 'WheelFfb.dll'))
    label = $Label; sourceCommit = $commit; installedUtc = [DateTime]::UtcNow.ToString('o'); buildDir = $BuildDir; backup = $stamp
    session = $sessionCapability
    files = @($owned | ForEach-Object { [ordered]@{ name = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $Target $_)).Hash } })
}
if ($PrivateSharedInput) { $receipt.privateSharedInput = $shared.metadata }
$receipt | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $Target $receiptName) -Encoding utf8
} catch {
    $failure = $_
    Restore-Files $backup $state
    throw $failure
}
"installed $($changes -join ', ') from $commit; backup deployment-backups\$stamp; receipt install-receipt.json"
