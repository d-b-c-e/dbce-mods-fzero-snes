#Requires -Version 7.0
param([Parameter(Mandatory)][ValidateSet('Prepare','Rehearse','Live')][string]$Mode,
 [Parameter(Mandatory)][string]$Evidence,[string]$PackageRoot,[string]$BuildReceipt,[string]$ExpectedSource,
 [string]$Rom,[string]$SaveRoot,[string]$CaptureTool,[string]$OwnerRoot,[string]$Go,
 [string]$BuildResult,[string]$Hub='E:/Source/dbce-project-mgmt')
$ErrorActionPreference='Stop'
. "$PSScriptRoot/Readback-Contract.ps1"
. "$PSScriptRoot/Candidate-Admission.ps1"
. "$PSScriptRoot/Ordinary-Contract.ps1"
function Json($p){Read-FzeroSharedText $p|ConvertFrom-Json -DateKind String}
function WriteJson($p,$v){$v|ConvertTo-Json -Depth 16|Set-Content -LiteralPath $p}
function Quote([string]$s){if($s -match '["\r\n]' -or $s.EndsWith('\')){throw 'Unsupported command argument'};'"'+$s+'"'}
function Rows($dir){@(Get-ChildItem -LiteralPath $dir -Recurse -File -Force|ForEach-Object{Assert-ReadbackPlain $_.FullName;@{path=$_.FullName;sha256=Get-ReadbackSha $_.FullName}})}
function CheckRows($rows){foreach($r in $rows){if((Get-ReadbackSha $r.path) -ine $r.sha256){throw "Pinned input drift: $($r.path)"}}}
$Evidence=[IO.Path]::GetFullPath($Evidence);Assert-ReadbackPlain $Evidence
if($Mode -eq 'Prepare'){
 if(Test-Path $Evidence){throw 'Fresh evidence root required'}
 foreach($p in @($PackageRoot,$BuildReceipt,$Rom,$SaveRoot,$CaptureTool,$BuildResult)){if(-not $p -or -not [IO.Path]::IsPathFullyQualified($p)){throw 'Absolute inputs required'};Assert-ReadbackPlain $p}
 if((Test-Path $SaveRoot) -or $SaveRoot.Length -gt 70 -or -not(Test-Path (Split-Path $SaveRoot) -PathType Container)){throw 'Fresh short root with existing plain parent required'}
 if((Get-ReadbackSha $Rom) -cne 'BF16C3C867C58E2AB061C70DE9295B6930D63F29F81CC986F5ECAE03E0AD18D2'){throw 'Stock ROM pin differs'}
 $candidate=Get-FzeroCandidate $PackageRoot $BuildReceipt $ExpectedSource
 $built=Json $BuildResult
 foreach($name in @('product-build','focused-tests','scratch-install','private-stage','private-package','verify-package','helper-plan')){if(@($built.steps|Where-Object {$_.name -ceq $name -and $_.exitCode -eq 0}).Count -ne 1){throw "Missing successful build step: $name"}}
 if(-not $built.jobEmpty -or -not $built.leaseReleased -or $built.source -cne $ExpectedSource){throw 'Build release/source mismatch'}
 $inputPlan=Json (Join-Path (Split-Path (Split-Path $BuildResult)) 'inputs.json');if($inputPlan.ordinaryDiagnostic -ne $true){throw 'Ordinary compile seam not receipted'}
 [void][IO.Directory]::CreateDirectory($Evidence);$app=Join-Path $Evidence app;[void][IO.Directory]::CreateDirectory($app)
 Get-ChildItem -LiteralPath $PackageRoot -Force|Copy-Item -Destination $app -Recurse -Force
 Copy-Item -LiteralPath $Rom -Destination "$app/fzero.sfc"
 [IO.File]::WriteAllText("$app/config.ini","; ordinary fixture preserved marker`n[General]`nSkipLauncher=1`n[Input]`nBackend=Native`n[Graphics]`nFullscreen=0`nWindowScale=3`n[Sound]`nVolume=0`nMsu1Enabled=0`n[ForceFeedback]`nEnabled=0`n[Telemetry]`nEnabled=0`n[Controller.00000000d1a600000000000000000001]`nSteeringAxis=7`nAcceleratorAxis=6`nBrakeAxis=5`nDeadzone=0`nAnalogSteering=1`nSteeringRangePercent=100`nSteeringResponsePercent=100`n[FixtureUnknown]`nKeep=unchanged`n")
 [IO.File]::WriteAllText("$app/fzero-video.ini","[FZeroVideo]`nBSDeluxe=0`nAspect=4:3`nEnhancedRenderer=1`nShowFPS=0`n")
 WriteJson "$Evidence/profile.json" (New-FzeroOrdinaryProfile)
 $sources=@(Rows $PackageRoot)+@(Rows $PSScriptRoot)+@(Rows (Split-Path $CaptureTool))
 foreach($p in @($BuildReceipt,$BuildResult,$Rom,"$PSScriptRoot/../../tests/Verify-ReadbackFiles.ps1","$PSScriptRoot/../../tests/Test-OrdinaryContract.ps1")){$sources+=@{path=[IO.Path]::GetFullPath($p);sha256=Get-ReadbackSha $p}}
 WriteJson "$Evidence/prepared.json" @{schema='fzero-ordinary-prepared@1';source=$ExpectedSource;candidate=$candidate;app=$app;appRows=Rows $app;sources=$sources;saveRoot=$SaveRoot;capture=$CaptureTool;profileSha256=Get-ReadbackSha "$Evidence/profile.json"}
 'Prepared admitted current private ordinary candidate; no executable launched.';return
}
$plan=Json "$Evidence/prepared.json";if($plan.schema -cne 'fzero-ordinary-prepared@1'){throw 'Wrong ordinary plan'}
CheckRows $plan.sources
if((Get-ReadbackSha "$Evidence/profile.json") -cne $plan.profileSha256){throw 'Profile drift'}
if(Get-Process|Where-Object ProcessName -Like 'FZeroSNESRecomp*'){throw 'Existing F-Zero process refused'}
Add-Type -Path "$PSScriptRoot/FzeroLive.cs"
$run=Join-Path $Evidence $Mode.ToLowerInvariant();if(Test-Path $run){throw 'One invocation per mode/evidence'};[void][IO.Directory]::CreateDirectory($run)
$lease=$false;$hooks=$false;$snapshot=$null;$owned=$null;$sink=$null;$capture=$null;$observations=[Collections.Generic.List[object]]::new();$packets=[Collections.Generic.List[object]]::new();$known=[Collections.Generic.List[object]]::new()
$result=@{pass=$false;mode=$Mode;source=$plan.source;gameStarted=$false;leaseReleased=$false;physicalWheel=$false;globalInput=$false;virtualController=$false}
$envBefore=@{};foreach($e in Get-ChildItem Env:){if($e.Name -match '^(FZERO_|SNESRECOMP_|SDL_)'){$envBefore[$e.Name]=$e.Value;[Environment]::SetEnvironmentVariable($e.Name,$null,'Process')}}
function Launch($exe,$argv,$name,[bool]$game=$false){
 $command=(@($exe)+$argv|ForEach-Object{Quote $_}) -join ' '
 $identity=[FzeroLive]::Launch($exe,$plan.app,$command,"$run/$name.stdout.txt","$run/$name.stderr.txt",$game);$known.Add($identity);return $identity
}
function ReceivePackets{
 while($sink.Client.Poll(0,[Net.Sockets.SelectMode]::SelectRead)){
  $endpoint=[Net.IPEndPoint]::new([Net.IPAddress]::Any,0);$bytes=$sink.Receive([ref]$endpoint)
  if($bytes.Length -ne 324 -or $endpoint.Address.ToString() -cne '127.0.0.1'){throw 'Unexpected UDP sender/length'}
  $packets.Add(@{address=$endpoint.Address.ToString();sourcePort=$endpoint.Port;sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant();bytes=$bytes;utc=[DateTime]::UtcNow.ToString('o')})
 }
}
try{
 if($Mode -eq 'Rehearse'){
  CheckRows $plan.appRows
  & pwsh -NoProfile -File "$PSScriptRoot/../../tests/Test-OrdinaryContract.ps1" -Evidence "$run/contract"|Out-Null;if($LASTEXITCODE){throw 'Offline parent rehearsal failed'}
  $hostImage=[FzeroLive]::ProcessIdentity($PID).image
  $id=Launch $hostImage @('-NoProfile','-NonInteractive','-File',"$PSScriptRoot/Invoke-FzeroFixtureApply.ps1",'-App',$plan.app,'-Profile',"$Evidence/profile.json",'-Evidence',$run) 'helper'
  $p=Get-Process -Id $id.pid;$null=$p.Handle;if(-not $p.WaitForExit(45000)){throw 'Offline helper timeout'};if($p.ExitCode){throw 'Offline helper fixture failed'};$p.Dispose()
  $applied=Json "$run/apply-result.json";if(-not $applied.pass){throw 'Helper apply not established'}
  $config=Read-FzeroSharedText "$($plan.app)/config.ini"
  if(-not $config.Contains('Keep=unchanged') -or -not $config.Contains('SteeringAxis=7')){throw 'Apply touched unrelated/staged native keys'}
  WriteJson "$Evidence/ready.json" @{revision=$applied.revision;appRows=Rows $plan.app;rehearsal=Get-ReadbackSha "$run/contract/result.json"}
  $result.apply=$applied;$result.pass=$true
 }else{
  if(-not $Go -or -not $OwnerRoot -or -not(Test-Path $OwnerRoot -PathType Container)){throw 'Live GO and exact owner root required'}
  $ready=Json "$Evidence/ready.json";CheckRows $ready.appRows
  if(-not (Json "$Evidence/rehearse/result.json").pass -or (Get-ReadbackSha "$Evidence/rehearse/contract/result.json") -cne $ready.rehearsal){throw 'Passing rehearsal required'}
  foreach($p in @($Evidence,$plan.saveRoot)){if((Test-ReadbackWithin $p $OwnerRoot) -or (Test-ReadbackWithin $OwnerRoot $p)){throw 'Owner/scratch overlap'}}
  & "$Hub/tools/Test-DesktopRoutes.ps1" -GameSeamPath "$PSScriptRoot/../../src/sdl_main.c" -CaptureToolPath $plan.capture -OutputPath "$run/routes.json"|Out-Null;if($LASTEXITCODE){throw 'Route inventory failed'}
  if([FzeroLive]::IdleSeconds() -lt 180){throw 'Owner idle below180s'}
  & "$Hub/tools/Rig-Lease.ps1" -Enter -Name napo-fzero -IdleMinutes 3 -Purpose "F-Zero ordinary raw/Apply/F6/owned UDP GO$Go"|Set-Content "$run/lease.txt";$lease=$true
  $roots=@('config.ini','fzero-video.ini','keybinds.ini','rom.cfg','install-receipt.json'|ForEach-Object{@{kind='file';path=Join-Path $OwnerRoot $_}})+@{kind='directory';path=Join-Path $env:LOCALAPPDATA 'dbce/fzero'}
  $snapshot=Save-ReadbackSnapshot $roots "$run/owner-snapshot"
  @($snapshot.roots|ForEach-Object files|ForEach-Object{@{path=$_.path;sha256=$_.sha256;attributes=$_.attributes;utcTicks=[string](Convert-ReadbackUtc $_.lastWriteUtc).Ticks}})|ConvertTo-Json -Depth 5|Set-Content "$run/owner-expected.json"
  $sink=[Net.Sockets.UdpClient]::new([Net.Sockets.AddressFamily]::InterNetwork);$sink.ExclusiveAddressUse=$true;$sink.Client.ReceiveBufferSize=1048576
  $sink.Client.Bind([Net.IPEndPoint]::new([Net.IPAddress]::Loopback,0));$port=$sink.Client.LocalEndPoint.Port;if($port -eq 8000 -or $port -lt 1024){throw 'Unsafe sink port'}
  $config=Read-FzeroSharedText "$($plan.app)/config.ini";$config=[regex]::Replace($config,'(?m)^\[Telemetry\]\r?\nEnabled=0',"[Telemetry]`nEnabled=1`nHost=127.0.0.1`nPort=$port")
  if($config -notmatch "(?m)^Port=$port\r?$"){throw 'Owned telemetry config not changed'};[IO.File]::WriteAllText("$($plan.app)/config.ini",$config)
  $nonce=[guid]::NewGuid().ToString('N');$gate="$run/gate.txt";$request="$run/request.txt";$statusPath="$($plan.saveRoot)/status.json"
  Set-FzeroOrdinaryGate $gate $nonce ([DateTime]::UtcNow.AddSeconds(2).ToFileTimeUtc())
  [IO.File]::WriteAllText($request,"schema=fzero.ordinary-r1@1`nnonce=$nonce`nroot=$($plan.saveRoot)`ngate=$gate`nexpiresFileTime=$([DateTime]::UtcNow.AddSeconds(90).ToFileTimeUtc())`nport=$port`n")
  $env:FZERO_ORDINARY_R1_REQUEST=$request;$env:SNESRECOMP_SAVE_ROOT=$plan.saveRoot;$env:SDL_AUDIODRIVER='dummy';$env:FZERO_ASPECT='4:3'
  $env:SNESRECOMP_FRAME_BMP="$run/final.bmp";$env:SNESRECOMP_WRAM_DUMP="$run/final.wram"
  $exe=Join-Path $plan.app $plan.candidate.launcher;$hooks=$true;[FzeroLive]::Start($exe)
  if([FzeroLive]::IdleSeconds() -lt 180){throw 'Owner returned before launch'}
  $owned=Launch $exe @("$($plan.app)/fzero.sfc") 'game' $true;$result.gameStarted=$true;WriteJson "$run/process.json" $owned
  $p=Get-Process -Id $owned.pid;$null=$p.Handle;$deadline=[DateTime]::UtcNow.AddSeconds(80);$lastAdvance=[DateTime]::UtcNow;$seq=0;$hwnd=0L;$captureNames=@();$baseline=$null
  while(-not $p.HasExited){
   if([DateTime]::UtcNow -gt $deadline){throw 'Ordinary live deadline'}
   $guard=[FzeroLive]::Snapshot();if($guard.abortReason){throw $guard.abortReason};if(-not $guard.hooksActive -or $guard.heartbeat -lt 1){throw 'LL recorder unhealthy'}
   Set-FzeroOrdinaryGate $gate $nonce ([DateTime]::UtcNow.AddSeconds(2).ToFileTimeUtc());ReceivePackets
   if(Test-Path $statusPath){
    $s=Get-FzeroOrdinaryAdmission (Json $statusPath) $nonce $owned $port $ready.revision $seq
    if($s.seq -gt $seq){$seq=$s.seq;$lastAdvance=[DateTime]::UtcNow;$observations.Add(@{utc=$lastAdvance.ToString('o');status=$s;foreground=$guard.current})}
    if(-not $baseline -and $s.frame -gt 0 -and $s.frame -lt 600){
     $videoPath="$($plan.app)/fzero-video.ini";[IO.File]::AppendAllText($videoPath,"; ordinary F6 preservation marker`n[FixtureUnknown]`nKeep=byte-for-byte`n");$baseline=[IO.File]::ReadAllText($videoPath);[IO.File]::WriteAllText("$run/video-before-f6.ini",$baseline)
    }
    if(-not $hwnd){$hwnd=[FzeroLive]::Window($owned.pid,$owned.birth)}
    $name=if($s.phase -ceq 'panel' -and $s.showFps -eq 1 -and 'panel' -notin $captureNames){'panel'}elseif($s.frame -ge 1820 -and $s.frame -lt 2300 -and 'race' -notin $captureNames){'race'}else{$null}
    if($name -and $hwnd -and -not $capture){
     $before=[FzeroLive]::Foreground();$ci=Launch $plan.capture @([string]$owned.pid,"$run/$name.png",'--hwnd',('0x'+$hwnd.ToString('X'))) "capture-$name"
     $capture=Get-Process -Id $ci.pid;$null=$capture.Handle;$captureUntil=[DateTime]::UtcNow.AddSeconds(8);$captureNames+=$name
     WriteJson "$run/capture-$name-identity.json" @{game=$owned;hwnd=$hwnd;before=$before;child=$ci;status=$s}
    }
   }
   if(([DateTime]::UtcNow-$lastAdvance).TotalSeconds -gt 5){throw 'Ordinary status stalled'}
   if($capture){if($capture.HasExited){if($capture.ExitCode){throw 'Exact HWND capture failed'};if(([FzeroLive]::Foreground()).Key -cne $before.Key){throw 'Capture changed foreground'};$capture.Dispose();$capture=$null}elseif([DateTime]::UtcNow -gt $captureUntil){throw 'Capture deadline'}}
   Start-Sleep -Milliseconds 40
  }
  $p.WaitForExit();$result.exitCode=$p.ExitCode;$p.Dispose();[FzeroLive]::BeginCleanup();ReceivePackets
  if($result.exitCode -ne 0){throw 'Ordinary host failed'}
  $s=Get-FzeroOrdinaryAdmission (Json $statusPath) $nonce $owned $port $ready.revision $seq
  if($s.phase -cne 'done' -or $s.frame -lt 3299 -or $s.frame -gt 3302 -or $s.panelSaved -ne 1 -or $s.showFps -ne 1 -or $s.panelOpen -ne 0 -or $capture -or 'race' -notin $captureNames -or 'panel' -notin $captureNames){throw 'Ordinary panel/capture/final state incomplete'}
  $trace=@(Get-Content "$($plan.saveRoot)/frames.jsonl"|ForEach-Object{$_|ConvertFrom-Json -DateKind String});if(@($trace|Where-Object nonce -CNE $nonce).Count){throw 'Trace nonce mismatch'}
  $result.rows=Get-FzeroOrdinaryRows $trace $packets $s.sourcePort
  $idle=@($packets|Where-Object {$_.sha256 -ceq $s.packetSha256 -and $_.sourcePort -eq $s.sourcePort});if($idle.Count -ne 1 -or [BitConverter]::ToInt32($idle[0].bytes,0) -ne 0){throw 'Shutdown idle packet not received'}
  $after=[IO.File]::ReadAllText("$($plan.app)/fzero-video.ini");[IO.File]::WriteAllText("$run/video-after-f6.ini",$after)
  if(-not $baseline -or [regex]::Replace($after,'(?m)^ShowFPS=1','ShowFPS=0') -cne $baseline){throw 'F6 row save changed unrelated bytes'}
  $finalConfig=Read-FzeroSharedText "$($plan.app)/config.ini";if(-not $finalConfig.Contains('Keep=unchanged') -or $finalConfig -notmatch '(?m)^SteeringAxis\s*=\s*0\s*$' -or $finalConfig -notmatch '(?m)^AcceleratorAxis\s*=\s*1\s*$' -or $finalConfig -notmatch '(?m)^BrakeAxis\s*=\s*2\s*$'){throw 'Native Apply readback incomplete'}
  foreach($name in $captureNames){$bytes=[IO.File]::ReadAllBytes("$run/$name.png");if($bytes.Length -lt 1000 -or [Convert]::ToHexString($bytes[0..7]) -cne '89504E470D0A1A0A'){throw 'Capture artifact invalid'}}
  $result.port=$port;$result.sourcePort=$s.sourcePort;$result.final=$s;$result.captures=$captureNames;$result.pass=$true
 }
}catch{$result.error=$_.Exception.Message;$result.failureLine=$_.ScriptStackTrace}
finally{
 if($hooks){[FzeroLive]::BeginCleanup()}
 if($gate){try{Set-FzeroOrdinaryGate $gate $nonce 0}catch{$result.revokeError=$_.Exception.Message;$result.pass=$false}}
 try{
  if(@([FzeroLive]::JobPids()).Count){
   $until=[DateTime]::UtcNow.AddSeconds(3);while(@([FzeroLive]::JobPids()).Count -and [DateTime]::UtcNow -lt $until){Start-Sleep -Milliseconds 50}
   if(@([FzeroLive]::JobPids()).Count){$result.beforeStop=@([FzeroLive]::JobPids()|ForEach-Object{[FzeroLive]::ProcessIdentity($_)});[FzeroLive]::KillOwned();$result.forcedOwnCleanup=$true;$result.pass=$false;$until=[DateTime]::UtcNow.AddSeconds(5);while(@([FzeroLive]::JobPids()).Count -and [DateTime]::UtcNow -lt $until){Start-Sleep -Milliseconds 50}}
  }
  [FzeroLive]::ReleaseJob();$result.jobEmpty=$true
  $result.processCensus=@(foreach($id in $known){Get-FzeroOrdinaryQuiescence $id})
  if($hooks){[FzeroLive]::Stop();$guard=[FzeroLive]::Snapshot();WriteJson "$run/guard.json" $guard;if($guard.abortReason){$result.pass=$false;$result.guardAbort=$guard.abortReason};[FzeroLive]::VerifyDisplay();$result.hooksReleased=$true}
  if($sink){ReceivePackets;$sink.Dispose();$sink=$null;$result.sinkClosed=$true}
  if($snapshot){Restore-ReadbackSnapshot $snapshot "$run/owner-after";$verified=Test-ReadbackRestored $snapshot;WriteJson "$run/owner-restore.json" $verified;if(-not $verified.pass){throw 'Owner restore mismatch'}
   & pwsh -NoProfile -File "$PSScriptRoot/../../tests/Verify-ReadbackFiles.ps1" -Expected "$run/owner-expected.json" -Report "$run/owner-independent.json"|Out-Null;if($LASTEXITCODE){throw 'Independent owner restore failed'}}
  CheckRows $plan.sources
  $immutable=if($Mode -eq 'Live'){$ready.appRows}else{$plan.appRows}
  $mutable=@((Join-Path $plan.app 'config.ini'),(Join-Path $plan.app 'fzero-video.ini'))
  CheckRows @($immutable|Where-Object {$_.path -notin $mutable})
  if($lease){$released=& "$Hub/tools/Rig-Lease.ps1" -Exit -Name napo-fzero;$released|Set-Content "$run/release.txt";if($released -notcontains 'lease released'){throw 'Lease release unconfirmed'};$result.leaseReleased=$true}
 }catch{$result.cleanupError=$_.Exception.Message;$result.pass=$false;$result.retainCustody=$lease}
 WriteJson "$run/observations.json" @($observations.ToArray());WriteJson "$run/packets.json" @($packets.ToArray());WriteJson "$run/known-processes.json" @($known.ToArray())
 foreach($e in Get-ChildItem Env:){if($e.Name -match '^(FZERO_|SNESRECOMP_|SDL_)'){[Environment]::SetEnvironmentVariable($e.Name,$null,'Process')}};foreach($key in $envBefore.Keys){[Environment]::SetEnvironmentVariable($key,$envBefore[$key],'Process')}
 WriteJson "$run/result.json" $result
}
$result|ConvertTo-Json -Depth 7;if(-not $result.pass){exit 1}
