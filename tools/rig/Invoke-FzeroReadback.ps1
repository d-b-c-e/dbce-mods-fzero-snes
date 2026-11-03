#Requires -Version 7.0
param(
 [Parameter(Mandatory)][ValidateSet('Prepare','Rehearse','Live')][string]$Mode,
 [Parameter(Mandatory)][string]$Evidence,
 [string]$PackageRoot,[string]$CaseRoot,[string]$CaptureTool,
 [string]$SaveRoot,[string]$OwnerRoot,[string]$Go,
 [string]$Hub='E:/Source/dbce-project-mgmt'
)
$ErrorActionPreference='Stop'
. "$PSScriptRoot/Readback-Contract.ps1"
function Json($p){Get-Content -LiteralPath $p -Raw|ConvertFrom-Json -DateKind String}
function WriteJson($p,$o){$o|ConvertTo-Json -Depth 12|Set-Content -LiteralPath $p}
function Quote([string]$s){if($s.Contains('"') -or $s.EndsWith('\')){throw 'Unsupported command argument'};'"'+$s+'"'}
function Rows([string]$root){@(Get-ChildItem -LiteralPath $root -File -Recurse -Force|ForEach-Object{Assert-ReadbackPlain $_.FullName;@{path=$_.FullName;sha256=Get-ReadbackSha $_.FullName}})}
function CheckRows($rows){foreach($r in $rows){if((Get-ReadbackSha $r.path) -ine $r.sha256){throw "Pinned input changed: $($r.path)"}}}
function Tape([string]$p){$f=[IO.File]::OpenRead($p);try{$h=[byte[]]::new(49);$f.ReadExactly($h);$n=[BitConverter]::ToUInt64($h,40);if([Text.Encoding]::ASCII.GetString($h,0,8) -cne 'FZPT0001' -or $h[48] -ne 1 -or $n -ne 3600 -or $f.Length -ne 49+12*$n){throw 'Expected sealed 3600-frame stock case'};@{frames=$n;rom=[Convert]::ToHexString($h[8..39]).ToLowerInvariant()}}finally{$f.Dispose()}}
$Evidence=[IO.Path]::GetFullPath($Evidence)
if($Mode -eq 'Prepare'){
 if(Test-Path -LiteralPath $Evidence){throw 'Fresh evidence root required'}
 foreach($p in @($PackageRoot,$CaseRoot,$CaptureTool,$SaveRoot)){if(-not $p -or -not [IO.Path]::IsPathFullyQualified($p)){throw 'Absolute package/case/capture/save paths required'};Assert-ReadbackPlain $p}
 if($SaveRoot.Length -gt 65 -or (Test-Path -LiteralPath $SaveRoot)){throw 'Fresh short save root required'}
 $manifest=Json "$PackageRoot/manifest.json"
 if($manifest.build.sourceRevision -cne '9dc40e99f5d3b5bc6d17f44c5ee2fbfd0dba2914' -or $manifest.build.bsDeluxe -ne $false -or $manifest.build.dirty -ne $false){throw 'Only receipted Oct30 stock candidate admitted'}
 foreach($m in $manifest.files.PSObject.Properties){$p=Join-Path $PackageRoot $m.Name;if(-not(Test-ReadbackWithin $p $PackageRoot) -or (Get-ReadbackSha $p) -ine $m.Value){throw "Package identity failed: $($m.Name)"}}
 $t=Tape "$CaseRoot/diagnostic.fzpt"
 if((Get-ReadbackSha "$CaseRoot/fzero.sfc").ToLowerInvariant() -cne $t.rom -or $t.rom -cne 'bf16c3c867c58e2ab061c70de9295b6930d63f29f81cc986f5ecae03e0ad18d2'){throw 'Stock ROM/tape mismatch'}
 [void][IO.Directory]::CreateDirectory($Evidence)
 $app=Join-Path $Evidence app;[void][IO.Directory]::CreateDirectory($app)
 # Literal source enumeration avoids wildcard ambiguity; package inventory was verified above.
 Get-ChildItem -LiteralPath $PackageRoot -Force|Copy-Item -Destination $app -Recurse -Force
 foreach($name in @('diagnostic.fzpt','diagnostic.fzpt.state','fzero.sfc')){Copy-Item -LiteralPath "$CaseRoot/$name" -Destination $app}
 [IO.File]::WriteAllText("$app/config.ini","[General]`nSkipLauncher=1`n[Graphics]`nFullscreen=0`nWindowScale=3`n[Sound]`nMsu1Enabled=0`nVolume=0`n[ForceFeedback]`nEnabled=0`n[Telemetry]`nEnabled=0`n")
 [IO.File]::WriteAllText("$app/fzero-video.ini","[FZeroVideo]`nBSDeluxe=0`nAspect=16:9`nEnhancedRenderer=1`n")
 $verifier=Join-Path $PSScriptRoot '../../tests/Verify-ReadbackFiles.ps1'
 $sourceRows=@(Rows $PSScriptRoot)+@(Rows (Split-Path $CaptureTool))+@{path=[IO.Path]::GetFullPath($verifier);sha256=Get-ReadbackSha $verifier}
 $sources=@(Rows $PackageRoot)+@('diagnostic.fzpt','diagnostic.fzpt.state','fzero.sfc'|ForEach-Object{@{path=Join-Path $CaseRoot $_;sha256=Get-ReadbackSha (Join-Path $CaseRoot $_)}})
 WriteJson "$Evidence/prepared.json" @{schema='fzero-readback-prepared@1';app=$app;source=$manifest.build.sourceRevision;saveRoot=$SaveRoot;capture=$CaptureTool;captureRows=$sourceRows;sources=$sources;appRows=Rows $app;frames=$t.frames;rom=$t.rom}
 Write-Output 'Prepared exact stock package and sealed case; no executable launched.';return
}
$plan=Json "$Evidence/prepared.json"
if($plan.schema -cne 'fzero-readback-prepared@1'){throw 'Wrong preparation schema'}
CheckRows $plan.appRows;CheckRows $plan.sources;CheckRows $plan.captureRows
if(Get-Process|Where-Object ProcessName -Like 'FZeroSNESRecomp*'){throw 'Existing F-Zero process: refused'}
Add-Type -Path "$PSScriptRoot/FzeroLive.cs"
$run=Join-Path $Evidence $Mode.ToLowerInvariant();if(Test-Path $run){throw 'One invocation per mode/evidence'};[void][IO.Directory]::CreateDirectory($run)
$app=$plan.app;$exe=Join-Path $app $(if($Mode -eq 'Live'){'FZeroSNESRecomp.exe'}else{'FZeroSNESRecompHeadless.exe'})
$save=Join-Path $plan.saveRoot $Mode.ToLowerInvariant();[void][IO.Directory]::CreateDirectory($save)
$envBefore=@{};foreach($e in Get-ChildItem Env:){if($e.Name -match '^(FZERO_|SNESRECOMP_|SDL_)'){$envBefore[$e.Name]=$e.Value;[Environment]::SetEnvironmentVariable($e.Name,$null,'Process')}}
$env:SNESRECOMP_SAVE_ROOT=$save;$env:SDL_AUDIODRIVER='dummy';$env:FZERO_ASPECT='16:9';$env:FZERO_REPLAY_PLAYTHROUGH=Join-Path $app diagnostic.fzpt
$env:SNESRECOMP_FRAME_BMP=Join-Path $run final.bmp;$env:SNESRECOMP_WRAM_DUMP=Join-Path $run final.wram
$lease=$false;$hooks=$false;$snapshot=$null;$owned=$null;$result=@{mode=$Mode;pass=$false;leaseReleased=$false;error=$null;source=$plan.source;frames=0}
$request=[guid]::NewGuid().ToString('N');$statusPath=Join-Path $run status.json
try{
 if($Mode -eq 'Live'){
  if(-not $Go -or -not $OwnerRoot -or -not(Test-Path -LiteralPath $OwnerRoot -PathType Container)){throw 'Live requires GO and exact owner root'}
  foreach($p in @($Evidence,$plan.saveRoot)){if((Test-ReadbackWithin $p $OwnerRoot) -or (Test-ReadbackWithin $OwnerRoot $p)){throw 'Scratch and owner roots overlap'}}
  $offline=Json "$Evidence/rehearse/result.json";if(-not $offline.pass){throw 'Passing offline rehearsal required'}
  & "$Hub/tools/Test-DesktopRoutes.ps1" -GameSeamPath "$PSScriptRoot/../../src/sdl_main.c" -CaptureToolPath $plan.capture -OutputPath "$run/routes.json"|Out-Null
  if($LASTEXITCODE){throw 'Desktop route inventory failed'}
  if([FzeroLive]::IdleSeconds() -lt 180){throw 'Owner idle below180s'}
  & "$Hub/tools/Rig-Lease.ps1" -Enter -Name napo-fzero -IdleMinutes 3 -Purpose "F-Zero stock replay R1/R2 GO$Go"|Set-Content "$run/lease.txt";$lease=$true
  $roots=@('config.ini','fzero-video.ini','keybinds.ini','rom.cfg','install-receipt.json'|ForEach-Object{@{kind='file';path=Join-Path $OwnerRoot $_}})
  $roots+=@{kind='directory';path=Join-Path $env:LOCALAPPDATA 'dbce/fzero'}
  $snapshot=Save-ReadbackSnapshot $roots "$run/owner-snapshot"
  @($snapshot.roots|ForEach-Object files|ForEach-Object{@{path=$_.path;sha256=$_.sha256;attributes=$_.attributes;utcTicks=[string](Convert-ReadbackUtc $_.lastWriteUtc).Ticks}})|ConvertTo-Json -Depth 5|Set-Content "$run/owner-expected.json"
  $hooks=$true;[FzeroLive]::Start($exe)
  if([FzeroLive]::IdleSeconds() -lt 180){throw 'Owner returned before launch'}
 }
 $args=if($Mode -eq 'Live'){@('--replay',(Join-Path $app diagnostic.fzpt),'--session-id',$request,'--session-status',$statusPath,(Join-Path $app fzero.sfc))}else{@((Join-Path $app fzero.sfc))}
 $command=(@($exe)+$args|ForEach-Object{Quote $_}) -join ' '
 $owned=[FzeroLive]::Launch($exe,$app,$command,"$run/stdout.txt","$run/stderr.txt",($Mode -eq 'Live'))
 $p=Get-Process -Id $owned.pid;$null=$p.Handle
 WriteJson "$run/process.json" $owned
 $deadline=[DateTime]::UtcNow.AddSeconds($(if($Mode -eq 'Live'){80}else{30}));$start=[DateTime]::UtcNow;$seenFrames=-1;$lastAdvance=$start;$captures=0;$hwnd=0L;$capture=$null;$observations=@()
 while(-not $p.HasExited){
  if([DateTime]::UtcNow -gt $deadline){throw 'Run deadline'}
  if($Mode -eq 'Live'){
   $guard=[FzeroLive]::Snapshot();if($guard.abortReason){throw $guard.abortReason}
   if(-not $guard.hooksActive -or $guard.heartbeat -lt 1){throw 'LL recorder not healthy'}
   if(Test-Path $statusPath){
    $s=Json $statusPath
    if($s.requestId -cne $request -or $s.processId -ne $owned.pid -or [long]$s.processCreationFileTime -ne ($owned.birth-504911232000000000) -or $s.outputMuted -ne $true -or $s.effectiveRomSha256 -cne $plan.rom -or $s.total -ne $plan.frames -or $s.action -cne 'replay' -or $s.state -notin @('starting','playing','verified')){throw 'Session identity/mute/status mismatch'}
    if($s.frames -lt $seenFrames){throw 'Session frame regressed'}
    if($s.frames -gt $seenFrames){$seenFrames=$s.frames;$lastAdvance=[DateTime]::UtcNow;$observations+=@{utc=$lastAdvance.ToString('o');frames=$s.frames;state=$s.state;foreground=$guard.current}}
   }
   if(([DateTime]::UtcNow-$lastAdvance).TotalSeconds -gt 10){throw 'Replay status stalled'}
   if(-not $hwnd){$hwnd=[FzeroLive]::Window($owned.pid,$owned.birth)}
   if($hwnd -and -not $capture -and $captures -lt 2 -and $seenFrames -ge (600+1200*$captures)){
    $before=[FzeroLive]::Foreground();$png=Join-Path $run "capture-$captures.png"
    $cmd=(@($plan.capture,[string]$owned.pid,$png,'--hwnd',('0x'+$hwnd.ToString('X')))|ForEach-Object{Quote $_}) -join ' '
    $ci=[FzeroLive]::Launch($plan.capture,$app,$cmd,"$run/capture-$captures.out","$run/capture-$captures.err",$false)
    $capture=Get-Process -Id $ci.pid;$null=$capture.Handle;$captureUntil=[DateTime]::UtcNow.AddSeconds(8);$captures++
    WriteJson "$run/capture-$($captures-1)-identity.json" @{game=$owned;hwnd=$hwnd;before=$before;child=$ci;frame=$seenFrames}
   }
   if($capture){if($capture.HasExited){if($capture.ExitCode -ne 0){throw 'R2 capture failed'};$after=[FzeroLive]::Foreground();if($after.Key -cne $before.Key){throw 'R2 foreground changed'};$capture.Dispose();$capture=$null}elseif([DateTime]::UtcNow -gt $captureUntil){throw 'R2 capture timeout'}}
  }
  Start-Sleep -Milliseconds 50
 }
 $p.WaitForExit();$result.exitCode=$p.ExitCode
 if($p.ExitCode -ne 0){throw 'Game exit failed'}
 $log=Get-Content "$run/stderr.txt" -Raw
 if($Mode -eq 'Live'){
  $s=Json $statusPath;if($s.state -cne 'verified' -or $s.frames -ne $plan.frames -or -not $s.outputMuted -or $captures -ne 2 -or $capture){throw 'Replay/capture incomplete'}
  if(-not(Test-Path "$run/final.bmp") -or -not(Test-Path "$run/final.wram")){throw 'Game-owned final frame/state absent'}
  if((Get-ReadbackSha "$run/final.wram") -cne (Get-ReadbackSha "$Evidence/rehearse/final.wram")){throw 'Graphical/headless final WRAM differs'}
  $images=@();foreach($i in 0..1){$png=Join-Path $run "capture-$i.png";$bytes=[IO.File]::ReadAllBytes($png);if($bytes.Length -lt 1000 -or [Convert]::ToHexString($bytes[0..7]) -cne '89504E470D0A1A0A'){throw 'R2 PNG invalid'};$w=[uint32]$bytes[16]*16777216+[uint32]$bytes[17]*65536+[uint32]$bytes[18]*256+$bytes[19];$h=[uint32]$bytes[20]*16777216+[uint32]$bytes[21]*65536+[uint32]$bytes[22]*256+$bytes[23];if($w -lt 100 -or $h -lt 100 -or $w -gt 8192 -or $h -gt 4096){throw 'R2 dimensions invalid'};$images+=@{path=$png;sha256=Get-ReadbackSha $png;width=$w;height=$h}}
  if($images[0].sha256 -ceq $images[1].sha256){throw 'R2 frames did not change'}
  WriteJson "$run/images.json" $images
  $result.frames=$s.frames;$result.captures=$captures
 }elseif($log -notmatch 'PASS frames=3600'){throw 'Headless WRAM replay did not verify3600'}else{$result.frames=3600}
 $result.pass=$true
}catch{$result.error=$_.Exception.Message}
finally{
 if($hooks){[FzeroLive]::BeginCleanup()}
 try{
  if(@([FzeroLive]::JobPids()).Count){
   if($owned -and $Mode -eq 'Live'){try{$h=[FzeroLive]::Window($owned.pid,$owned.birth);if($h){[void][FzeroLive]::PostMessage([IntPtr]$h,0x10,[IntPtr]::Zero,[IntPtr]::Zero)}}catch{}}
   $until=[DateTime]::UtcNow.AddSeconds(5);while(@([FzeroLive]::JobPids()).Count -and [DateTime]::UtcNow -lt $until){Start-Sleep -Milliseconds 50}
   if(@([FzeroLive]::JobPids()).Count){$result.forcedClose=$true;$result.pass=$false;[FzeroLive]::KillOwned();$until=[DateTime]::UtcNow.AddSeconds(5);while(@([FzeroLive]::JobPids()).Count -and [DateTime]::UtcNow -lt $until){Start-Sleep -Milliseconds 50}}
  }
  [FzeroLive]::ReleaseJob();$result.jobEmpty=$true
  if($hooks){[FzeroLive]::Stop();$finalGuard=[FzeroLive]::Snapshot();WriteJson "$run/guard.json" $finalGuard;if($finalGuard.abortReason){$result.pass=$false;$result.guardAbort=$finalGuard.abortReason};[FzeroLive]::VerifyDisplay();$result.hooksReleased=$true}
  if($snapshot){Restore-ReadbackSnapshot $snapshot "$run/owner-after";$verification=Test-ReadbackRestored $snapshot;WriteJson "$run/owner-restore.json" $verification;if(-not $verification.pass){throw 'Owner restore mismatch'}
   & pwsh -NoProfile -File "$PSScriptRoot/../../tests/Verify-ReadbackFiles.ps1" -Expected "$run/owner-expected.json" -Report "$run/owner-independent.json"|Out-Null;if($LASTEXITCODE){throw 'Independent owner verification failed'}}
  CheckRows $plan.appRows;CheckRows $plan.sources
  if($lease){$released=& "$Hub/tools/Rig-Lease.ps1" -Exit -Name napo-fzero;$released|Set-Content "$run/release.txt";if($released -notcontains 'lease released'){throw 'Lease release not confirmed'};$result.leaseReleased=$true}
 }catch{$result.pass=$false;$result.cleanupError=$_.Exception.Message;$result.retainCustody=$lease}
 if($Mode -eq 'Live'){WriteJson "$run/observations.json" @($observations)}
 foreach($e in Get-ChildItem Env:){if($e.Name -match '^(FZERO_|SNESRECOMP_|SDL_)'){[Environment]::SetEnvironmentVariable($e.Name,$null,'Process')}};foreach($key in $envBefore.Keys){[Environment]::SetEnvironmentVariable($key,$envBefore[$key],'Process')}
 WriteJson "$run/result.json" $result
}
$result|ConvertTo-Json -Depth 5
if(-not $result.pass){exit 1}
