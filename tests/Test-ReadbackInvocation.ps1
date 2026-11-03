# The actual runner invocation is passed to the real C ingress parser, without SDL/game/assets.
param([string]$Evidence)
$ErrorActionPreference='Stop'
. "$PSScriptRoot/../tools/rig/Replay-Invocation.ps1"
if(-not $Evidence){throw 'Explicit fresh evidence folder required'}
if(Test-Path $Evidence){throw 'Fresh evidence folder required'}
[void][IO.Directory]::CreateDirectory($Evidence);$Evidence=[IO.Path]::GetFullPath($Evidence)
$root=Split-Path $PSScriptRoot;$exe=Join-Path $Evidence ingress.exe
& gcc -std=c17 -Wall -Wextra -Werror -I "$root/src" "$PSScriptRoot/readback_cli_probe.c" "$root/src/fzero_session_ui.c" "$root/src/fzero_playthrough.c" -o $exe
if($LASTEXITCODE){throw 'Production parser probe compile failed'}
function Probe($invocation,$override){
 $si=[Diagnostics.ProcessStartInfo]::new($exe);$si.UseShellExecute=$false;$si.CreateNoWindow=$true;$si.RedirectStandardOutput=$true;$si.RedirectStandardError=$true
 foreach($key in @($si.Environment.Keys)){if($key -match '^(FZERO_|SNESRECOMP_)'){[void]$si.Environment.Remove($key)}}
 if($invocation.replayEnvironment){$si.Environment['FZERO_REPLAY_PLAYTHROUGH']=$invocation.replayEnvironment}
 if($override){$si.Environment['FZERO_REPLAY_PLAYTHROUGH']=$override}
 foreach($arg in $invocation.arguments){$si.ArgumentList.Add($arg)}
 $p=[Diagnostics.Process]::Start($si);$out=$p.StandardOutput.ReadToEnd();$err=$p.StandardError.ReadToEnd();$p.WaitForExit();@{exit=$p.ExitCode;stdout=$out;stderr=$err}
}
$request='0123456789abcdef0123456789abcdef'
$live=Get-FzeroReplayInvocation Live 'C:/scratch app' $request 'C:/scratch/status.json'
$headless=Get-FzeroReplayInvocation Rehearse 'C:/scratch app' $request 'C:/scratch/status.json'
$a=Probe $live $null;$b=Probe $headless $null;$c=Probe $live 'C:/scratch app/diagnostic.fzpt'
if($a.exit -ne 0 -or $a.stdout.Trim() -cne 'mode=2 env=0'){throw 'Live CLI rejected by real production parser'}
if($b.exit -ne 0 -or $b.stdout.Trim() -cne 'mode=0 env=1'){throw 'Headless environment route changed'}
if($c.exit -ne 2 -or $c.stderr -notlike '*Explicit session conflicts*'){throw 'Failed-run conflict not reproduced'}
$result=@{pass=$true;checks=3;live=$a;headless=$b;originalFailure=$c;scope='real ingress parser; no SDL, game, hooks, lease or device'}
$result|ConvertTo-Json -Depth 4|Set-Content "$Evidence/result.json"
$result|ConvertTo-Json -Depth 4
