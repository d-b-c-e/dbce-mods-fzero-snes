#Requires -Version 7.0
param([Parameter(Mandatory)][string]$OutputRoot,
      [string]$BuildRoot='E:/fzero-research/owner-checks-nemo-20261016/run-01/game-build',
      [switch]$HostControlsOnly,
      [string]$WheelkitRoot)
$ErrorActionPreference='Stop'
$sourceRoot=[IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))
if($HostControlsOnly -and !(Test-Path -LiteralPath (Join-Path $WheelkitRoot 'tests/Wheelkit.Core.Tests/Wheelkit.Core.Tests.csproj'))){throw 'Host controls checks require the frozen Wheelkit worktree'}
$outputRoot=[IO.Path]::GetFullPath($OutputRoot)
if(Test-Path -LiteralPath $outputRoot){throw 'Fresh result directory required'}
[IO.Directory]::CreateDirectory($outputRoot)|Out-Null
$observeExternal=$null
$script:result=[ordered]@{schema='nemo.shared-owner-affected-checks.v1';startedUtc=[DateTime]::UtcNow.ToString('O');state='running';steps=@();leaseReleased=$false;sourceHead=(& git -C $sourceRoot rev-parse HEAD);scope=$(if($HostControlsOnly){'host-controls'}else{'shared-owner'})}
. E:/Source/toolkits/dbce-wheel-mod-toolkit/tools/playback/Stage-RigLease.ps1
. E:/Source/toolkits/dbce-wheel-mod-toolkit/tools/playback/Stage-OwnedProcess.ps1
$lease=$null
try {
  $lease=Enter-StageRigLease -Path (Join-Path $env:LOCALAPPDATA 'dbce/test-slot.txt') -Owner nemo -Purpose ('FZERO-'+$script:result.scope.ToUpperInvariant()+'-DEVICE-FREE-CHECKS')
  $script:result.lease=$lease;Save-Result
  if($HostControlsOnly) {
  $script:result.wheelkitHead=(& git -C $WheelkitRoot rev-parse HEAD)
  $script:result.steps+=,[ordered]@{name='game-configure';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'game-configure' -Executable (Get-Command cmake).Source -Arguments @('-S',$sourceRoot,'-B',$BuildRoot,'-DFZERO_BUILD_GAME=OFF','-DBUILD_TESTING=ON') -TimeoutSeconds 120
  }
  $script:result.steps+=,[ordered]@{name='game-build';state='running'};Save-Result
  $targets=if($HostControlsOnly){@('fzero_shim_map_tests','fzero_shim_host_tests','fzero_shim_ordinary_tests')}else{@('fzero_shared_owner_tests','fzero_shared_ffb_tests','fzero_shim_host_tests','fzero_shim_ordinary_tests')}
  Invoke-OwnedProcess -Name 'game-build' -Executable (Get-Command cmake).Source -Arguments (@('--build',$BuildRoot,'--config','Release','--parallel','2','--target')+$targets) -TimeoutSeconds 180
  Assert-StageRigLease -Path $lease.path -Token $lease.token
  $script:result.steps+=,[ordered]@{name='game-tests';state='running'};Save-Result
  $filter=if($HostControlsOnly){'^fzero_(shim_map|shim_host_hostcontrols|shim_ordinary_(hosts|hostsnoflag|hostsrecord|hostschanged|extras|extrasmissing|launchersaved))$'}else{'^fzero_(shared_|shim_host_|shim_ordinary_)'}
  Invoke-OwnedProcess -Name 'game-tests' -Executable (Get-Command ctest).Source -Arguments @('--test-dir',$BuildRoot,'-C','Release','--output-on-failure','-R',$filter) -TimeoutSeconds 120
  if($HostControlsOnly) {
  $script:result.steps+=,[ordered]@{name='package-tests';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'package-tests' -Executable (Get-Command python).Source -Arguments @('-m','unittest','discover','-s',(Join-Path $sourceRoot 'tests'),'-p','test_unified_package.py','-k','extras') -TimeoutSeconds 60
  $script:result.steps+=,[ordered]@{name='host-package-test';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'host-package-test' -Executable (Get-Command python).Source -Arguments @('-m','unittest','discover','-s',(Join-Path $sourceRoot 'tests'),'-p','test_unified_package.py','-k','host_controls') -TimeoutSeconds 60
  $script:result.steps+=,[ordered]@{name='installer-tests';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'installer-tests' -Executable (Get-Command pwsh).Source -Arguments @('-NoProfile','-File',(Join-Path $sourceRoot 'tests/test_private_shared_install.ps1')) -TimeoutSeconds 60
  $script:result.steps+=,[ordered]@{name='wheelkit-tests';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'wheelkit-tests' -Executable (Get-Command dotnet).Source -Arguments @('test',(Join-Path $WheelkitRoot 'tests/Wheelkit.Core.Tests/Wheelkit.Core.Tests.csproj'),'-c','Release','--filter','FullyQualifiedName~FZeroShimControlTests','--logger',('trx;LogFileName='+ (Join-Path $outputRoot 'wheelkit.trx'))) -TimeoutSeconds 180
  }
  $script:result.state='pass'
} catch {$script:result.state='failed';$script:result.error=$_.Exception.Message}
finally {
  Complete-Owned
  if($lease -and $script:result.childrenQuiescent -and $script:result.processQuerySafe){$script:result.leaseReleased=Exit-StageRigLease -Lease $lease}
  $script:result.endedUtc=[DateTime]::UtcNow.ToString('O');Save-Result
}
$script:result|ConvertTo-Json -Depth 12
if($script:result.state -ne 'pass' -or !$script:result.leaseReleased){exit 1}
