#Requires -Version 7.0
<# Private diagnostic copy/supervisor. Prepare and Check never launch a game.
   Run holds the shared lease, sends no navigation, and automatically ends the
   first pilot at 75 s. Commands require a recent owned-window capture receipt.
   Unresolved child exit retains the lease; never kills or adopts a replacement. #>
[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Prepare','Check','Run','Status','Capture','Command')][string]$Action,
    [Parameter(Mandatory)][string]$Probe,
    [string]$Install = 'E:/Source/toolkits/launchbox/Launchbox-Racing/Games/Windows/F-Zero (SNES Recomp)',
    [string]$Build = (Join-Path $PSScriptRoot '../build-shim-game'),
    [string]$Toolkit = 'E:/Source/toolkits/dbce-wheel-mod-toolkit',
    [string]$Hub = 'E:/Source/dbce-project-mgmt',
    [string]$Finalizer = 'E:/Source/dbce-project-mgmt/artifacts/shim-abi2-review-a724dab-finalizer/bin/P21Finalizer/release',
    [string]$JournalReader = 'E:/Source/research/.worktrees/shim-device-path/windows/tests/control_journal/read_journal.py',
    [string]$DeluxeData = 'E:/Source/games/dbce-mods-fzero-snes/captures/bs-deluxe/mods/bs-deluxe.dat',
    [string[]]$Commands,
    [string]$InspectedFrame,
    [long]$WindowHandle = 0
)
$ErrorActionPreference = 'Stop'
$Probe = [IO.Path]::GetFullPath($Probe)
if ($Probe -notmatch '^[A-Za-z]:[\\/]fz-probe-[a-zA-Z0-9-]+$') { throw 'Use a fresh short drive-root fz-probe-<id> folder' }
$utf8 = [Text.UTF8Encoding]::new($false)
function Write-Json($Path,$Object) { [IO.File]::WriteAllText($Path,($Object | ConvertTo-Json -Depth 16),$utf8) }
function Hash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Assert-NoLink([string]$Path) {
    $cursor = [IO.Path]::GetFullPath($Path)
    while ($cursor) {
        if ((Test-Path -LiteralPath $cursor) -and ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Link refused: $cursor" }
        $cursor = Split-Path $cursor -Parent
    }
}
function Inventory([string]$Root,[string[]]$Names) {
    $rows = @()
    foreach ($name in $Names) {
        $target = Join-Path $Root $name
        if (!(Test-Path -LiteralPath $target)) { $rows += @{path=$name;sha256=$null}; continue }
        Assert-NoLink $target
        $item = Get-Item -LiteralPath $target -Force
        $items = if ($item.PSIsContainer) { @(Get-ChildItem -LiteralPath $target -Recurse -Force) } else { @($item) }
        foreach ($file in $items) {
            if ($file.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Link refused: $($file.FullName)" }
            if (!$file.PSIsContainer) { $rows += @{path=[IO.Path]::GetRelativePath($Root,$file.FullName).Replace('\','/');sha256=(Hash $file.FullName)} }
        }
    }
    return @($rows | Sort-Object { $_.path })
}
function Check-Rows($Root,$Rows) {
    foreach ($row in $Rows) {
        $path = Join-Path $Root $row.path
        Assert-NoLink $path
        if ($null -eq $row.sha256) { if (Test-Path -LiteralPath $path) { throw "Previously absent file appeared: $path" } }
        elseif (!(Test-Path -LiteralPath $path -PathType Leaf) -or (Hash $path) -cne $row.sha256) { throw "Hash mismatch: $path" }
    }
}
function Check-Inventory($Root,$Rows) {
    Check-Rows $Root $Rows
    $now = Inventory $Root @('.')
    if (($now.path -join "`n") -cne ($Rows.path -join "`n")) { throw "Copied file inventory changed: $Root" }
}
function Check-Owner($Manifest) {
    Check-Rows $Manifest.install $Manifest.ownerFiles
    $now = Inventory $Manifest.install $Manifest.ownerNames
    if (($now.path -join "`n") -cne ($Manifest.ownerFiles.path -join "`n")) { throw 'Owner file inventory changed' }
}
function Copy-Tree([string]$Source,[string]$Destination) {
    Assert-NoLink $Source
    foreach ($entry in Get-ChildItem -LiteralPath $Source -Recurse -Force) { if($entry.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Link refused: $($entry.FullName)"} }
    Copy-Item -LiteralPath $Source -Destination $Destination -Recurse
}
function Read-Json($Path) { Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json }
function New-NeutralBatch([string]$Nonce) {
    # Core refuses an existing empty file. A released Start is a real, bounded
    # zero command through the same mapper; it cannot press Start or navigate.
    return "nonce=$Nonce`nbatch=1`ninject action start 0 ms=50`n"
}
function Lease-Script { Join-Path $Toolkit 'tools/playback/Stage-RigLease.ps1' }
function Assert-Child($Identity) {
    $child = Get-Process -Id $Identity.pid -ErrorAction Stop
    if ($child.HasExited -or $child.StartTime.ToUniversalTime().ToFileTimeUtc() -ne $Identity.creationTime -or
        ![StringComparer]::OrdinalIgnoreCase.Equals($child.MainModule.FileName,[IO.Path]::GetFullPath($Identity.executable))) { throw 'Exact diagnostic child is not alive; no command or close sent' }
    return $child
}
function Check-Lease {
    . (Lease-Script)
    $lease = Read-Json (Join-Path $Probe 'lease.json')
    Assert-StageRigLease -Path $lease.path -Token $lease.token
}
function Invoke-BoundedHelper([string]$Name,[string]$Executable,[string[]]$Arguments,[int]$Seconds) {
    if ($Seconds -lt 1) { throw 'External 220-second deadline reached; no helper started' }
    $info=[Diagnostics.ProcessStartInfo]::new($Executable)
    $info.UseShellExecute=$false; $info.CreateNoWindow=$true
    $info.RedirectStandardOutput=$true; $info.RedirectStandardError=$true
    foreach($argument in $Arguments){$info.ArgumentList.Add($argument)}
    $helper=[Diagnostics.Process]::Start($info)
    try {
        $helperIdentity=@{pid=$helper.Id;creationTime=$helper.StartTime.ToUniversalTime().ToFileTimeUtc();executable=$helper.MainModule.FileName}
        Write-Json (Join-Path $Probe ($Name+'-process.json')) $helperIdentity
        $output=$helper.StandardOutput.ReadToEndAsync(); $errors=$helper.StandardError.ReadToEndAsync()
        if(!$helper.WaitForExit($Seconds*1000)) {
            $script:retain=$true
            throw "Helper $Name exceeded its deadline; process identity recorded, lease retained, no success claimed"
        }
        $stdout=$output.GetAwaiter().GetResult(); $stderr=$errors.GetAwaiter().GetResult()
        [IO.File]::WriteAllText((Join-Path $Probe ($Name+'.log')),$stdout+$stderr,$utf8)
        if($helper.ExitCode -ne 0){throw "Helper $Name exit $($helper.ExitCode); see $Name.log"}
        return $stdout
    } finally { $helper.Dispose() }
}
function Publish-Batch([string[]]$Lines,[string]$Frame,[switch]$DeadlineEnd) {
    Check-Lease
    $identity = Read-Json (Join-Path $Probe 'process.json')
    $owned = Assert-Child $identity
    $owned.Dispose()
    $control = Join-Path $Probe 'control'
    $mutex = [IO.File]::Open((Join-Path $control 'publish.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
    try {
        $ending = $Lines.Count -eq 1 -and $Lines[0] -ceq 'end'
        if (!$ending) {
            if (!$Frame) { throw 'Inspect an owned-window Capture first and supply -InspectedFrame <png>' }
            $framePath = [IO.Path]::GetFullPath($Frame)
            if (![IO.Path]::GetDirectoryName($framePath).Equals((Join-Path $Probe 'frames'),[StringComparison]::OrdinalIgnoreCase)) { throw 'Frame is outside this probe' }
            $receipt = Read-Json ($framePath+'.json')
            if ($receipt.pid -ne $identity.pid -or $receipt.creationTime -ne $identity.creationTime -or
                (Hash $framePath) -cne $receipt.sha256 -or [DateTimeOffset]::UtcNow.Subtract([DateTimeOffset]$receipt.capturedUtc).TotalSeconds -gt 30) { throw 'Frame is stale or belongs to another child' }
            if (!$Lines -or $Lines.Count -gt 8) { throw 'Use 1..8 simultaneous action commands' }
            $profile = [IO.File]::ReadAllText((Join-Path $Probe 'app/config.ini'))
            foreach ($line in $Lines) {
                if ($line -notmatch '^inject action (steer|throttle|brake|confirm|back|start|select|navUp|navDown|navLeft|navRight) (-?[0-9]+(?:\.[0-9]+)?) ms=([0-9]+)$') { throw "Unsupported command: $line" }
                $name=$Matches[1]; $value=[double]::Parse($Matches[2],[Globalization.CultureInfo]::InvariantCulture); $ms=[int]$Matches[3]
                if (!$profile.Contains($name+' =') -or ![double]::IsFinite($value) -or $value -gt 1 -or $value -lt $(if($name -eq 'steer'){-1}else{0}) -or $ms -lt 50 -or $ms -gt 3000) { throw 'Unbound action or out-of-range command' }
            }
        }
        $last = [int]([IO.File]::ReadAllText((Join-Path $control 'batch-sequence.txt')))
        $next = $last + 1
        $nonce = [IO.File]::ReadAllText((Join-Path $control 'nonce.txt')).Trim()
        $content = "nonce=$nonce`nbatch=$next`n" + ($Lines -join "`n") + "`n"
        $temp = Join-Path $control ('publish-'+[guid]::NewGuid().ToString('N')+'.tmp')
        [IO.File]::WriteAllText($temp,$content,$utf8)
        [IO.File]::Move($temp,(Join-Path $control 'inject.txt'),$true)
        [IO.File]::WriteAllText((Join-Path $control 'batch-sequence.txt'),[string]$next,$utf8)
        $command = @{batch=$next;utc=[DateTimeOffset]::UtcNow.ToString('O');commands=$Lines;inspectedFrame=$Frame;deadlineEnd=[bool]$DeadlineEnd}
        Write-Json (Join-Path $control ("batch-{0:D3}.json" -f $next)) $command
        return $command
    } finally { $mutex.Dispose() }
}
Assert-NoLink $Probe
if ($Action -eq 'Prepare') {
    if (Test-Path -LiteralPath $Probe) { throw 'Probe already exists; prepare a fresh copy' }
    $Install=[IO.Path]::GetFullPath($Install); $Build=[IO.Path]::GetFullPath($Build)
    Assert-NoLink $Install; Assert-NoLink $Build
    $buildReceipt=Read-Json (Join-Path $Build 'fzero-shim-build.json')
    if ($buildReceipt.sourceRevision -cne '4f43f1a7a2224d06c4d3020a624f371dd89d2579' -or $buildReceipt.dirty -ne $false) { throw 'Expected reviewed 4f43f1a clean build receipt' }
    $pins=@{
        'FZeroSNESRecomp.exe'='0a37843731a672e305040ef673fc4c7618ce81f61637017ba54b55f840b2cef5'
        'FZeroSNESRecompHeadless.exe'='e55c48f39cc43fb5c9d7a832698d97a67a24e4ad41f7a8a5d740426574e01103'
        'dbce_shim.dll'='9b8a036f664cf8f45cccf82b98e5e4376f93215b60eac50b6db965bf498015d8'
        'WheelFfb.dll'='3f6cc51444879d01feb900098b2de94242594d093f523e3c34bd511d69c0d946'
    }
    foreach($name in $pins.Keys) { if ((Hash (Join-Path $Build $name)) -cne $pins[$name] -or $buildReceipt.files.$name -cne $pins[$name]) { throw "Reviewed candidate mismatch: $name" } }
    Assert-NoLink $DeluxeData
    if ((Hash $DeluxeData) -cne 'a9e31fd3a32103cc6591e70c31e4a182b631989b7ab31b21cb5aec6c5c1bc02b') { throw 'Reviewed private Deluxe data mismatch' }
    if ((Hash (Join-Path $Install 'fzero.sfc')) -cne 'bf16c3c867c58e2ab061c70de9295b6930d63f29f81cc986f5ecae03e0ad18d2') { throw 'Reviewed stock ROM mismatch' }
    $copyNames=@('config.ini','fzero-video.ini','keybinds.ini','fzero.sfc','SDL3.dll','libiconv-2.dll','assets','shaders-custom','music')
    $ownerNames=$copyNames+@('mods/bs-deluxe.dat','saves','FZeroSNESRecomp-wheel-launcher.exe','FZeroSNESRecomp.exe','WheelFfb.dll','rom.cfg','install-receipt.json')
    $ownerFiles=Inventory $Install $ownerNames
    foreach($name in $copyNames) { if (!(Test-Path -LiteralPath (Join-Path $Install $name))) { throw "Missing copy input: $name" } }
    if ([IO.File]::ReadAllText((Join-Path $Install 'config.ini')) -notmatch '(?m)^\[Controls\]\s*$') { throw 'Owner profile has no Controls section; do not synthesize one' }
    [IO.Directory]::CreateDirectory((Join-Path $Probe 'app')) | Out-Null
    foreach($name in $copyNames) {
        $from=Join-Path $Install $name; $dest=Join-Path $Probe ('app/'+$name)
        [IO.Directory]::CreateDirectory((Split-Path $dest -Parent)) | Out-Null
        if ((Get-Item -LiteralPath $from).PSIsContainer) { Copy-Tree $from $dest } else { Copy-Item -LiteralPath $from -Destination $dest }
    }
    foreach($name in $pins.Keys) { Copy-Item -LiteralPath (Join-Path $Build $name) -Destination (Join-Path $Probe 'app') }
    Assert-NoLink $DeluxeData
    [IO.Directory]::CreateDirectory((Join-Path $Probe 'app/mods')) | Out-Null
    Copy-Item -LiteralPath $DeluxeData -Destination (Join-Path $Probe 'app/mods/bs-deluxe.dat')
    [IO.Directory]::CreateDirectory((Join-Path $Probe 'tools')) | Out-Null
    Copy-Tree $Finalizer (Join-Path $Probe 'tools/finalizer')
    Copy-Tree (Join-Path $Toolkit 'tools/window-capture/bin/Release/net8.0-windows10.0.22621.0') (Join-Path $Probe 'tools/wgc')
    Copy-Item -LiteralPath $JournalReader -Destination (Join-Path $Probe 'tools/read_journal.py')
    Copy-Item -LiteralPath (Join-Path $Build 'fzero-capture-build.json') -Destination $Probe
    Copy-Item -LiteralPath (Join-Path $Build 'fzero-shim-build.json') -Destination $Probe
    foreach($dir in @('control','frames')) { [IO.Directory]::CreateDirectory((Join-Path $Probe $dir)) | Out-Null }
    $manifest=@{schema='fzero.private-shim-copy';version=1;preparedUtc=[DateTimeOffset]::UtcNow.ToString('O');install=$Install;ownerNames=$ownerNames;ownerFiles=$ownerFiles;
        candidateSource=$buildReceipt.sourceRevision;deluxeDataSource=$DeluxeData;deluxeDataSha256=(Hash $DeluxeData);toolkit=$Toolkit;hub=$Hub;appFiles=(Inventory (Join-Path $Probe 'app') @('.'));toolFiles=(Inventory (Join-Path $Probe 'tools') @('.'));runtimeLaunched=$false}
    Check-Owner $manifest
    Write-Json (Join-Path $Probe 'prepared.json') $manifest
    "Prepared only: $Probe"; return
}
$manifest=Read-Json (Join-Path $Probe 'prepared.json')
$Toolkit=$manifest.toolkit; $Hub=$manifest.hub
if ($Action -eq 'Check' -or $Action -eq 'Run') {
    Check-Owner $manifest
    Check-Inventory (Join-Path $Probe 'app') $manifest.appFiles
    Check-Inventory (Join-Path $Probe 'tools') $manifest.toolFiles
    if ($Action -eq 'Check') { "PASS copied runtime/tools and owner pins; no launch: $Probe"; return }
}
if ($Action -eq 'Status') {
    foreach($name in @('process.json','supervisor.json','result.json')) { if(Test-Path -LiteralPath (Join-Path $Probe $name)){Get-Content -LiteralPath (Join-Path $Probe $name) -Raw} }
    $journal=Join-Path $Probe 'run/controls-journal.jsonl'
    if(Test-Path -LiteralPath $journal){Get-Content -LiteralPath $journal -Tail 8}
    return
}
if ($Action -eq 'Command') { Publish-Batch $Commands $InspectedFrame | ConvertTo-Json; return }
if ($Action -eq 'Capture') {
    Check-Lease
    $identity=Read-Json (Join-Path $Probe 'process.json'); $child=Assert-Child $identity
    $child.Dispose()
    $wgc=Join-Path $Probe 'tools/wgc/dbce-window-capture.exe'
    $listing=& $wgc $identity.pid --list
    if($LASTEXITCODE -ne 0){throw 'Unable to list owned game windows'}
    $listing | Set-Content -LiteralPath (Join-Path $Probe 'frames/windows.json')
    if(!$WindowHandle){$listing; 'Use -WindowHandle from this list to capture the centre game window';return}
    $out=Join-Path $Probe ('frames/'+[DateTime]::UtcNow.ToString('HHmmssfff')+'.png')
    & $wgc $identity.pid $out --hwnd $WindowHandle
    if($LASTEXITCODE -ne 0){throw 'WGC failed'}
    $child=Assert-Child $identity; $child.Dispose()
    Write-Json ($out+'.json') @{pid=$identity.pid;creationTime=$identity.creationTime;capturedUtc=[DateTimeOffset]::UtcNow.ToString('O');sha256=(Hash $out);windowHandle=$WindowHandle}
    $out;return
}
if ($Action -ne 'Run') {throw 'Unexpected action'}
if (Test-Path -LiteralPath (Join-Path $Probe 'run')) {throw 'Never reuse a run; prepare a fresh probe'}
. (Lease-Script)
$idle=& (Join-Path $Hub 'tools/Owner-Input.ps1') -IdleSeconds
if($idle -lt 300){throw "Owner input $idle seconds ago; no launch"}
$lease=Enter-StageRigLease -Path (Join-Path $env:LOCALAPPDATA 'dbce/test-slot.txt') -Owner 'astra-fzero' -Purpose 'private muted 90-second scene-inspected shim pilot'
$child=$null; $identity=$null; $retain=$false; $outTask=$null; $errTask=$null
$result=@{schema='fzero.private-shim-result';version=1;errors=@();forced=$false;sealed=$false;exitObserved=$false;runtimeQualified=$false}
try {
    Write-Json (Join-Path $Probe 'lease.json') $lease
    Check-Owner $manifest
    $nonce=[guid]::NewGuid().ToString('N'); $control=Join-Path $Probe 'control'
    [IO.File]::WriteAllText((Join-Path $control 'nonce.txt'),$nonce,$utf8)
    [IO.File]::WriteAllText((Join-Path $control 'batch-sequence.txt'),'1',$utf8)
    [IO.File]::WriteAllText((Join-Path $control 'inject.on'),"nonce=$nonce`nexpires=$([DateTimeOffset]::UtcNow.ToUnixTimeSeconds()+210)`n",$utf8)
    [IO.File]::WriteAllText((Join-Path $control 'inject.txt'),(New-NeutralBatch $nonce),$utf8)
    Write-Json (Join-Path $control 'batch-001.json') @{batch=1;utc=[DateTimeOffset]::UtcNow.ToString('O');commands=@('inject action start 0 ms=50');purpose='neutral startup; no navigation';inspectedFrame=$null}
    $info=[Diagnostics.ProcessStartInfo]::new([IO.Path]::GetFullPath((Join-Path $Probe 'app/FZeroSNESRecomp.exe')))
    $info.WorkingDirectory=Join-Path $Probe 'app'; $info.UseShellExecute=$false
    $info.ArgumentList.Add((Join-Path $Probe 'app/fzero.sfc'))
    $info.RedirectStandardOutput=$true; $info.RedirectStandardError=$true
    foreach($key in @($info.Environment.Keys)){if($key -match '^(FZERO_|SNESRECOMP_)'){$info.Environment.Remove($key) | Out-Null}}
    $info.Environment['FZERO_SHIM_DIAGNOSTIC_RUN']=Join-Path $Probe 'run'
    $info.Environment['FZERO_SHIM_SESSION_NONCE']=$nonce
    $info.Environment['FZERO_SHIM_SESSION_FILE']=Join-Path $control 'inject.on'
    $info.Environment['FZERO_SHIM_BATCH_FILE']=Join-Path $control 'inject.txt'
    $info.Environment['FZERO_DELUXE_DATA']=Join-Path $Probe 'app/mods/bs-deluxe.dat'
    $child=[Diagnostics.Process]::Start($info)
    $null=$child.Handle # retain this process object/handle through exit and seal.
    $identity=@{pid=$child.Id;creationTime=$child.StartTime.ToUniversalTime().ToFileTimeUtc();executable=$info.FileName;sha256=(Hash $info.FileName)}
    Write-Json (Join-Path $Probe 'process.json') $identity
    $outTask=$child.StandardOutput.ReadToEndAsync();$errTask=$child.StandardError.ReadToEndAsync()
    $timer=[Diagnostics.Stopwatch]::StartNew();$ended=$false;$closed=$false
    while(!$child.WaitForExit(250)) {
        $elapsed=$timer.Elapsed.TotalSeconds
        Assert-StageRigLease -Path $lease.path -Token $lease.token
        Write-Json (Join-Path $Probe 'supervisor.json') @{pid=$identity.pid;seconds=$elapsed;pilotEndAt=75;normalCloseAt=90;hardDeadline=220;ended=$ended;closed=$closed}
        if($elapsed -ge 75 -and !$ended){Publish-Batch @('end') '' -DeadlineEnd | Out-Null;$ended=$true}
        if($elapsed -ge 90 -and !$closed){$owned=Assert-Child $identity;$null=$owned.CloseMainWindow();$owned.Dispose();$closed=$true}
        if(($closed -and $elapsed -ge 100) -or $elapsed -ge 220){$retain=$true;throw 'Exact child did not exit normally; lease retained for recovery, no kill'}
    }
    $result.exitObserved=$true;$result.exitCode=$child.ExitCode
    [IO.File]::WriteAllText((Join-Path $Probe 'stdout.log'),$outTask.GetAwaiter().GetResult(),$utf8)
    [IO.File]::WriteAllText((Join-Path $Probe 'stderr.log'),$errTask.GetAwaiter().GetResult(),$utf8)
    if($child.ExitCode -ne 0){throw "Game exit $($child.ExitCode); original not qualified"}
    $hostReport=Read-Json (Join-Path $Probe 'run/host-shutdown.json')
    if($hostReport.state -cne 'resolved'){throw 'Shared host did not resolve'}
    $journalText=Invoke-BoundedHelper 'journal' 'py' @('-3',(Join-Path $Probe 'tools/read_journal.py'),(Join-Path $Probe 'run/controls-journal.jsonl')) ([Math]::Min(20,[Math]::Floor(220-$timer.Elapsed.TotalSeconds)))
    [IO.File]::WriteAllText((Join-Path $Probe 'journal-result.json'),$journalText,$utf8)
    $journal=$journalText | ConvertFrom-Json
    if(!$journal.qualifies -or $journal.terminalDetail -notmatch '^completed:'){throw 'No explicit completed session'}
    Check-Owner $manifest
    $cap=Join-Path $Probe 'run/capture'
    Copy-Item -LiteralPath (Join-Path $Probe 'run/host-shutdown.json') -Destination (Join-Path $cap 'host-shutdown.json')
    Write-Json (Join-Path $cap 'exit.json') @{pid=$identity.pid;creationTime=$identity.creationTime;exitCode=$child.ExitCode;forced=$false}
    Invoke-BoundedHelper 'seal' 'dotnet' @((Join-Path $Probe 'tools/finalizer/P21Finalizer.dll'),'--seal-export',$cap,(Join-Path $Probe 'validated.tsv')) ([Math]::Min(60,[Math]::Floor(220-$timer.Elapsed.TotalSeconds))) | Out-Null
    $result.sealed=$true;$result.runtimeQualified=$false # moving route and cold WRAM comparison are separate proofs.
} catch {$result.errors+= $_.Exception.Message}
finally {
  try {
    if($child -and !$child.HasExited -and !$retain){
        try{$owned=Assert-Child $identity;$null=$owned.CloseMainWindow();$owned.Dispose();if(!$child.WaitForExit(10000)){$retain=$true}}catch{$retain=$true;$result.errors+=$_.Exception.Message}
    }
    if($child -and !$child.HasExited){$retain=$true}
    if($child -and $child.HasExited){
        $result.exitObserved=$true;$result.exitCode=$child.ExitCode
        if($outTask){[IO.File]::WriteAllText((Join-Path $Probe 'stdout.log'),$outTask.GetAwaiter().GetResult(),$utf8)}
        if($errTask){[IO.File]::WriteAllText((Join-Path $Probe 'stderr.log'),$errTask.GetAwaiter().GetResult(),$utf8)}
    }
    try{Check-Owner $manifest;$result.ownerPinsUnchanged=$true}catch{$result.ownerPinsUnchanged=$false;$result.errors+=$_.Exception.Message}
  } finally {
    if($child -and !$child.HasExited){$retain=$true}
    $result.leaseRetained=$retain
    try { Write-Json (Join-Path $Probe 'result.json') $result }
    finally {
      if(!$retain){$released=Exit-StageRigLease -Lease $lease;if(!$released){throw 'Lease no longer ours; retained existing file'}}
      if($child){$child.Dispose()}
    }
  }
}
$result | ConvertTo-Json -Depth 8
if($result.errors.Count -or !$result.sealed){exit 1}
