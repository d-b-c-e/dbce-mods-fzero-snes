# Dummy files only; never launches a game or loads WheelFfb.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$fixture = Join-Path $repo ('build/install-native-pin-' + [guid]::NewGuid().ToString('N'))
$build = Join-Path $fixture 'build'
$target = Join-Path $fixture 'target'
New-Item -ItemType Directory -Path $build,$target | Out-Null
$installer = Join-Path $PSScriptRoot 'Install-StreamDeck.ps1'
$shell = (Get-Process -Id $PID).Path
function Check([bool]$condition,[string]$message) { if (!$condition) { throw $message }; $script:checks++ }
function Snapshot { (@(Get-ChildItem -LiteralPath $target -Recurse -File | Sort-Object FullName | ForEach-Object { $_.FullName + ':' + (Get-FileHash $_.FullName).Hash }) -join "`n") }
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
    $log = & $shell -NoProfile -File $installer -BuildDir $build -Target $target -Label fixture 2>&1
    Check ($LASTEXITCODE -ne 0) "$kind runtime accepted"
    Check (($log -join "`n") -match 'native pin/build runtime|differs from the source pin') "$kind refused for wrong reason"
    Check ((Snapshot) -ceq $before) "$kind refusal changed target"
}
Copy-Item -LiteralPath (Join-Path $repo 'lib/toolkit/native/WheelFfb.dll') -Destination (Join-Path $build 'WheelFfb.dll')
$log = & $shell -NoProfile -File $installer -BuildDir $build -Target $target -Label fixture 2>&1
Check ($LASTEXITCODE -eq 0) ('Exact native install failed: ' + ($log -join "`n"))
Check ((Get-FileHash (Join-Path $target 'WheelFfb.dll')).Hash -ceq (Get-FileHash (Join-Path $build 'WheelFfb.dll')).Hash) 'Native copy differs'
Check ((Get-FileHash (Join-Path $target 'config.ini')).Hash -ceq $configHash) 'Config changed'
Check ((Get-FileHash (Join-Path $target 'fzero-video.ini')).Hash -ceq $videoHash) 'Video changed'
$receipt = Get-Content (Join-Path $target 'install-receipt.json') -Raw | ConvertFrom-Json
Check ((Get-Content (Join-Path $target ('deployment-backups/' + $receipt.backup + '/WheelFfb.dll')) -Raw) -ceq 'Old native fixture') 'Native backup differs'
[ordered]@{ status='passed'; checks=$checks; fixture=$fixture; devicesOpened=$false } | ConvertTo-Json
