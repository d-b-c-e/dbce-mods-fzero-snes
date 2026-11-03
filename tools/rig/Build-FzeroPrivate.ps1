#Requires -Version 7.0
param([Parameter(Mandatory)][string]$Evidence,[switch]$Execute,[string]$Go)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
. "$PSScriptRoot/Readback-Contract.ps1"
function Sha($p){Get-ReadbackSha $p}
function WriteJson($p,$v){$v|ConvertTo-Json -Depth 15|Set-Content -LiteralPath $p}
function ReadJson($p){Get-Content -LiteralPath $p -Raw|ConvertFrom-Json -DateKind String}
function Head($p){$h=(& git -C $p rev-parse HEAD).Trim();if($LASTEXITCODE -or @(& git -C $p status --porcelain --untracked-files=no).Count){throw "Source not clean: $p"};$h}
function Quote($s){if($s -match '["\r\n]' -or $s.EndsWith('\')){throw 'Unsupported argument'};'"'+$s+'"'}
$Evidence=[IO.Path]::GetFullPath($Evidence);Assert-ReadbackPlain $Evidence
$canonical='E:/Source/game-modding/dbce-mods-fzero-snes'
$helper='E:/Source/_work/napo-fzero-helper-d0f333-20261103'
$archive="$canonical/build-vemi-clean/_deps/sdl3-subbuild/sdl3-populate-prefix/src/SDL3-3.4.10.tar.gz"
$shim='E:/Source/tools/launchbox/Launchbox-Racing/Games/Windows/F-Zero (SNES Recomp)/dbce_shim.dll'
$sdk='E:/Source/game-modding/_toolkits/dbce-wheel-mod-toolkit'
$vcvars='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat'
if(-not $Execute){
 if(Test-Path $Evidence){throw 'Fresh evidence directory required'}
 $heads=@{};$heads[$root]=Head $root;$heads["$canonical/snesrecomp"]=Head "$canonical/snesrecomp";$heads["$canonical/recomp-ui"]=Head "$canonical/recomp-ui";$heads[$helper]=Head $helper
 if($heads[$helper] -cne 'd0f333c60892dc44beb36e277b19bd6a650507b7' -or $heads["$canonical/snesrecomp"] -cne '59d2966fa71d0346f92c43753e09b1ba7a1956f5' -or $heads["$canonical/recomp-ui"] -cne '6d9b71526f82c0182f85ec47610eb559303a77b8'){throw 'Dependency/helper pin differs'}
 if((Sha $archive) -cne '12B34280415EC8418C864408B93D008A20A6530687EE613D60BFBD20411F2785' -or (Sha $shim) -cne 'BF6455E63696A93B37C19DABF09B1E0606F26B0534BCE828417B1DF602DD8A7A' -or (Sha "$root/lib/toolkit/native/WheelFfb.dll") -cne '3F6CC51444879D01FEB900098B2DE94242594D093F523E3C34BD511D69C0D946'){throw 'Runtime/archive pin differs'}
 $paths=@($archive,$shim,$vcvars,"$root/lib/toolkit/native/WheelFfb.dll","$root/lib/toolkit/source-to-binary-manifest.json","$root/vendor/dbce-shim/PIN.json","$canonical/captures/bs-deluxe/mods/bs-deluxe.dat","$canonical/captures/bs-deluxe/gen/deluxe_namespace.h","$PSScriptRoot/FzeroLive.cs",$PSCommandPath)
 $paths+=@(Get-ChildItem "$canonical/src/gen" -File|ForEach-Object FullName)
 $paths+=@(Get-ChildItem "$canonical/captures/bs-deluxe/gen" -File -Filter '*.c'|ForEach-Object FullName)
 $pins=@($paths|Select-Object -Unique|ForEach-Object{Assert-ReadbackPlain $_;@{path=$_;sha256=Sha $_}})
 [void][IO.Directory]::CreateDirectory($Evidence)
 WriteJson "$Evidence/inputs.json" @{schema='fzero-private-build@1';heads=$heads;pins=$pins;maxSeconds=1200;source=$heads[$root];helper=$heads[$helper];archive=$archive;root=$root;helperRoot=$helper;canonical=$canonical;shim=$shim;vcvars=$vcvars}
 Write-Output 'Prepared clean source/dependency/generated-input pins; no build/lease/game.';return
}
if(-not $Go){throw 'CPU GO required'}
$plan=ReadJson "$Evidence/inputs.json"
foreach($h in $plan.heads.PSObject.Properties){if((Head $h.Name) -cne $h.Value){throw 'Source HEAD drift'}}
foreach($p in $plan.pins){if((Sha $p.path) -cne $p.sha256){throw "Input drift: $($p.path)"}}
$run=Join-Path $Evidence run;if(Test-Path $run){throw 'Fresh run required'};[void][IO.Directory]::CreateDirectory($run)
Add-Type -Path "$PSScriptRoot/FzeroLive.cs"
$lease=$false;$result=@{pass=$false;source=$plan.source;helper=$plan.helper;startedUtc=[DateTime]::UtcNow.ToString('o');steps=@();leaseReleased=$false;gameStarted=$false;ownerInstalled=$false;publicPush=$false}
$deadline=[DateTime]::UtcNow.AddSeconds($plan.maxSeconds)
function Step([string]$name,[string]$exe,[string[]]$argv,[int]$seconds=300,[switch]$Msvc){
 if(([DateTime]::UtcNow.AddSeconds(60)) -gt $deadline){throw 'Build batch deadline; reserve cleanup'}
 $command=(@($exe)+$argv|ForEach-Object{Quote $_}) -join ' '
 if($Msvc){$cmd=Join-Path $run "$name.cmd";[IO.File]::WriteAllLines($cmd,@('@echo off','set VSCMD_SKIP_SENDTELEMETRY=1',('call '+(Quote $vcvars)+' >nul'),'if errorlevel 1 exit /b 1',$command,'exit /b %errorlevel%'));$exe=$env:ComSpec;$command='cmd.exe /d /c '+(Quote $cmd)}
 $id=[FzeroLive]::Launch($exe,$root,$command,"$run/$name.stdout.txt","$run/$name.stderr.txt",$false);$p=Get-Process -Id $id.pid;$null=$p.Handle
 $until=[DateTime]::UtcNow.AddSeconds($seconds);if($until -gt $deadline.AddSeconds(-60)){$until=$deadline.AddSeconds(-60)}
 while(-not $p.HasExited){if([DateTime]::UtcNow -gt $until){throw "Step timeout: $name"};Start-Sleep -Milliseconds 200}
 $p.WaitForExit();$result.steps+=@{name=$name;process=$id;exitCode=$p.ExitCode;utc=[DateTime]::UtcNow.ToString('o')};WriteJson "$run/result.json" $result
 if($p.ExitCode -ne 0){throw "Step failed: $name ($($p.ExitCode))"};$p.Dispose()
}
try{
 & E:/Source/dbce-project-mgmt/tools/Rig-Lease.ps1 -Enter -Name napo-fzero-build -Purpose "F-Zero private current build CPU GO$Go; no game/UI/hardware" -IdleMinutes 0|Set-Content "$run/lease.txt";$lease=$true
 $env:DOTNET_CLI_DO_NOT_USE_MSBUILD_SERVER='1';$env:DOTNET_GENERATE_ASPNET_CERTIFICATE='false'
 $python=(Get-Command python).Source;$cmake=(Get-Command cmake).Source;$pwsh=(Get-Command pwsh).Source;$ctest=(Get-Command ctest).Source
 $sourceDir=Join-Path $run sdl-source;[void][IO.Directory]::CreateDirectory($sourceDir)
 Step extract $python @('-c',"import sys,tarfile; t=tarfile.open(sys.argv[1]); t.extractall(sys.argv[2],filter='data')",$archive,$sourceDir) 60
 $sdl=Join-Path $sourceDir 'SDL3-3.4.10'
 Step helper-publish $pwsh @('-NoProfile','-File',"$helper/tools/Publish-StandaloneSetup.ps1",'-Artifacts',"$run/helper-publish") 300
 $bundle=Join-Path $run 'helper-publish/standalone-build-d0f333c60892/stage'
 $build=Join-Path $run build
 $config=@('-S',$root,'-B',$build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_TRY_COMPILE_CONFIGURATION=Release','-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded','-DCMAKE_C_FLAGS_RELEASE=/O2 /Ob2 /DNDEBUG /Z7','-DCMAKE_CXX_FLAGS_RELEASE=/O2 /Ob2 /DNDEBUG /Z7',"-DSNESRECOMP_ROOT=$canonical/snesrecomp","-DRECOMP_UI_ROOT=$canonical/recomp-ui","-DFZERO_GEN_DIR=$canonical/src/gen","-DFZERO_DELUXE_GEN_DIR=$canonical/captures/bs-deluxe/gen","-DFZERO_DELUXE_DATA_FILE=$canonical/captures/bs-deluxe/mods/bs-deluxe.dat","-DFZERO_SHIM_DLL=$shim","-DFZERO_SETUP_BUNDLE=$bundle","-DSNESRECOMP_SDL3_SOURCE_DIR=$sdl",'-DBUILD_TESTING=ON')
 Step configure $cmake $config 180 -Msvc
 $probes=Get-Content "$build/CMakeFiles/CMakeConfigureLog.yaml" -Raw;if($probes -match 'cl\.exe"[^\r\n]*\s/Zi\s'){throw 'Debug compiler probe created PDB custody risk'}
 $targets=@('FZeroSNESRecomp','FZeroSNESRecompHeadless','FZeroRenderCapture','fzero_controls_contract_tests','fzero_gamepad_tests','fzero_controls_tests','fzero_telemetry_tests','fzero_panel_tests','fzero_hotkeys_tests','fzero_renderer_tests')
 Step product-build $cmake (@('--build',$build,'--parallel','2','--target')+$targets) 600 -Msvc
 Step focused-tests $ctest @('--test-dir',$build,'--output-on-failure','-R','^fzero_(controls_contract|gamepad|controls|telemetry|panel|hotkeys|renderer)$') 120
 $scratch=Join-Path $run scratch;[void][IO.Directory]::CreateDirectory($scratch)
 Step scratch-install $pwsh @('-NoProfile','-File',"$root/tools/Install-StreamDeck.ps1",'-BuildDir',$build,'-Target',$scratch,'-Label','napo-current-20261103','-PrivateSharedInput','-PrivateControlsSetup','-PrivateHostControls','-PrivatePackageScratch') 120
 Step private-stage $python @("$root/tools/stage_unified.py",'--build',$build,'--output',"$run/stage",'--private-shared-input','--private-controls-setup','--private-host-controls','--private-deluxe-install',$scratch) 120
 Step private-package $python @("$root/tools/package_unified.py",'--payload',"$run/stage/payload",'--receipt',"$run/stage/build-receipt.json",'--output',"$run/delivery",'--private-deluxe') 120
 $zip=Get-ChildItem "$run/delivery" -Filter '*.zip'|Select-Object -First 1;if(-not $zip){throw 'Private archive missing'}
 Step verify-package $python @("$root/tools/package_unified.py",'--verify',$zip.FullName,'--private-deluxe') 120
 Step helper-plan $pwsh @('-NoProfile','-File',"$run/stage/payload/controls-setup/Start-ModSetup.ps1",'-Game','f-zero','-GameDir',"$run/stage/payload",'-PlanOnly') 60
 foreach($pin in $plan.pins){if((Sha $pin.path) -cne $pin.sha256){throw 'Input changed during build'}}
 foreach($h in $plan.heads.PSObject.Properties){if((Head $h.Name) -cne $h.Value){throw 'Source changed during build'}}
 $receipt=ReadJson "$run/stage/build-receipt.json";if($receipt.sourceRevision -cne $plan.source -or -not $receipt.bsDeluxe -or $receipt.privateControlsSetup.sourceCommit -cne $plan.helper){throw 'Final candidate receipt identity differs'}
 $result.receiptSha256=Sha "$run/stage/build-receipt.json";$result.gameSha256=Sha "$build/FZeroSNESRecomp.exe";$result.archive=$zip.FullName;$result.archiveSha256=Sha $zip.FullName;$result.pass=$true
}catch{$result.error=$_.Exception.Message}
finally{
 try{
  $until=[DateTime]::UtcNow.AddSeconds(10);while(@([FzeroLive]::JobPids()).Count -and [DateTime]::UtcNow -lt $until){Start-Sleep -Milliseconds 100}
  if(@([FzeroLive]::JobPids()).Count){$result.forcedOwnChildCleanup=$true;$result.pass=$false;[FzeroLive]::KillOwned();$until=[DateTime]::UtcNow.AddSeconds(5);while(@([FzeroLive]::JobPids()).Count -and [DateTime]::UtcNow -lt $until){Start-Sleep -Milliseconds 50}}
  [FzeroLive]::ReleaseJob();$result.jobEmpty=$true
  if($lease){$r=& E:/Source/dbce-project-mgmt/tools/Rig-Lease.ps1 -Exit -Name napo-fzero-build;$r|Set-Content "$run/release.txt";if($r -notcontains 'lease released'){throw 'Lease release not confirmed'};$result.leaseReleased=$true}
 }catch{$result.cleanupError=$_.Exception.Message;$result.pass=$false;$result.retainCustody=$lease}
 $result.finishedUtc=[DateTime]::UtcNow.ToString('o');WriteJson "$run/result.json" $result
}
$result|ConvertTo-Json -Depth 7
if(-not $result.pass){exit 1}
