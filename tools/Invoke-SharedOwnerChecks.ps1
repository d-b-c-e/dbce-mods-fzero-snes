#Requires -Version 7.0
param([Parameter(Mandatory)][string]$OutputRoot,
      [string]$BuildRoot='E:/fzero-research/owner-checks-nemo-20261016/run-01/game-build')
$ErrorActionPreference='Stop'
$sourceRoot=[IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))
$outputRoot=[IO.Path]::GetFullPath($OutputRoot)
if(Test-Path -LiteralPath $outputRoot){throw 'Fresh result directory required'}
[IO.Directory]::CreateDirectory($outputRoot)|Out-Null
$observeExternal=$null
$script:result=[ordered]@{schema='nemo.shared-owner-affected-checks.v1';startedUtc=[DateTime]::UtcNow.ToString('O');state='running';steps=@();leaseReleased=$false;sourceHead=(& git -C $sourceRoot rev-parse HEAD)}
. E:/Source/toolkits/dbce-wheel-mod-toolkit/tools/playback/Stage-RigLease.ps1
. E:/Source/toolkits/dbce-wheel-mod-toolkit/tools/playback/Stage-OwnedProcess.ps1
$lease=$null
try {
  $lease=Enter-StageRigLease -Path (Join-Path $env:LOCALAPPDATA 'dbce/test-slot.txt') -Owner nemo -Purpose 'FZERO-SHARED-OWNER-GAME-ONLY-CORRECTED-FAKE-CHECKS'
  $script:result.lease=$lease;Save-Result
  $script:result.steps+=,[ordered]@{name='game-build';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'game-build' -Executable (Get-Command cmake).Source -Arguments @('--build',$BuildRoot,'--config','Release','--parallel','2','--target','fzero_shared_owner_tests','fzero_shared_ffb_tests','fzero_shim_host_tests','fzero_shim_ordinary_tests') -TimeoutSeconds 180
  Assert-StageRigLease -Path $lease.path -Token $lease.token
  $script:result.steps+=,[ordered]@{name='game-tests';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'game-tests' -Executable (Get-Command ctest).Source -Arguments @('--test-dir',$BuildRoot,'-C','Release','--output-on-failure','-R','^fzero_(shared_|shim_host_|shim_ordinary_)') -TimeoutSeconds 120
  $script:result.state='pass'
} catch {$script:result.state='failed';$script:result.error=$_.Exception.Message}
finally {
  Complete-Owned
  if($lease -and $script:result.childrenQuiescent -and $script:result.processQuerySafe){$script:result.leaseReleased=Exit-StageRigLease -Lease $lease}
  $script:result.endedUtc=[DateTime]::UtcNow.ToString('O');Save-Result
}
$script:result|ConvertTo-Json -Depth 12
if($script:result.state -ne 'pass' -or !$script:result.leaseReleased){exit 1}
