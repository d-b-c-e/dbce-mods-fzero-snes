# Real status writer and real consumer admission, no SDL/game/hardware/lease.
param([Parameter(Mandatory)][string]$Evidence)
$ErrorActionPreference='Stop'
. "$PSScriptRoot/../tools/rig/Session-Readback.ps1"
if(Test-Path $Evidence){throw 'Fresh evidence required'}
[void][IO.Directory]::CreateDirectory($Evidence);$Evidence=[IO.Path]::GetFullPath($Evidence)
$root=Split-Path $PSScriptRoot;$exe=Join-Path $Evidence status-probe.exe
& gcc -std=c17 -Wall -Wextra -Werror -I "$root/src" "$PSScriptRoot/readback_status_probe.c" "$root/src/fzero_session_control.c" "$root/src/fzero_session_ui.c" "$root/src/fzero_playthrough.c" -o $exe
if($LASTEXITCODE){throw 'Status probe compile failed'}
$si=[Diagnostics.ProcessStartInfo]::new($exe);$si.UseShellExecute=$false;$si.CreateNoWindow=$true
foreach($name in @('status.json','starting.json','playing.json')){$si.ArgumentList.Add((Join-Path $Evidence $name))}
$p=[Diagnostics.Process]::Start($si);@{pid=$p.Id;birth=$p.StartTime.ToUniversalTime().Ticks;image=$exe}|ConvertTo-Json|Set-Content "$Evidence/process.json"
if(-not $p.WaitForExit(10000) -or $p.ExitCode -ne 0){throw 'Status probe failed; preserve process identity for recovery'}
$starting=Get-Content "$Evidence/starting.json" -Raw|ConvertFrom-Json
$playing=Get-Content "$Evidence/playing.json" -Raw|ConvertFrom-Json
$identity=@{pid=$starting.processId;birth=[long]$starting.processCreationFileTime+504911232000000000}
$request=$starting.requestId;$rom='bf16c3c867c58e2ab061c70de9295b6930d63f29f81cc986f5ecae03e0ad18d2';$script:n=0
function Check($v,$why){if(-not $v){throw $why};$script:n++}
function Admit($s,[bool]$seen=$false){Get-FzeroReplayAdmission $s $request $identity $rom 3600 $seen}
Check (-not (Admit $starting).ready) 'Initial production record must be pending, not rejected or ready'
Check ((Admit $playing).ready) 'Actual producer playing record refused'
$bad=$playing|ConvertTo-Json|ConvertFrom-Json;$bad.state='verified';$bad.frames=3600
Check ((Admit $bad $true).ready) 'Terminal verified record refused'
foreach($case in @(
 @{base=$starting;field='schema';value='other'},@{base=$starting;field='requestId';value='other'},
 @{base=$starting;field='processId';value=0},@{base=$starting;field='processCreationFileTime';value=0},
 @{base=$starting;field='outputMuted';value=$false},@{base=$starting;field='frames';value=1},
 @{base=$starting;field='total';value=3600},@{base=$starting;field='effectiveRomSha256';value=$rom},
 @{base=$playing;field='effectiveRomSha256';value=''},@{base=$playing;field='total';value=0},
 @{base=$playing;field='frames';value=-1},@{base=$playing;field='frames';value=3601},
 @{base=$playing;field='state';value='failed'})){
 $bad=$case.base|ConvertTo-Json|ConvertFrom-Json;$bad.($case.field)=$case.value
 $refused=$false;try{Admit $bad|Out-Null}catch{$refused=$true};Check $refused ('Unsafe status admitted: '+$case.field)
}
$refused=$false;try{Admit $starting $true|Out-Null}catch{$refused=$true};Check $refused 'Starting regression after ready admitted'
@{pass=$true;checks=$script:n;producer=$exe;scope='production writer startup/playing bytes and consumer; no game'}|ConvertTo-Json|Tee-Object -FilePath "$Evidence/result.json"
