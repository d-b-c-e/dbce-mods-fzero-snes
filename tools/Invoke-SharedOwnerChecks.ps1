#Requires -Version 7.0
param([Parameter(Mandatory)][string]$OutputRoot,
      [string]$BuildRoot='E:/fzero-research/owner-checks-nemo-20261016/run-01/game-build',
      [switch]$HostControlsOnly,
      [string]$WheelkitRoot,
      [string]$HostControlsResumeFrom,
      [string]$HostControlsPackageResult)
$ErrorActionPreference='Stop'
$sourceRoot=[IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))
if($HostControlsOnly -and !(Test-Path -LiteralPath (Join-Path $WheelkitRoot 'tests/Wheelkit.Core.Tests/Wheelkit.Core.Tests.csproj'))){throw 'Host controls checks require the frozen Wheelkit worktree'}
if($HostControlsResumeFrom) {
  if(!$HostControlsOnly){throw 'Resume is scoped to the host controls checks'}
  $prior=Get-Content -LiteralPath $HostControlsResumeFrom -Raw|ConvertFrom-Json
  if($prior.scope -ne 'host-controls' -or !$prior.leaseReleased -or !$prior.childrenQuiescent -or !$prior.processQuerySafe){throw 'Prior host controls result must have safely released'}
  foreach($name in @('game-configure','game-build','game-tests')) {
    $steps=@($prior.steps|Where-Object name -eq $name)
    if($steps.Count -ne 1 -or $steps[0].state -ne 'pass'){throw ('Prior game step is not qualified: '+$name)}
  }
  & git -C $sourceRoot diff --quiet $prior.sourceHead HEAD -- CMakeLists.txt src tests/test_shim_map.c tests/test_shim_ordinary.cpp tests/test_shim_host.cpp tests/fake_shim.cpp
  if($LASTEXITCODE){throw 'Game inputs changed since the retained qualification'}
}
if($HostControlsPackageResult) {
  if(!$HostControlsResumeFrom){throw 'Retained package checks require the original game qualification'}
  $packagePrior=Get-Content -LiteralPath $HostControlsPackageResult -Raw|ConvertFrom-Json
  if($packagePrior.scope -ne 'host-controls' -or !$packagePrior.leaseReleased -or !$packagePrior.childrenQuiescent -or !$packagePrior.processQuerySafe){throw 'Prior package result must have safely released'}
  foreach($name in @('package-tests','host-package-test')) {
    $steps=@($packagePrior.steps|Where-Object name -eq $name)
    if($steps.Count -ne 1 -or $steps[0].state -ne 'pass'){throw ('Prior package step is not qualified: '+$name)}
  }
  & git -C $sourceRoot diff --quiet $packagePrior.sourceHead HEAD -- tools/stage_unified.py tools/package_unified.py tools/setup_bundle.py tests/test_unified_package.py
  if($LASTEXITCODE){throw 'Package inputs changed since the retained qualification'}
}
$outputRoot=[IO.Path]::GetFullPath($OutputRoot)
if(Test-Path -LiteralPath $outputRoot){throw 'Fresh result directory required'}
[IO.Directory]::CreateDirectory($outputRoot)|Out-Null
$observeExternal=$null
$oldPythonPath=[Environment]::GetEnvironmentVariable('PYTHONPATH','Process')
$oldPythonPathPresent=Test-Path Env:PYTHONPATH
$script:result=[ordered]@{schema='nemo.shared-owner-affected-checks.v1';startedUtc=[DateTime]::UtcNow.ToString('O');state='running';steps=@();leaseReleased=$false;sourceHead=(& git -C $sourceRoot rev-parse HEAD);scope=$(if($HostControlsOnly){'host-controls'}else{'shared-owner'})}
if($HostControlsResumeFrom){$script:result.retainedGameResult=[ordered]@{path=[IO.Path]::GetFullPath($HostControlsResumeFrom);sha256=(Get-FileHash -LiteralPath $HostControlsResumeFrom -Algorithm SHA256).Hash;sourceHead=$prior.sourceHead}}
if($HostControlsPackageResult){$script:result.retainedPackageResult=[ordered]@{path=[IO.Path]::GetFullPath($HostControlsPackageResult);sha256=(Get-FileHash -LiteralPath $HostControlsPackageResult -Algorithm SHA256).Hash;sourceHead=$packagePrior.sourceHead}}
. E:/Source/toolkits/dbce-wheel-mod-toolkit/tools/playback/Stage-RigLease.ps1
. E:/Source/toolkits/dbce-wheel-mod-toolkit/tools/playback/Stage-OwnedProcess.ps1
$lease=$null
try {
  $lease=Enter-StageRigLease -Path (Join-Path $env:LOCALAPPDATA 'dbce/test-slot.txt') -Owner nemo -Purpose ('FZERO-'+$script:result.scope.ToUpperInvariant()+'-DEVICE-FREE-CHECKS')
  $script:result.lease=$lease;Save-Result
  if(!$HostControlsResumeFrom) {
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
  }
  if($HostControlsOnly) {
  $script:result.wheelkitHead=(& git -C $WheelkitRoot rev-parse HEAD)
  $env:PYTHONPATH=(Join-Path $sourceRoot 'tools')+$(if($oldPythonPath){[IO.Path]::PathSeparator+$oldPythonPath}else{''})
  if(!$HostControlsPackageResult) {
  $script:result.steps+=,[ordered]@{name='package-tests';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'package-tests' -Executable (Get-Command python).Source -Arguments @('-m','unittest','discover','-s',(Join-Path $sourceRoot 'tests'),'-p','test_unified_package.py','-k','extras') -TimeoutSeconds 60
  $script:result.steps+=,[ordered]@{name='host-package-test';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'host-package-test' -Executable (Get-Command python).Source -Arguments @('-m','unittest','discover','-s',(Join-Path $sourceRoot 'tests'),'-p','test_unified_package.py','-k','host_controls') -TimeoutSeconds 60
  }
  $script:result.steps+=,[ordered]@{name='installer-tests';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'installer-tests' -Executable (Get-Command pwsh).Source -Arguments @('-NoProfile','-File',(Join-Path $sourceRoot 'tests/test_private_shared_install.ps1')) -TimeoutSeconds 60
  $script:result.steps+=,[ordered]@{name='wheelkit-tests';state='running'};Save-Result
  Invoke-OwnedProcess -Name 'wheelkit-tests' -Executable (Get-Command dotnet).Source -Arguments @('test',(Join-Path $WheelkitRoot 'tests/Wheelkit.Core.Tests/Wheelkit.Core.Tests.csproj'),'-c','Release','--filter','FullyQualifiedName~FZeroShimControlTests','--logger',('trx;LogFileName='+ (Join-Path $outputRoot 'wheelkit.trx'))) -TimeoutSeconds 180
  }
  $script:result.state='pass'
} catch {$script:result.state='failed';$script:result.error=$_.Exception.Message}
finally {
  if($oldPythonPathPresent){$env:PYTHONPATH=$oldPythonPath}else{Remove-Item Env:PYTHONPATH -ErrorAction SilentlyContinue}
  $script:result.pythonPathRestored=((Test-Path Env:PYTHONPATH) -eq $oldPythonPathPresent -and [string][Environment]::GetEnvironmentVariable('PYTHONPATH','Process') -ceq [string]$oldPythonPath)
  Complete-Owned
  if($lease -and $script:result.childrenQuiescent -and $script:result.processQuerySafe){$script:result.leaseReleased=Exit-StageRigLease -Lease $lease}
  $script:result.endedUtc=[DateTime]::UtcNow.ToString('O');Save-Result
}
$script:result|ConvertTo-Json -Depth 12
if($script:result.state -ne 'pass' -or !$script:result.leaseReleased){exit 1}
