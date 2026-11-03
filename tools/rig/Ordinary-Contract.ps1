# Ordinary diagnostic admission/receiver logic; no native UI calls on import.
function Read-FzeroSharedText([string]$Path){
 $stream=[IO.FileStream]::new($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete)
 try{$reader=[IO.StreamReader]::new($stream);try{$reader.ReadToEnd()}finally{$reader.Dispose()}}finally{$stream.Dispose()}
}
function Get-FzeroOrdinaryAdmission($Status,[string]$Nonce,$Identity,[int]$Port,[string]$Revision,[int]$PreviousSequence=0){
 if($Status.schema -cne 'fzero.ordinary-r1@1' -or $Status.nonce -cne $Nonce -or
    [int]$Status.pid -ne $Identity.pid -or [long]$Status.birthFileTime -ne ([long]$Identity.birth-504911232000000000L) -or
    $Status.forceBlocked -ne $true -or $Status.syntheticRaw -ne $true -or [int]$Status.controllers -ne 0 -or
    [int]$Status.port -ne $Port -or $Port -lt 1024 -or $Port -eq 8000 -or
    $Status.revision -cne $Revision -or [int]$Status.seq -lt $PreviousSequence -or
    $Status.phase -notin @('running','panel','done') -or [int]$Status.frame -lt 0 -or [int]$Status.frame -gt 3302){throw 'Ordinary status identity/fence/sequence refused'}
 if($Status.packetSha256 -cnotmatch '^[a-f0-9]{64}$' -or [int]$Status.sourcePort -lt 1024 -or [int]$Status.sourcePort -gt 65535){throw 'Ordinary actual telemetry witness absent'}
 return $Status
}
function Set-FzeroOrdinaryGate([string]$Path,[string]$Nonce,[long]$Until){
 $temp=$Path+'.new';[IO.File]::WriteAllText($temp,"nonce=$Nonce`nuntilFileTime=$Until`n")
 # Native reader holds the old gate only for one small read. A sharing collision
 # is bounded; never interpret a failed replacement as a refreshed heartbeat.
 $deadline=[Environment]::TickCount64+100
 while($true){try{[IO.File]::Move($temp,$Path,$true);return}catch [IO.IOException], [UnauthorizedAccessException]{if([Environment]::TickCount64 -ge $deadline){throw};Start-Sleep -Milliseconds 2}}
}
function New-FzeroOrdinaryProfile{
 $bindings=[ordered]@{}
 foreach($spec in @(@('steer',0,32768,4096,32768,61440),@('throttle',1,0,0,0,65535),@('brake',2,0,0,0,65535))){
  $bindings[$spec[0]]=[ordered]@{InstanceGuid='11111111-1111-1111-1111-111111111111';ProductGuid='0006346e-0000-0000-0000-504944564944';DeviceName='Synthetic ordinary R1 fixture';Kind='Axis';Index=$spec[1];Direction=1;Rest=$spec[2];Minimum=0;Maximum=65535;Calibrated=$true;CalLow=$spec[3];CalCentre=$spec[4];CalHigh=$spec[5]}
 }
 $names=@('confirm','back','start','select','navUp','navDown','navLeft','navRight')
 for($i=0;$i -lt $names.Count;$i++){$bindings[$names[$i]]=@{InstanceGuid='11111111-1111-1111-1111-111111111111';ProductGuid='0006346e-0000-0000-0000-504944564944';DeviceName='Synthetic ordinary R1 fixture';Kind='Button';Index=$i;Direction=1;Rest=0;Minimum=0;Maximum=65535;Calibrated=$false}}
 @{Schema=2;ActiveId='ordinary-fixture';Profiles=@(@{Id='ordinary-fixture';Profile=@{Schema=1;Name='Synthetic ordinary R1';Transmission='Automatic';Bindings=$bindings}})}
}
function Get-FzeroOrdinaryRows($Rows,$Packets,[int]$SourcePort){
 # Production host batches at most four frames before the outer stop check.
 if($Rows.Count -lt 3300 -or $Rows.Count -gt 3303 -or $Rows[0].frame -ne 0 -or $Rows[-1].frame -ne $Rows.Count-1){throw 'Ordinary frame trace incomplete'}
 $byHash=@{};foreach($packet in $Packets){if($packet.sourcePort -eq $SourcePort -and $packet.address -ceq '127.0.0.1' -and $packet.bytes.Length -eq 324){$byHash[$packet.sha256]=$packet}}
 $previous=-1;$matched=0;$race=0;$moved=$false;$position=$null;$left=0;$right=0;$gas=0;$brake=0;$neutral=0
 foreach($r in $Rows){
  if([int]$r.frame -ne $previous+1 -or $r.canonical.Count -ne 3 -or @($r.canonical|Where-Object {$_ -ne 1}).Count){throw 'Ordinary frame/canonical trace invalid'};$previous=[int]$r.frame
  if(-not $byHash.ContainsKey($r.packetSha256) -or $r.sourcePort -ne $SourcePort){throw "Actual sent packet missing at frame $previous"};$packet=$byHash[$r.packetSha256].bytes;$matched++
  $word=[uint32]$r.word
  if($packet[315] -ne $(if($word -band 1){255}else{0}) -or $packet[316] -ne $(if($word -band 2){255}else{0}) -or $packet[320] -ne $(if($word -band 0x40){129}elseif($word -band 0x80){127}else{0})){throw 'Packet/consumed word mismatch'}
  $isRace=$r.state[0] -eq 2 -and $r.state[1] -ge 3
  if([BitConverter]::ToInt32($packet,0) -ne [int]$isRace){throw 'Packet/guest state mismatch'}
  if([BitConverter]::ToSingle($packet,244) -ne $r.x*0.25 -or [BitConverter]::ToSingle($packet,252) -ne $r.y*0.25){throw 'Packet/coherent guest position mismatch'}
  if($isRace){$race++;$current="$($r.x):$($r.y)";if($position -and $current -cne $position){$moved=$true};$position=$current}
  if($previous -ge 1500 -and $previous -le 1530 -and $isRace -and $r.raw[0] -eq -28672 -and $r.normalized[0] -le -0.99 -and ($word -band 0x40)){$left++}
  if($previous -ge 1800 -and $previous -le 1830 -and $isRace -and $r.raw[0] -eq 28672 -and $r.normalized[0] -ge 0.99 -and ($word -band 0x80)){$right++}
  if($previous -ge 1100 -and $previous -lt 2000 -and $isRace -and $r.normalized[1] -ge 0.99 -and ($word -band 1)){$gas++}
  if($previous -ge 2100 -and $previous -lt 2160 -and $isRace -and $r.normalized[2] -ge 0.99 -and ($word -band 2)){$brake++}
  if($previous -ge 3000 -and ($word -band 0xffff) -eq 0 -and $r.normalized[0] -eq 0 -and $r.normalized[1] -eq 0 -and $r.normalized[2] -eq 0){$neutral++}
 }
 if(-not $moved -or -not $left -or -not $right -or -not $gas -or -not $brake -or $neutral -ne $Rows.Count-3000){throw "Ordinary response incomplete: race=$race moved=$moved left=$left right=$right gas=$gas brake=$brake neutral=$neutral"}
 @{frames=$Rows.Count;matchedPackets=$matched;raceFrames=$race;positionChanged=$moved;leftFrames=$left;rightFrames=$right;gasFrames=$gas;brakeFrames=$brake;neutralFrames=$neutral;physicalWheel=$false}
}
