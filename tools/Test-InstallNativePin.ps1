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
Check ($installerExitCode -eq 0) ('Exact native install failed: ' + ($log -join "`n"))
Check ((Get-FileHash (Join-Path $target 'WheelFfb.dll')).Hash -ceq (Get-FileHash (Join-Path $build 'WheelFfb.dll')).Hash) 'Native copy differs'
Check ((Get-FileHash (Join-Path $target 'config.ini')).Hash -ceq $configHash) 'Config changed'
Check ((Get-FileHash (Join-Path $target 'fzero-video.ini')).Hash -ceq $videoHash) 'Video changed'
$receipt = Get-Content (Join-Path $target 'install-receipt.json') -Raw | ConvertFrom-Json
Check ((Get-Content (Join-Path $target ('deployment-backups/' + $receipt.backup + '/WheelFfb.dll')) -Raw) -ceq 'Old native fixture') 'Native backup differs'
Check ($receipt.schemaVersion -eq 1 -and $receipt.controlsProfileSchema -eq 1 -and $receipt.controlsProfileAdapter -ceq 'fzero-raw-wheel-1') 'Capability absent'
Check ($receipt.launcherSha256 -ceq (Get-FileHash $exe).Hash -and $receipt.wheelFfbSha256 -ceq (Get-FileHash (Join-Path $build 'WheelFfb.dll')).Hash) 'Capability hashes differ'
Check ($receipt.sourceCommit -ceq (git -C $repo rev-parse HEAD).Trim()) 'Source commit differs'
$installedReceiptHash = (Get-FileHash (Join-Path $target 'install-receipt.json')).Hash
$log = Invoke-Installer -BuildDir $build -Target $target -Label fixture 2>&1
Check ($installerExitCode -eq 0 -and ($log -join "`n") -match 'already match') 'Exact re-install not a no-op'
Check ((Get-FileHash (Join-Path $target 'install-receipt.json')).Hash -ceq $installedReceiptHash) 'No-op changed receipt'
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
[ordered]@{ status='passed'; checks=$checks; fixture=$fixture; devicesOpened=$false } | ConvertTo-Json
