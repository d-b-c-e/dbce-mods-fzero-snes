#Requires -Version 7.0
param([Parameter(Mandatory)][string]$Evidence)
$ErrorActionPreference='Stop'
. "$PSScriptRoot/../tools/rig/Ordinary-Contract.ps1"
$Evidence=[IO.Path]::GetFullPath($Evidence);if(Test-Path $Evidence){throw 'Fresh evidence required'};[void][IO.Directory]::CreateDirectory($Evidence)
$checks=0
function Check([bool]$value,[string]$why){$script:checks++;if(-not $value){throw $why}}
function Refused([scriptblock]$action,[string]$why){$failed=$false;try{& $action|Out-Null}catch{$failed=$true};Check $failed $why}
$identity=@{pid=12345;birth=639293045704477514L};$nonce='1234567890abcdef1234567890abcdef';$revision='fixture-revision'
$valid=@{schema='fzero.ordinary-r1@1';nonce=$nonce;pid=$identity.pid;birthFileTime=$identity.birth-504911232000000000L;forceBlocked=$true;syntheticRaw=$true;controllers=0;port=49152;sourcePort=49153;revision=$revision;seq=7;frame=1200;phase='running';packetSha256=('ab'*32)}
Check ((Get-FzeroOrdinaryAdmission $valid $nonce $identity 49152 $revision 6).frame -eq 1200) 'Valid status refused'
foreach($change in @(@('nonce','bad'),@('pid',12346),@('birthFileTime',0),@('forceBlocked',$false),@('syntheticRaw',$false),@('controllers',8192),@('port',8000),@('sourcePort',0),@('revision','other'),@('seq',5),@('phase','revoked'),@('packetSha256','bad'))){
 $copy=$valid.Clone();$copy[$change[0]]=$change[1];Refused {Get-FzeroOrdinaryAdmission $copy $nonce $identity 49152 $revision 6} "Admission accepted bad $($change[0])"
}
$gate="$Evidence/gate.txt";Set-FzeroOrdinaryGate $gate $nonce ([DateTime]::UtcNow.AddSeconds(2).ToFileTimeUtc())
Check ((Read-FzeroSharedText $gate).Contains("nonce=$nonce")) 'Heartbeat nonce missing'
Set-FzeroOrdinaryGate $gate $nonce 0;Check ((Read-FzeroSharedText $gate).Contains('untilFileTime=0')) 'Terminal revoke absent'

# A separate real writer retains the stdout file open without delete sharing;
# also replaces status atomically while this process repeatedly reads both.
$writer=@'
param($root)
$ErrorActionPreference='Stop'
$stream=[IO.FileStream]::new("$root/active.log",[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::ReadWrite)
try{
 for($i=0;$i -lt 150;$i++){
  $bytes=[Text.Encoding]::UTF8.GetBytes("writer=$i`n");$stream.Write($bytes);$stream.Flush()
  [IO.File]::WriteAllText("$root/status.new",('{"seq":'+$i+'}'));[IO.File]::Move("$root/status.new","$root/status.json",$true)
  Start-Sleep -Milliseconds 4
 }
}finally{$stream.Dispose()}
[IO.File]::WriteAllText("$root/writer-done.txt",'done')
'@
[IO.File]::WriteAllText("$Evidence/writer.ps1",$writer)
$start=[Diagnostics.ProcessStartInfo]::new((Get-Process -Id $PID).Path);$start.UseShellExecute=$false;$start.CreateNoWindow=$true
foreach($arg in @('-NoProfile','-NonInteractive','-File',"$Evidence/writer.ps1",$Evidence)){[void]$start.ArgumentList.Add($arg)}
$p=[Diagnostics.Process]::new();$p.StartInfo=$start;[void]$p.Start();$deadline=[DateTime]::UtcNow.AddSeconds(10);$reads=0;$replaceReads=0
try{
 while(-not $p.HasExited){
  if([DateTime]::UtcNow -gt $deadline){throw 'Concurrent writer timeout'}
  if(Test-Path "$Evidence/active.log"){[void](Read-FzeroSharedText "$Evidence/active.log");$reads++}
  if(Test-Path "$Evidence/status.json"){$s=Read-FzeroSharedText "$Evidence/status.json"|ConvertFrom-Json;Check ($s.seq -ge 0 -and $s.seq -lt 150) 'Replaced status malformed';$replaceReads++}
  Start-Sleep -Milliseconds 2
 }
 $p.WaitForExit();Check ($p.ExitCode -eq 0) 'Concurrent writer failed'
}finally{if(-not $p.HasExited){$p.Kill();$p.WaitForExit()};$p.Dispose()}
Check ($reads -gt 20 -and $replaceReads -gt 20) 'Active shared reads not exercised'
Check ((Read-FzeroSharedText "$Evidence/active.log").Contains('writer=149')) 'Writer incomplete'
Check ((Get-Content "$Evidence/writer-done.txt" -Raw) -ceq 'done') 'Writer release incomplete'
$profile=New-FzeroOrdinaryProfile;Check ($profile.Profiles[0].Profile.Bindings.steer.CalCentre -eq 32768) 'Fixture shape malformed'
@{pass=$true;checks=$checks;sharedLogReads=$reads;sharedAtomicStatusReads=$replaceReads;gameStarted=$false;hooksStarted=$false;virtualController=$false;globalInput=$false}|ConvertTo-Json|Set-Content "$Evidence/result.json"
"PASS ordinary parent contract $checks checks; $reads active log/$replaceReads atomic status reads"
