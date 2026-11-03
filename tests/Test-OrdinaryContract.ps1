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
  [IO.File]::WriteAllText("$root/status.new",('{"seq":'+$i+'}'))
  $until=[Environment]::TickCount64+1000
  while($true){try{[IO.File]::Move("$root/status.new","$root/status.json",$true);break}catch [IO.IOException], [UnauthorizedAccessException]{if([Environment]::TickCount64 -ge $until){throw};Start-Sleep -Milliseconds 2}}
  Start-Sleep -Milliseconds 4
 }
}finally{$stream.Dispose()}
[IO.File]::WriteAllText("$root/writer-done.txt",'done')
'@
[IO.File]::WriteAllText("$Evidence/writer.ps1",$writer)
$start=[Diagnostics.ProcessStartInfo]::new((Get-Process -Id $PID).Path);$start.UseShellExecute=$false;$start.CreateNoWindow=$true
$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
foreach($arg in @('-NoProfile','-NonInteractive','-File',"$Evidence/writer.ps1",$Evidence)){[void]$start.ArgumentList.Add($arg)}
$p=[Diagnostics.Process]::new();$p.StartInfo=$start;[void]$p.Start();$stdout=$p.StandardOutput.ReadToEndAsync();$stderr=$p.StandardError.ReadToEndAsync();$deadline=[DateTime]::UtcNow.AddSeconds(10);$reads=0;$replaceReads=0
$writerIdentity=@{pid=$p.Id;birth=$p.StartTime.ToUniversalTime().Ticks}
Refused {Get-FzeroOrdinaryQuiescence $writerIdentity} 'Live exact writer mistaken for quiescent'
try{
 while(-not $p.HasExited){
  if([DateTime]::UtcNow -gt $deadline){throw 'Concurrent writer timeout'}
  if(Test-Path "$Evidence/active.log"){[void](Read-FzeroSharedText "$Evidence/active.log");$reads++}
  if(Test-Path "$Evidence/status.json"){$s=Read-FzeroSharedText "$Evidence/status.json"|ConvertFrom-Json;Check ($s.seq -ge 0 -and $s.seq -lt 150) 'Replaced status malformed';$replaceReads++}
  Start-Sleep -Milliseconds 2
 }
 $p.WaitForExit();[IO.File]::WriteAllText("$Evidence/writer.stdout.txt",$stdout.GetAwaiter().GetResult());[IO.File]::WriteAllText("$Evidence/writer.stderr.txt",$stderr.GetAwaiter().GetResult());Check ($p.ExitCode -eq 0) 'Concurrent writer failed'
 Check ((Get-FzeroOrdinaryQuiescence $writerIdentity).originalAbsent -eq $true) 'Exited held-handle writer mistaken for live'
}finally{if(-not $p.HasExited){$p.Kill();$p.WaitForExit()};$p.Dispose()}
Check ($reads -gt 20 -and $replaceReads -gt 20) 'Active shared reads not exercised'
Check ((Read-FzeroSharedText "$Evidence/active.log").Contains('writer=149')) 'Writer incomplete'
Check ((Get-Content "$Evidence/writer-done.txt" -Raw) -ceq 'done') 'Writer release incomplete'
$profile=New-FzeroOrdinaryProfile;Check ($profile.Profiles[0].Profile.Bindings.steer.CalCentre -eq 32768) 'Fixture shape malformed'
& {
 $rows=[Collections.Generic.List[object]]::new();$packets=[Collections.Generic.List[object]]::new()
 for($frame=0;$frame -lt 3300;$frame++){
  $left=$frame -ge 1500 -and $frame -le 1530;$right=$frame -ge 1800 -and $frame -le 1830
  $gas=($frame -ge 1100 -and $frame -lt 2000) -or ($frame -ge 2200 -and $frame -lt 3000);$brake=$frame -ge 2100 -and $frame -lt 2160
  $word=1073741824L;if($left){$word=$word -bor 0x40};if($right){$word=$word -bor 0x80};if($gas){$word=$word -bor 1};if($brake){$word=$word -bor 2}
  $x=$frame;$y=100;$race=$frame -ge 900;$bytes=[byte[]]::new(324)
  [BitConverter]::GetBytes([int]$race).CopyTo($bytes,0);[BitConverter]::GetBytes([int]($frame*17)).CopyTo($bytes,4)
  [BitConverter]::GetBytes([single]($x*0.25)).CopyTo($bytes,244);[BitConverter]::GetBytes([single]($y*0.25)).CopyTo($bytes,252)
  $bytes[315]=if($gas){255}else{0};$bytes[316]=if($brake){255}else{0};$bytes[320]=if($left){129}elseif($right){127}else{0}
  $hash=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
  $packets.Add(@{address='127.0.0.1';sourcePort=49153;bytes=$bytes;sha256=$hash})
  $rows.Add(@{frame=$frame;canonical=@(1,1,1);state=@($(if($race){2}else{0}),3,0);word=$word;x=$x;y=$y;yaw=0;sourcePort=49153;packetSha256=$hash;raw=@($(if($left){-28672}elseif($right){28672}else{0}),$(if($gas){32767}else{-32768}),$(if($brake){32767}else{-32768}));normalized=@($(if($left){-1}elseif($right){1}else{0}),[int]$gas,[int]$brake)})
 }
 $result=Get-FzeroOrdinaryRows $rows $packets 49153;Check ($result.frames -eq 3300 -and $result.matchedPackets -eq 3300) 'Coherent receiver trace refused'
 $rows[1500].canonical[0]=0;Refused {Get-FzeroOrdinaryRows $rows $packets 49153} 'Unqualified calibration accepted';$rows[1500].canonical[0]=1
 $packets[1500].bytes[315]=0;Refused {Get-FzeroOrdinaryRows $rows $packets 49153} 'Mismatched transport input accepted';$packets[1500].bytes[315]=255
 $rows[1500].x++;Refused {Get-FzeroOrdinaryRows $rows $packets 49153} 'Incoherent guest position accepted';$rows[1500].x--
 Refused {Get-FzeroOrdinaryRows $rows $packets 49154} 'Foreign source-port accepted'
 Refused {Get-FzeroOrdinaryRows @($rows.ToArray()[1..3299]) $packets 49153} 'Incomplete frame trace accepted'
}
@{pass=$true;checks=$checks;sharedLogReads=$reads;sharedAtomicStatusReads=$replaceReads;gameStarted=$false;hooksStarted=$false;virtualController=$false;globalInput=$false}|ConvertTo-Json|Set-Content "$Evidence/result.json"
"PASS ordinary parent contract $checks checks; $reads active log/$replaceReads atomic status reads"
