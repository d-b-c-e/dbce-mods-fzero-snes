<#
.SYNOPSIS
Record or replay from boot using the existing .fzpt CLI. Never edits configuration.
.EXAMPLE
./tools/Recording.ps1 Record -Rom ./fzero.sfc -Tape ./recordings/my-drive.fzpt
.EXAMPLE
./tools/Recording.ps1 Replay -Rom ./fzero.sfc -Tape ./recordings/my-drive.fzpt
#>
[CmdletBinding()]
param(
    [Parameter(Position=0)][ValidateSet('Record','Replay')][string]$Action,
    [string]$Rom,
    [string]$Tape,
    [switch]$Record, [switch]$Playback, [switch]$Stop,
    [string]$GameDir, [string]$Recording, [string]$Result,
    [ValidatePattern('^[0-9a-f]{32}$')][string]$RequestId,
    [ValidateSet('human','automated')][string]$InputOrigin = 'human',
    [ValidateRange(10,1800)][int]$Seconds = 60,
    [string]$RecordingSha256
)
$ErrorActionPreference = 'Stop'

# Installed ordinary companion. This metadata wraps the existing .fzpt/state
# pair; replay validation and every WRAM comparison remain inside the game.
function SessionHash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function SessionJson([string]$Path) {
    if (!(Test-Path -LiteralPath $Path -PathType Leaf) -or (Get-Item -LiteralPath $Path).Length -gt 65536) { throw "Missing or oversized session metadata: $Path" }
    $text = Get-Content -LiteralPath $Path -Raw
    $document = [Text.Json.JsonDocument]::Parse($text)
    try { SessionUnique $document.RootElement } finally { $document.Dispose() }
    $text | ConvertFrom-Json -AsHashtable
}
function SessionUnique($Element) {
    if ($Element.ValueKind -eq [Text.Json.JsonValueKind]::Object) {
        $names = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
        foreach ($property in $Element.EnumerateObject()) {
            if (!$names.Add($property.Name)) { throw 'Duplicate session JSON field.' }
            SessionUnique $property.Value
        }
    } elseif ($Element.ValueKind -eq [Text.Json.JsonValueKind]::Array) {
        foreach ($item in $Element.EnumerateArray()) { SessionUnique $item }
    }
}
function SessionWrite([string]$Path, $Value) {
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($Value | ConvertTo-Json -Depth 12) + "`n")
    $file = [IO.File]::Open($Path + '.tmp', [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try { $file.Write($bytes); $file.Flush($true) } finally { $file.Dispose() }
    [IO.File]::Move($Path + '.tmp', $Path, $false)
}
function SessionBound([string]$Root, [string]$Relative) {
    if ([IO.Path]::IsPathRooted($Relative) -or $Relative.Contains(':') -or
        @($Relative.Replace('\','/').Split('/') | Where-Object { $_ -in @('','..','.') }).Count) { throw 'Invalid session member path.' }
    $path = [IO.Path]::GetFullPath((Join-Path $Root $Relative))
    $current = $path
    while ($current) {
        if ((Test-Path -LiteralPath $current) -and ((Get-Item -Force -LiteralPath $current).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Session paths cannot traverse links: $current" }
        $current = [IO.Path]::GetDirectoryName($current)
    }
    $path
}
function SessionCapability([string]$Root) {
    $r = SessionJson (SessionBound $Root 'install-receipt.json')
    if ($r.schemaVersion -ne 1 -or $r.launcher -cne 'FZeroSNESRecomp-wheel-launcher.exe' -or
        $r.sourceCommit -cnotmatch '^[0-9a-f]{40}$' -or $r.session.schema -cne 'fzero.session-capability@1' -or
        $r.session.protocol -cne 'fzero.session-control@1' -or $r.session.record -ne $true -or
        $r.session.replay -ne $true -or $r.session.stopShortcut -cne 'Ctrl+F11' -or
        $r.session.companion -cne 'tools/Recording.ps1') { throw 'This installed copy has no ordinary F-Zero session capability.' }
    foreach ($name in @($r.launcher, 'WheelFfb.dll', 'tools/Recording.ps1')) {
        $entries = @($r.files | Where-Object { $_.name -ceq $name })
        if ($entries.Count -ne 1 -or $entries[0].sha256 -cnotmatch '^[0-9A-Fa-f]{64}$' -or
            (SessionHash (SessionBound $Root $name)) -ine $entries[0].sha256) { throw "Installed session ownership differs: $name" }
    }
    if ((SessionHash (Join-Path $Root $r.launcher)) -ine $r.launcherSha256 -or
        (SessionHash $PSCommandPath) -ine (SessionHash (Join-Path $Root 'tools/Recording.ps1'))) { throw 'Companion or installed executable differs from its receipt.' }
    $r
}
function SessionIni([string]$Path, [string]$WantedSection, [string]$Key, [string]$Default) {
    $section = ''; $found = $false; $value = $Default
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { return $value }
    foreach ($line in [IO.File]::ReadLines($Path)) {
        $text = $line.Trim()
        if ($text -match '^\[([^]]+)\]$') { $section = $Matches[1]; continue }
        if ($section -ieq $WantedSection -and $text -match '^([^=]+)=(.*)$' -and $Matches[1].Trim() -ieq $Key) {
            if ($found) { throw "Ambiguous session configuration: $WantedSection/$Key" }
            $found = $true; $value = ($Matches[2] -split '[;#]',2)[0].Trim()
        }
    }
    $value
}
function SessionIdentity([string]$Root, [string]$SelectedRom) {
    $paths = [Collections.Generic.List[string]]::new()
    $paths.Add([IO.Path]::GetFullPath($SelectedRom))
    foreach ($name in @('config.ini','fzero-video.ini','keybinds.ini')) { $paths.Add((Join-Path $Root $name)) }
    $config = Join-Path $Root 'config.ini'
    $enabled = SessionIni $config 'Sound' 'Msu1Enabled' '0'
    if ($enabled -cnotmatch '^-?\d+$') { throw 'Invalid MSU selection.' }
    if ([int]$enabled -ne 0) {
        $pack = (SessionIni $config 'Sound' 'Msu1Dir' 'auto').Trim('"')
        if (!$pack -or $pack -in @('auto','1','on')) { $pack = [IO.Path]::ChangeExtension($SelectedRom, $null) }
        elseif (![IO.Path]::IsPathRooted($pack)) { $pack = Join-Path $Root $pack }
        if ($pack -notin @('off','0')) {
            $directory = if (Test-Path -LiteralPath $pack -PathType Container) { $pack } else { Split-Path -Parent $pack }
            if (!(Test-Path -LiteralPath $directory -PathType Container)) { throw 'Selected MSU music folder is missing.' }
            $files = @(Get-ChildItem -LiteralPath $directory -File | Where-Object { $_.Extension -in @('.pcm','.msu','.ips') } | Sort-Object FullName)
            if (!$files.Count -or $files.Count -gt 512 -or ($files | Measure-Object Length -Sum).Sum -gt 32GB) { throw 'Selected MSU inventory is missing or exceeds session bounds.' }
            foreach ($f in $files) { $paths.Add($f.FullName) }
        }
    }
    @($paths | Select-Object -Unique | ForEach-Object {
        [ordered]@{ path = $_; sha256 = if (Test-Path -LiteralPath $_ -PathType Leaf) { SessionHash $_ } else { $null } }
    })
}
function SessionCheckIdentity($Identity, [string]$Root, [string]$SelectedRom) {
    if (!$Identity -or $Identity.Count -lt 4 -or $Identity.Count -gt 516) { throw 'Invalid recorded ROM/config/MSU inventory.' }
    $current = SessionIdentity $Root $SelectedRom
    if ($current.Count -ne $Identity.Count) { throw 'Recorded ROM/config/MSU inventory changed.' }
    for ($i=0; $i -lt $Identity.Count; $i++) {
        if ($current[$i].path -ine $Identity[$i].path -or $current[$i].sha256 -cne $Identity[$i].sha256) { throw "Recorded ROM/config/MSU input changed: $($Identity[$i].path)" }
    }
}
function SessionTape([string]$Path) {
    $file = [IO.File]::OpenRead($Path)
    try {
        $header = [byte[]]::new(49)
        $file.ReadExactly($header)
        $count = [BitConverter]::ToUInt64($header, 40)
        if ([Text.Encoding]::ASCII.GetString($header,0,8) -cne 'FZPT0001' -or $header[48] -ne 1 -or
            $count -lt 1 -or $count -gt 2000000 -or $file.Length -ne 49 + 12 * $count) { throw 'F-Zero tape is incomplete or its frame count/length is invalid.' }
        [ordered]@{ frames = $count; effectiveRomSha256 = [Convert]::ToHexString($header[8..39]).ToLowerInvariant() }
    } finally { $file.Dispose() }
}
function SessionStatus([string]$Folder, [string]$Id, $Launch) {
    $s = SessionJson (SessionBound $Folder 'status.json')
    if ($s.schema -cne 'fzero.session-status@1' -or $s.requestId -cne $Id -or
        $s.processId -ne $Launch.processId -or $s.processCreationFileTime -ne $Launch.processCreationFileTime -or
        $s.action -cne $Launch.action -or $s.state -cnotin @('starting','recording','playing','saved','verified','failed') -or
        $s.frames -lt 0 -or $s.frames -gt 2000000) { throw 'Ordinary F-Zero status does not belong to this request/process.' }
    $s
}
function SessionProcess($Launch) {
    $p = Get-Process -Id $Launch.processId -ErrorAction Stop
    if ($p.StartTime.ToUniversalTime().ToFileTimeUtc() -ne $Launch.processCreationFileTime -or
        [IO.Path]::GetFullPath($p.Path) -ine $Launch.exe) { throw 'The session process identity changed; no command was sent.' }
    $p
}
function SessionStop([string]$Folder, [string]$Id, $Launch) {
    $s = SessionStatus $Folder $Id $Launch
    $p = SessionProcess $Launch
    if ($Launch.action -ceq 'replay') {
        if ($s.state -cne 'playing' -or !$p.CloseMainWindow()) { throw 'The owned replay could not be closed normally.' }
        return
    }
    if ($s.state -ceq 'saved') { return }
    if ($s.state -cne 'recording') { throw 'Wait for the ordinary recorder to acknowledge this request.' }
    $path = SessionBound $Folder 'status.json.stop'
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes("fzero.session-stop@1`n$Id`n")
    if (Test-Path -LiteralPath $path) {
        if ([Convert]::ToHexString([IO.File]::ReadAllBytes($path)) -cne [Convert]::ToHexString($bytes)) { throw 'A different Stop request already exists.' }
        return
    }
    $file = [IO.File]::Open($path + '.tmp', [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try { $file.Write($bytes); $file.Flush($true) } finally { $file.Dispose() }
    [IO.File]::Move($path + '.tmp', $path, $false)
}
function InvokeInstalledSession {
    if (@($Record,$Playback,$Stop | Where-Object { $_ }).Count -ne 1 -or $Action -or $Tape -or !$GameDir -or !$Result -or !$RequestId) { throw 'Choose exactly one installed Record, Playback or Stop request.' }
    $root = [IO.Path]::GetFullPath($GameDir); $folder = [IO.Path]::GetFullPath($Result)
    if ($Record -and $InputOrigin -ceq 'automated') { throw 'Automated F-Zero recording requires a reviewed owner-state isolation companion. Human recording and muted playback are supported by this contract.' }
    $receipt = SessionCapability $root
    if ($Record -and (SessionIni (Join-Path $root 'config.ini') 'Input' 'Backend' 'Native') -ine 'Native') {
        throw 'Ordinary recording requires installed ownership of the selected input runtime. This installer owns Native only; a private Shared build receipt does not enable this route.'
    }
    if ($Stop) {
        $launched = SessionJson (SessionBound $folder 'companion.json')
        if ($launched.schema -cne 'fzero.companion-session@1' -or $launched.requestId -cne $RequestId -or
            $launched.exe -ine (Join-Path $root $receipt.launcher) -or $launched.exeSha256 -ine $receipt.launcherSha256) { throw 'Stop belongs to another installed launch.' }
        SessionStop $folder $RequestId $launched
        Write-Output 'Stop requested; completion remains the native recorder/status result.'
        return
    }
    if (!$Recording) { throw 'Choose a recording folder.' }
    $recordingRoot = [IO.Path]::GetFullPath($Recording)
    $separate = @($root, $folder, $recordingRoot) | ForEach-Object { $_.TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar }
    for ($i=0; $i -lt 3; $i++) { for ($j=0; $j -lt 3; $j++) {
        if ($i -ne $j -and $separate[$i].StartsWith($separate[$j], [StringComparison]::OrdinalIgnoreCase)) { throw 'Game, result and recording folders must be separate.' }
    } }
    if ((Test-Path -LiteralPath $folder) -or ($Record -and (Test-Path -LiteralPath $recordingRoot))) { throw 'Choose fresh session output folders.' }
    if (Get-Process | Where-Object { $_.ProcessName -like 'FZeroSNESRecomp*' }) { throw 'Close F-Zero before recording or replay.' }
    if (Get-ChildItem Env: | Where-Object { $_.Name -match '^(FZERO_|SNESRECOMP_)' -and $_.Value }) { throw 'Ordinary session launch refuses inherited FZERO_/SNESRECOMP_ overrides.' }
    if (!$Rom) {
        $Rom = Join-Path $root 'fzero.sfc'
        if (!(Test-Path -LiteralPath $Rom -PathType Leaf)) {
            $cache = SessionBound $root 'rom.cfg'
            if (!(Test-Path -LiteralPath $cache -PathType Leaf) -or (Get-Item -LiteralPath $cache).Length -gt 4096) { throw 'Select a verified ROM in the F-Zero launcher first.' }
            $Rom = [IO.File]::ReadLines($cache) | Select-Object -First 1
        }
    }
    if (!$Rom) { throw 'No ROM is selected.' }
    $Rom = [IO.Path]::GetFullPath($Rom, $root)
    if (!(Test-Path -LiteralPath $Rom -PathType Leaf)) { throw 'The selected ROM is missing.' }
    $tapePath = SessionBound $recordingRoot 'recording.fzpt'
    $descriptor = SessionBound $recordingRoot 'recording.json'
    $expected = $null
    if ($Playback) {
        if (!$RecordingSha256 -or (SessionHash $descriptor) -cne $RecordingSha256) { throw 'Recording identity changed after selection.' }
        $expected = SessionJson $descriptor
        if ($expected.schema -cne 'fzero.recording-package@1' -or $expected.gameBuildSha256 -ine $receipt.launcherSha256 -or
            $expected.sourceCommit -cne $receipt.sourceCommit -or $expected.romPath -ine $Rom -or
            (SessionHash $tapePath) -cne $expected.tapeSha256 -or (SessionHash ($tapePath + '.state')) -cne $expected.stateSha256) { throw 'The recording needs its original installed build, ROM and input/state pair.' }
        SessionCheckIdentity $expected.identity $root $Rom
        $header = SessionTape $tapePath
        if ($header.frames -ne $expected.frames -or $header.effectiveRomSha256 -cne $expected.effectiveRomSha256) { throw 'Recorded header and descriptor differ.' }
    }
    [void][IO.Directory]::CreateDirectory((Split-Path (SessionBound $folder 'status.json') -Parent))
    if ($Record) { [void][IO.Directory]::CreateDirectory($recordingRoot) }
    $start = [Diagnostics.ProcessStartInfo]::new((Join-Path $root $receipt.launcher))
    $start.WorkingDirectory = $root; $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true; $start.RedirectStandardError = $true
    foreach ($arg in @($(if ($Record) { '--record' } else { '--replay' }), $tapePath, '--session-id', $RequestId,
            '--session-status', (Join-Path $folder 'status.json'), $Rom)) { $start.ArgumentList.Add($arg) }
    if ($Playback) {
        # Existing replay save-root option avoids creating Deluxe/MSU folders in
        # the owner copy. Current consumer suffixes require a short root.
        $saves = Join-Path ([IO.Path]::GetTempPath()) ('fzr-' + $RequestId.Substring(0, 8))
        if ($saves.Length -gt 70 -or (Test-Path -LiteralPath $saves)) { throw 'A fresh short replay save root is required.' }
        [void][IO.Directory]::CreateDirectory($saves)
        $start.Environment['SNESRECOMP_SAVE_ROOT'] = $saves
    }
    if ($Record -and $InputOrigin -ceq 'automated') { $start.ArgumentList.Add('--mute-outputs') }
    $p = [Diagnostics.Process]::new(); $p.StartInfo = $start
    if (!$p.Start()) { throw 'F-Zero did not start.' }
    $out = $p.StandardOutput.ReadToEndAsync(); $err = $p.StandardError.ReadToEndAsync()
    $launch = [ordered]@{ schema = 'fzero.companion-session@1'; requestId = $RequestId; action = $(if ($Record) {'record'} else {'replay'})
        processId = $p.Id; processCreationFileTime = $p.StartTime.ToUniversalTime().ToFileTimeUtc(); exe = $start.FileName
        exeSha256 = $receipt.launcherSha256.ToLowerInvariant(); inputOrigin = $InputOrigin }
    $started = [DateTime]::UtcNow; $identity = $null; $sealed = $false; $timedOut = $false
    try {
        SessionWrite (Join-Path $folder 'companion.json') $launch
        while (!$p.HasExited) {
            if (Test-Path -LiteralPath (Join-Path $folder 'status.json')) {
                $s = SessionStatus $folder $RequestId $launch
                if ($s.state -cin @('recording','saved') -and !$identity) { $identity = SessionIdentity $root $Rom }
                if ($s.state -ceq 'saved' -and !$sealed) {
                    SessionCheckIdentity $identity $root $Rom
                    $header = SessionTape $tapePath
                    if ($header.frames -ne $s.frames -or $header.effectiveRomSha256 -cne $s.effectiveRomSha256) { throw 'Native completion and tape differ.' }
                    SessionWrite $descriptor ([ordered]@{ schema='fzero.recording-package@1'; requestId=$RequestId; recordedUtc=$started.ToString('o'); inputOrigin=$InputOrigin
                        frames=$header.frames; effectiveRomSha256=$header.effectiveRomSha256; gameBuildSha256=$launch.exeSha256; sourceCommit=$receipt.sourceCommit
                        romPath=$Rom; identity=$identity; tapeSha256=(SessionHash $tapePath); stateSha256=(SessionHash ($tapePath + '.state')) })
                    $sealed = $true
                    Write-Output 'Recording saved. The game remains open; close normally when finished.'
                }
            }
            $elapsed = ([DateTime]::UtcNow - $started).TotalSeconds
            if ($Playback -or $InputOrigin -ceq 'automated') {
                $deadline = if ($Playback) { [Math]::Ceiling($expected.frames / 60.0988) + 90 } else { $Seconds }
                if ($elapsed -gt $deadline -and !$timedOut) {
                    if ($Record -and $s.state -ceq 'recording') { SessionStop $folder $RequestId $launch }
                    $timedOut = $true
                }
                if ($timedOut -and $elapsed -gt $deadline + 5) { [void](SessionProcess $launch).CloseMainWindow() }
                if ($timedOut -and $elapsed -gt $deadline + 20) { throw 'Owned game did not close normally; retain the rig lease for recovery.' }
            }
            Start-Sleep -Milliseconds 100
        }
        $p.WaitForExit()
        $s = SessionStatus $folder $RequestId $launch
        # A fast ordinary close can complete between monitor polls. Seal using
        # the authoritative terminal state, never an inferred successful exit.
        if ($Record -and !$sealed -and $s.state -ceq 'saved') {
            if (!$identity) { throw 'Recording completed before its configuration identity was captured; evidence is retained.' }
            SessionCheckIdentity $identity $root $Rom
            $header = SessionTape $tapePath
            if ($header.frames -ne $s.frames -or $header.effectiveRomSha256 -cne $s.effectiveRomSha256) { throw 'Native completion and tape differ.' }
            SessionWrite $descriptor ([ordered]@{ schema='fzero.recording-package@1'; requestId=$RequestId; recordedUtc=$started.ToString('o'); inputOrigin=$InputOrigin
                frames=$header.frames; effectiveRomSha256=$header.effectiveRomSha256; gameBuildSha256=$launch.exeSha256; sourceCommit=$receipt.sourceCommit
                romPath=$Rom; identity=$identity; tapeSha256=(SessionHash $tapePath); stateSha256=(SessionHash ($tapePath + '.state')) })
            $sealed = $true
        }
        $passed = $p.ExitCode -eq 0 -and $(if ($Record) { $sealed -and $s.state -ceq 'saved' } else {
            $s.state -ceq 'verified' -and $s.outputMuted -eq $true -and $s.frames -eq $expected.frames -and $s.total -eq $expected.frames -and $s.effectiveRomSha256 -ceq $expected.effectiveRomSha256 })
        if ($Playback) {
            SessionCheckIdentity $expected.identity $root $Rom
            if ((SessionHash $descriptor) -cne $RecordingSha256 -or (SessionHash $tapePath) -cne $expected.tapeSha256 -or
                (SessionHash ($tapePath + '.state')) -cne $expected.stateSha256) { throw 'Recording changed during replay.' }
        }
        SessionWrite (Join-Path $folder 'result.json') ([ordered]@{ schema='fzero.session-result@1'; requestId=$RequestId; processId=$launch.processId
            processCreationFileTime=$launch.processCreationFileTime; passed=$passed; gameClosed=$true; exitCode=$p.ExitCode; frames=$s.frames; state=$s.state })
        if (!$passed) { throw 'Native recording/replay did not complete successfully. Evidence is retained.' }
    } finally {
        if (!$p.HasExited) { try { [void](SessionProcess $launch).CloseMainWindow(); [void]$p.WaitForExit(5000) } catch { Write-Warning $_.Exception.Message } }
        if ($p.HasExited) {
            [IO.File]::WriteAllText((Join-Path $folder 'game.stdout.log'), $out.GetAwaiter().GetResult())
            [IO.File]::WriteAllText((Join-Path $folder 'game.stderr.log'), $err.GetAwaiter().GetResult())
        } else { [IO.File]::WriteAllText((Join-Path $folder 'close-pending.txt'), "Exact owned process $($launch.processId)/$($launch.processCreationFileTime) remains live. Retain the shared lease.") }
        $p.Dispose()
    }
}
try {
    if ($Record -or $Playback -or $Stop) { InvokeInstalledSession; exit 0 }
    if (!$Action -or !$Rom -or !$Tape) { throw 'Use Record/Replay with -Rom and -Tape, or an installed session request.' }
    $packageRoot = Split-Path $PSScriptRoot -Parent
    $exe = Join-Path $packageRoot 'FZeroSNESRecomp.exe'
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw 'Run this helper from an extracted candidate containing FZeroSNESRecomp.exe.' }
    # Resolve against the caller first; the child runs beside its own assets.
    $romPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Rom)
    $tapePath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Tape)
    if (-not (Test-Path -LiteralPath $romPath -PathType Leaf)) { throw "ROM not found: $romPath" }
    if ($Action -eq 'Record') {
        if (Test-Path -LiteralPath $tapePath) { throw "Recording already exists: $tapePath" }
        if (Test-Path -LiteralPath ($tapePath + '.state')) { throw 'Recording state already exists; choose a fresh name.' }
        if (-not (Test-Path -LiteralPath (Split-Path $tapePath -Parent) -PathType Container)) { throw 'Create the recording folder first.' }
        Write-Host 'Recording from boot. Ctrl+F11 stops recording; close normally to save. Your configured force settings apply.'
    } else {
        if (-not (Test-Path -LiteralPath $tapePath -PathType Leaf) -or
            -not (Test-Path -LiteralPath ($tapePath + '.state') -PathType Leaf)) { throw 'Replay requires both the .fzpt recording and its .fzpt.state sibling.' }
        Write-Host 'Replaying recorded input with forces and UDP delivery off. Keep the original ROM and game configuration.'
    }
    Push-Location -LiteralPath $packageRoot
    try {
        & $exe ('--' + $Action.ToLowerInvariant()) $tapePath $romPath | Out-Host
        $gameExit = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    exit $gameExit
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
