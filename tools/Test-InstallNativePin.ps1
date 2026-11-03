# Dummy files only; never launches a game or loads WheelFfb.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$fixture = Join-Path $repo ('build/install-native-pin-' + [guid]::NewGuid().ToString('N'))
$build = Join-Path $fixture 'build'
$target = Join-Path $fixture 'target'
New-Item -ItemType Directory -Path $build,$target | Out-Null
New-Item -ItemType Directory -Path (Join-Path $build 'generated') | Out-Null
$installer = Join-Path $PSScriptRoot 'Install-StreamDeck.ps1'
$shell = (Get-Process -Id $PID).Path
function Check([bool]$condition,[string]$message) { if (!$condition) { throw $message }; $script:checks++ }
function Snapshot { (@(Get-ChildItem -LiteralPath $target -Recurse -File | Sort-Object FullName | ForEach-Object { $_.FullName + ':' + (Get-FileHash $_.FullName).Hash }) -join "`n") }
function Invoke-Installer {
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue' # PS5.1 surfaces expected native stderr as ErrorRecords.
        & $shell -NoProfile -File $installer @args 2>&1
        $script:installerExitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
}
$checks = 0
$actions = @('nativeX','nativeY','nativeL','nativeR','saveStateMenu','rewind')
[IO.File]::WriteAllText((Join-Path $build 'CMakeCache.txt'), 'FZERO_DELUXE_DATA_FILE:FILEPATH=dummy-fixture.dat')
[IO.File]::WriteAllText((Join-Path $build 'FZeroSNESRecomp.exe'), 'Non-executable new launcher fixture')
[IO.File]::WriteAllText((Join-Path $target 'FZeroSNESRecomp-wheel-launcher.exe'), 'Non-executable old launcher fixture')
[IO.File]::WriteAllText((Join-Path $target 'WheelFfb.dll'), 'Old native fixture')
[IO.File]::WriteAllText((Join-Path $target 'config.ini'), '[ForceFeedback] retained-owner-fixture')
[IO.File]::WriteAllText((Join-Path $target 'fzero-video.ini'), 'retained-video-fixture')
$before = Snapshot
$configHash = (Get-FileHash (Join-Path $target 'config.ini')).Hash
$videoHash = (Get-FileHash (Join-Path $target 'fzero-video.ini')).Hash
foreach ($kind in 'missing','wrong') {
    if ($kind -eq 'wrong') { [IO.File]::WriteAllText((Join-Path $build 'WheelFfb.dll'), 'Stale native fixture') }
    $log = Invoke-Installer -BuildDir $build -Target $target -Label fixture 2>&1
    Check ($installerExitCode -ne 0) "$kind runtime accepted"
    Check (($log -join "`n") -match 'native pin/build runtime|differs from the source pin') "$kind refused for wrong reason"
    Check ((Snapshot) -ceq $before) "$kind refusal changed target"
}
Copy-Item -LiteralPath (Join-Path $repo 'lib/toolkit/native/WheelFfb.dll') -Destination (Join-Path $build 'WheelFfb.dll')
$description = (git -C $repo describe --always --dirty --abbrev=12).Trim()
$header = Join-Path $build 'generated/fzero_build.h'
$exe = Join-Path $build 'FZeroSNESRecomp.exe'
foreach ($kind in 'missing-header','stale-header','stale-image') {
    if ($kind -eq 'stale-header') { [IO.File]::WriteAllText($header, '#define FZERO_SOURCE_REVISION "old"') }
    if ($kind -eq 'stale-image') { [IO.File]::WriteAllText($header, '#define FZERO_SOURCE_REVISION "' + $description + '"') }
    $log = Invoke-Installer -BuildDir $build -Target $target -Label fixture 2>&1
    Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'Build source stamp') "$kind was not refused for its stamp"
    Check ((Snapshot) -ceq $before) "$kind refusal changed target"
}
[IO.File]::WriteAllText($exe, 'Non-executable stamped fixture ' + $description + [char]0)
$log = Invoke-Installer -BuildDir $build -Target $target -Label fixture 2>&1
Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'Build source stamp') 'Pre-adapter image accepted'
Check ((Snapshot) -ceq $before) 'Pre-adapter refusal changed target'
[IO.File]::AppendAllText($exe, '[fzero-controls]')
$log = Invoke-Installer -BuildDir $build -Target $target -Label fixture 2>&1
Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'Build source stamp') 'Pre-session image accepted'
Check ((Snapshot) -ceq $before) 'Pre-session refusal changed target'
$stampedImage = [IO.File]::ReadAllText($exe) + 'fzero.session-status@1' + [char]0
# A fresh stamp and even a claimed receipt cannot grant a missing mapper action.
$claimedReceipt = Join-Path $target 'install-receipt.json'
foreach ($missing in $actions) {
    [IO.File]::WriteAllText($exe, $stampedImage + ((@($actions | Where-Object { $_ -cne $missing }) -join [char]0) + [char]0))
    [IO.File]::WriteAllText($claimedReceipt, (ConvertTo-Json -InputObject @{controlsProfileActions=$actions}))
    $beforeRefusal = Snapshot
    $log = Invoke-Installer -BuildDir $build -Target $target -Label fixture 2>&1
    Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'Native mapper lacks optional profile actions') "$missing mapper support was inferred from receipt"
    Check ((Snapshot) -ceq $beforeRefusal) "$missing mapper refusal changed target"
}
Remove-Item -LiteralPath $claimedReceipt
$supportedImage = $stampedImage + ($actions -join [char]0) + [char]0
$axisKeys = @('SteeringProfileBinding','AcceleratorProfileBinding','BrakeProfileBinding')
foreach ($missing in $axisKeys) {
    [IO.File]::WriteAllText($exe, $supportedImage + ((@($axisKeys | Where-Object { $_ -cne $missing }) -join [char]0) + [char]0))
    $beforeRefusal = Snapshot
    $log = Invoke-Installer -BuildDir $build -Target $target -Label fixture 2>&1
    Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'Native mapper lacks Schema 2') "$missing Schema 2 reader was inferred from receipt"
    Check ((Snapshot) -ceq $beforeRefusal) "$missing Schema 2 refusal changed target"
}
$supportedImage += ($axisKeys -join [char]0) + [char]0
foreach ($kind in 'mismatched-header','mismatched-image') {
    [IO.File]::WriteAllText($exe, $(if ($kind -eq 'mismatched-image') { $supportedImage.Replace($description, 'old-revision') } else { $supportedImage }))
    [IO.File]::WriteAllText($header, '#define FZERO_SOURCE_REVISION "' + $(if ($kind -eq 'mismatched-header') { 'old-revision' } else { $description }) + '"')
    $log = Invoke-Installer -BuildDir $build -Target $target -Label fixture 2>&1
    Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'Build source stamp') "$kind gained support from mapper strings alone"
    Check ((Snapshot) -ceq $before) "$kind refusal changed target"
}
[IO.File]::WriteAllText($exe, $supportedImage)
$log = Invoke-Installer -BuildDir $build -Target $target -Label fixture 2>&1
Check ($installerExitCode -eq 0) ('Exact native install failed: ' + ($log -join "`n"))
Check ((Get-FileHash (Join-Path $target 'WheelFfb.dll')).Hash -ceq (Get-FileHash (Join-Path $build 'WheelFfb.dll')).Hash) 'Native copy differs'
Check ((Get-FileHash (Join-Path $target 'config.ini')).Hash -ceq $configHash) 'Config changed'
Check ((Get-FileHash (Join-Path $target 'fzero-video.ini')).Hash -ceq $videoHash) 'Video changed'
$receipt = Get-Content (Join-Path $target 'install-receipt.json') -Raw | ConvertFrom-Json
Check ((Get-Content (Join-Path $target ('deployment-backups/' + $receipt.backup + '/WheelFfb.dll')) -Raw) -ceq 'Old native fixture') 'Native backup differs'
Check ($receipt.schemaVersion -eq 1 -and $receipt.controlsProfileSchema -eq 1 -and $receipt.controlsProfileAdapter -ceq 'fzero-raw-wheel-1') 'Capability absent'
Check ($receipt.nativeControls.schema -eq 2 -and $receipt.nativeControls.adapter -ceq 'fzero-raw-wheel-2' -and $receipt.nativeControls.consumer -ceq 'FZeroSNESRecomp-wheel-launcher.exe') 'Scoped Native Schema 2 capability differs'
Check ((ConvertTo-Json -InputObject $receipt.controlsProfileActions -Compress) -ceq '["nativeX","nativeY","nativeL","nativeR","saveStateMenu","rewind"]') 'Optional action array differs'
Check (!$receipt.privateSharedInput -and !$receipt.privateControlsSetup) 'Native receipt claims Shared support'
Check ($receipt.launcherSha256 -ceq (Get-FileHash $exe).Hash -and $receipt.wheelFfbSha256 -ceq (Get-FileHash (Join-Path $build 'WheelFfb.dll')).Hash) 'Capability hashes differ'
Check ($receipt.sourceCommit -ceq (git -C $repo rev-parse HEAD).Trim()) 'Source commit differs'
$installedReceiptHash = (Get-FileHash (Join-Path $target 'install-receipt.json')).Hash
$beforeNoop = Snapshot
$log = Invoke-Installer -BuildDir $build -Target $target -Label fixture 2>&1
Check ($installerExitCode -eq 0 -and ($log -join "`n") -match 'already match') 'Exact re-install not a no-op'
Check ((Get-FileHash (Join-Path $target 'install-receipt.json')).Hash -ceq $installedReceiptHash) 'No-op changed receipt'
Check ((Snapshot) -ceq $beforeNoop) 'No-op changed target or backups'
# Same payload and source: only an exact array may bypass the backup transaction.
$exactReceiptBytes = [IO.File]::ReadAllText($claimedReceipt)
foreach ($kind in 'missing','subset','case','extra','reordered','scalar','native-missing','native-future') {
    $prior = $exactReceiptBytes | ConvertFrom-Json
    switch ($kind) {
        'missing' { $prior.PSObject.Properties.Remove('controlsProfileActions') }
        'subset' { $prior.controlsProfileActions = @('nativeX') }
        'case' { $prior.controlsProfileActions = @('NativeX','nativeY','nativeL','nativeR','saveStateMenu','rewind') }
        'extra' { $prior.controlsProfileActions = $actions + 'future' }
        'reordered' { $prior.controlsProfileActions = @('rewind','saveStateMenu','nativeR','nativeL','nativeY','nativeX') }
        'scalar' { $prior.controlsProfileActions = $actions -join ',' }
        'native-missing' { $prior.PSObject.Properties.Remove('nativeControls') }
        'native-future' { $prior.nativeControls.schema = 3 }
    }
    $prior | Add-Member -NotePropertyName ownerMetadata -NotePropertyValue 'retain exact bytes'
    $priorBytes = ConvertTo-Json -InputObject $prior -Depth 4
    [IO.File]::WriteAllText($claimedReceipt, $priorBytes)
    $payloadHashes = @($prior.files | ForEach-Object { (Get-FileHash (Join-Path $target $_.name)).Hash }) -join ','
    $log = Invoke-Installer -BuildDir $build -Target $target -Label ('fixture-actions-' + $kind) 2>&1
    Check ($installerExitCode -eq 0 -and ($log -join "`n") -notmatch 'already match') "$kind actions receipt did not refresh"
    $refreshed = Get-Content -LiteralPath $claimedReceipt -Raw | ConvertFrom-Json
    Check ((ConvertTo-Json -InputObject $refreshed.controlsProfileActions -Compress) -ceq (ConvertTo-Json -InputObject $actions -Compress)) "$kind refresh actions differ"
    Check ($refreshed.nativeControls.schema -eq 2 -and $refreshed.nativeControls.adapter -ceq 'fzero-raw-wheel-2') "$kind Native capability did not refresh"
    Check ([IO.File]::ReadAllText((Join-Path $target ('deployment-backups/' + $refreshed.backup + '/install-receipt.json'))) -ceq $priorBytes) "$kind refresh lost previous receipt bytes"
    Check ((@($prior.files | ForEach-Object { (Get-FileHash (Join-Path $target $_.name)).Hash }) -join ',') -ceq $payloadHashes) "$kind receipt refresh changed payload"
    $log = Invoke-Installer -Target $target -Rollback $refreshed.backup 2>&1
    Check ($installerExitCode -eq 0 -and [IO.File]::ReadAllText($claimedReceipt) -ceq $priorBytes) "$kind refresh rollback lost previous receipt"
}
[IO.File]::WriteAllText($claimedReceipt, $exactReceiptBytes)
$log = Invoke-Installer -Target $target -Rollback $receipt.backup 2>&1
Check ($installerExitCode -eq 0) ('Rollback failed: ' + ($log -join "`n"))
Check (!(Test-Path (Join-Path $target 'install-receipt.json'))) 'Rollback retained absent-before receipt'
Check ((Get-Content (Join-Path $target 'FZeroSNESRecomp-wheel-launcher.exe') -Raw) -ceq 'Non-executable old launcher fixture') 'Rollback launcher differs'
Check ((Get-Content (Join-Path $target 'WheelFfb.dll') -Raw) -ceq 'Old native fixture') 'Rollback native differs'
# Prior receipt bytes, including unknown metadata, travel with the runtime.
$oldReceiptBytes = '{"label":"legacy-owner", "unknown":"retain exact bytes"}'
[IO.File]::WriteAllText((Join-Path $target 'install-receipt.json'), $oldReceiptBytes)
$log = Invoke-Installer -BuildDir $build -Target $target -Label fixture-receipt 2>&1
Check ($installerExitCode -eq 0) 'Install over legacy receipt failed'
$receipt = Get-Content (Join-Path $target 'install-receipt.json') -Raw | ConvertFrom-Json
$backupExe = Join-Path $target ('deployment-backups/' + $receipt.backup + '/FZeroSNESRecomp-wheel-launcher.exe')
$originalBackup = [IO.File]::ReadAllBytes($backupExe)
[IO.File]::WriteAllText($backupExe, 'tampered')
$beforeRefusal = Snapshot
$log = Invoke-Installer -Target $target -Rollback $receipt.backup 2>&1
Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'Backup hash differs') 'Tampered backup accepted'
Check ((Snapshot) -ceq $beforeRefusal) 'Tampered backup refusal changed files'
[IO.File]::WriteAllBytes($backupExe, $originalBackup)
$log = Invoke-Installer -Target $target -Rollback $receipt.backup 2>&1
Check ($installerExitCode -eq 0) 'Legacy receipt rollback failed'
Check ((Get-Content (Join-Path $target 'install-receipt.json') -Raw) -ceq $oldReceiptBytes) 'Receipt not restored byte-exact'
Check ((Get-FileHash (Join-Path $target 'config.ini')).Hash -ceq $configHash -and (Get-FileHash (Join-Path $target 'fzero-video.ini')).Hash -ceq $videoHash) 'Rollback touched settings'
# Matching payloads with no capability must refresh just the receipt (and back it up).
Copy-Item -LiteralPath $exe -Destination (Join-Path $target 'FZeroSNESRecomp-wheel-launcher.exe') -Force
Copy-Item -LiteralPath (Join-Path $build 'WheelFfb.dll') -Destination (Join-Path $target 'WheelFfb.dll') -Force
$log = Invoke-Installer -BuildDir $build -Target $target -Label fixture-refresh 2>&1
Check ($installerExitCode -eq 0) 'Matching payload receipt refresh failed'
$receipt = Get-Content (Join-Path $target 'install-receipt.json') -Raw | ConvertFrom-Json
Check ($receipt.controlsProfileAdapter -ceq 'fzero-raw-wheel-1') 'Receipt refresh did not advertise capability'
$log = Invoke-Installer -Target $target -Rollback $receipt.backup 2>&1
Check ($installerExitCode -eq 0 -and (Get-Content (Join-Path $target 'install-receipt.json') -Raw) -ceq $oldReceiptBytes) 'Receipt-only rollback failed'
# Native receipt actions must not withdraw separate Shared ownership or host gates.
$tokens=$null; $errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($installer,[ref]$tokens,[ref]$errors)
Check (!$errors.Count) 'Installer source did not parse'
foreach ($function in $ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst]},$false)) {
    if ($function.Name -in @('Shared-ControlsDescriptor','Assert-SharedOwnershipTransition')) { Invoke-Expression $function.Extent.Text }
}
$sharedControls = Shared-ControlsDescriptor
Check ($sharedControls.adapter -ceq 'fzero-shim-ini-controls-1' -and $sharedControls.optIn.key -ceq 'Backend' -and $sharedControls.optIn.value -ceq 'Shared') 'Shared controls partition changed'
foreach ($kind in 'Shared','HostControls') {
    $previous = @{controlsProfileActions=$actions; privateSharedInput=@{abi=2;extras=@{declarationSha256='761789a517b35b88395d6b3ba06e2d2ba60628168abda6637e69d581a7a51cf7'}};files=@()}
    $refused = $false
    try { Assert-SharedOwnershipTransition ($kind -eq 'HostControls') $previous } catch { $refused = $true }
    Check $refused "Native receipt actions bypassed the $kind ownership gate"
}
# Private package preparation must not need a dummy preinstalled launcher.
$scratch = Join-Path $fixture 'empty-package-target'
New-Item -ItemType Directory -Path $scratch | Out-Null
$log = Invoke-Installer -BuildDir $build -Target $scratch -Label scratch-default 2>&1
Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'Not the F-Zero Stream Deck folder') 'Normal install admitted an empty target'
Check (@(Get-ChildItem -LiteralPath $scratch -Force).Count -eq 0) 'Default refusal changed empty target'
$log = Invoke-Installer -BuildDir $build -Label scratch-implicit -PrivatePackageScratch 2>&1
Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'explicit existing empty') 'Scratch used implicit owner target'
$log = Invoke-Installer -BuildDir $build -Target (Join-Path $fixture 'missing-target') -Label scratch-missing -PrivatePackageScratch 2>&1
Check ($installerExitCode -ne 0 -and !(Test-Path -LiteralPath (Join-Path $fixture 'missing-target'))) 'Scratch implicitly created its target'
$log = Invoke-Installer -Target $scratch -Rollback wrong -PrivatePackageScratch 2>&1
Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'cannot be used with -Rollback') 'Scratch bypassed rollback admission'
$sentinel = Join-Path $scratch '.owner-data'
[IO.File]::WriteAllText($sentinel, 'preserve hidden and unknown files')
$sentinelHash = (Get-FileHash -LiteralPath $sentinel).Hash
$log = Invoke-Installer -BuildDir $build -Target $scratch -Label scratch-occupied -PrivatePackageScratch 2>&1
Check ($installerExitCode -ne 0 -and ($log -join "`n") -match 'empty target') 'Scratch accepted existing files'
Check ((Get-FileHash -LiteralPath $sentinel).Hash -ceq $sentinelHash -and @(Get-ChildItem -LiteralPath $scratch -Force).Count -eq 1) 'Scratch refusal altered existing files'
Remove-Item -LiteralPath $sentinel
$log = Invoke-Installer -BuildDir $build -Target $scratch -Label scratch-valid -PrivatePackageScratch 2>&1
Check ($installerExitCode -eq 0) ('Fresh package scratch refused: ' + ($log -join "`n"))
$scratchReceipt = Get-Content -LiteralPath (Join-Path $scratch 'install-receipt.json') -Raw | ConvertFrom-Json
Check ($scratchReceipt.nativeControls.schema -eq 2 -and $scratchReceipt.sourceCommit -ceq (git -C $repo rev-parse HEAD).Trim()) 'Scratch capability/source differs'
$scratchState = Get-Content -LiteralPath (Join-Path $scratch ('deployment-backups/' + $scratchReceipt.backup + '/backup-state.json')) -Raw | ConvertFrom-Json
Check (@($scratchState.files | Where-Object { $_.existed -or $null -ne $_.sha256 }).Count -eq 0) 'Scratch backup claims preexisting owner files'
Check ($scratchState.files.Count -eq $scratchReceipt.files.Count + 1) 'Scratch rollback inventory differs'
$log = Invoke-Installer -Target $scratch -Rollback $scratchReceipt.backup 2>&1
Check ($installerExitCode -eq 0 -and !(Test-Path -LiteralPath (Join-Path $scratch 'FZeroSNESRecomp-wheel-launcher.exe')) -and !(Test-Path -LiteralPath (Join-Path $scratch 'install-receipt.json'))) 'Scratch rollback did not restore absent runtime/receipt'
[ordered]@{ status='passed'; checks=$checks; fixture=$fixture; devicesOpened=$false } | ConvertTo-Json
