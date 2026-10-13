# Device-free fixtures for the installed companion's actual file helpers.
# Load only function definitions through the parser; never execute its launch body.
$ErrorActionPreference = 'Stop'
$source = Join-Path (Split-Path $PSScriptRoot -Parent) 'tools/Recording.ps1'
$tokens = $null; $errors = $null
$ast = [Management.Automation.Language.Parser]::ParseFile($source, [ref]$tokens, [ref]$errors)
if ($errors.Count) { throw ($errors | Out-String) }
foreach ($function in $ast.FindAll({ param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] }, $false)) {
    Invoke-Expression $function.Extent.Text
}
$root = Join-Path ([IO.Path]::GetTempPath()) ('fzero-session-fixture-' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($root)
$script:checks = 0
function Check($Condition, [string]$Why) { $script:checks++; if (!$Condition) { throw $Why } }
function Refuses([scriptblock]$Action, [string]$Why) {
    $failed = $false; try { & $Action } catch { $failed = $true }
    Check $failed $Why
}
try {
    foreach ($name in @('fzero.sfc','config.ini','fzero-video.ini','keybinds.ini')) { [IO.File]::WriteAllText((Join-Path $root $name), 'fixture ' + $name) }
    $rom = Join-Path $root 'fzero.sfc'
    $identity = SessionIdentity $root $rom
    Check ($identity.Count -eq 5 -and $null -eq $identity[4].sha256) 'Expected ROM, three configuration members and explicit absent Deluxe override.'
    SessionCheckIdentity $identity $root $rom
    $deluxe = Join-Path $root 'mods/bs-deluxe.dat'
    [void][IO.Directory]::CreateDirectory((Split-Path -Parent $deluxe))
    [IO.File]::WriteAllText($deluxe, 'private Deluxe fixture')
    Refuses { SessionCheckIdentity $identity $root $rom } 'A new Deluxe override must invalidate recorded absence.'
    $withDeluxe = SessionIdentity $root $rom
    SessionCheckIdentity $withDeluxe $root $rom
    [IO.File]::WriteAllText($deluxe, 'changed private Deluxe fixture')
    Refuses { SessionCheckIdentity $withDeluxe $root $rom } 'Changed Deluxe override bytes must refuse.'
    [IO.File]::Delete($deluxe)
    Refuses { SessionCheckIdentity $withDeluxe $root $rom } 'Removing a recorded Deluxe override must refuse.'
    SessionCheckIdentity $identity $root $rom
    $old = [IO.File]::ReadAllText((Join-Path $root 'config.ini'))
    [IO.File]::WriteAllText((Join-Path $root 'config.ini'), 'changed')
    Refuses { SessionCheckIdentity $identity $root $rom } 'Real outside configuration change must refuse.'
    [IO.File]::WriteAllText((Join-Path $root 'config.ini'), $old)
    [IO.File]::Delete((Join-Path $root 'keybinds.ini'))
    Refuses { SessionCheckIdentity $identity $root $rom } 'Missing recorded file must refuse.'
    [IO.File]::WriteAllText((Join-Path $root 'keybinds.ini'), 'fixture keybinds.ini')
    [IO.File]::WriteAllText((Join-Path $root 'config.ini'), "[Sound]`nMsu1Enabled=1`nMsu1Dir=$root`n")
    [IO.File]::WriteAllText((Join-Path $root 'track.pcm'), 'audio fixture')
    $msu = SessionIdentity $root $rom
    Check ($msu.Count -eq 6) 'Selected MSU files and absent Deluxe override must be in the identity.'
    [IO.File]::WriteAllText((Join-Path $root 'new.pcm'), 'new audio fixture')
    Refuses { SessionCheckIdentity $msu $root $rom } 'New MSU track must invalidate inventory.'
    Refuses { SessionBound $root '../outside' } 'Parent traversal must refuse.'
    Refuses { SessionBound $root 'C:/outside' } 'Absolute member must refuse.'
    $metadata = Join-Path $root 'metadata.json'
    SessionWrite $metadata ([ordered]@{ schema='fixture'; value=123 })
    Check ((SessionJson $metadata).value -eq 123) 'Atomic metadata did not round trip.'
    Refuses { SessionWrite $metadata @{ value=456 } } 'An immutable descriptor cannot be replaced.'
    [IO.File]::WriteAllText($metadata, '{"schema":"a","schema":"b"}')
    Refuses { SessionJson $metadata } 'Duplicate identity fields must refuse.'
    $tape = Join-Path $root 'fixture.fzpt'
    $bytes = [byte[]]::new(61)
    [Text.Encoding]::ASCII.GetBytes('FZPT0001').CopyTo($bytes,0)
    [BitConverter]::GetBytes([uint64]1).CopyTo($bytes,40); $bytes[48]=1
    [IO.File]::WriteAllBytes($tape,$bytes)
    Check ((SessionTape $tape).frames -eq 1) 'Existing FZPT0001 header must be accepted.'
    $bytes[48]=0; [IO.File]::WriteAllBytes($tape,$bytes)
    Refuses { SessionTape $tape } 'Incomplete native tape must refuse.'
    $bytes[48]=1; [BitConverter]::GetBytes([uint64]2).CopyTo($bytes,40); [IO.File]::WriteAllBytes($tape,$bytes)
    Refuses { SessionTape $tape } 'Truncated native tape must refuse.'
    $id = '0123456789abcdef0123456789abcdef'
    $launch = @{ processId=123; processCreationFileTime=456; action='record' }
    SessionWrite (Join-Path $root 'status.json') @{ schema='fzero.session-status@1'; requestId=$id; action='record'; state='saved'; processId=123; processCreationFileTime=456; frames=1 }
    Check ((SessionStatus $root $id $launch).state -ceq 'saved') 'Native saved status must remain authoritative without closing the window.'
    Refuses { SessionStatus $root ('f' * 32) $launch } 'Foreign request must refuse.'
    $launch.processCreationFileTime=457
    Refuses { SessionStatus $root $id $launch } 'Reused PID with another creation time must refuse.'
    # Synthetic installed Shared ownership. No DLL is loaded.
    [void][IO.Directory]::CreateDirectory((Join-Path $root 'shared-input'))
    foreach ($name in @('dbce_shim.dll','shared-input/dbce_shim.h','shared-input/PROVENANCE.md')) {
        [IO.File]::WriteAllText((Join-Path $root $name), 'fixture ' + $name)
    }
    $pin = @{ schema='dbce.fzero-shared-input-pin'; version=1; abi=2; distribution='private-only'; sourceRevision=('a' * 40)
        files=@{'dbce_shim.dll'=(SessionHash (Join-Path $root 'dbce_shim.dll')); 'dbce_shim.h'=(SessionHash (Join-Path $root 'shared-input/dbce_shim.h'))} }
    $pinPath = Join-Path $root 'shared-input/PIN.json'
    [IO.File]::WriteAllText($pinPath, ($pin | ConvertTo-Json -Depth 4))
    $receipt = @{ session=@{recordBackends=@('Native','Shared')}; privateSharedInput=@{abi=2;distribution='private-only';sourceRevision=$pin.sourceRevision;runtimeSha256=$pin.files.'dbce_shim.dll'}
        files=@('dbce_shim.dll','shared-input/dbce_shim.h','shared-input/PIN.json','shared-input/PROVENANCE.md') | ForEach-Object { @{name=$_;sha256=(SessionHash (Join-Path $root $_))} } }
    foreach ($enabled in @('', '0', '+0 trailing', '-0 ; comment')) {
        $config = "[Input]`nBackend=Shared`n[ForceFeedback]`n" + $(if ($enabled) { "Enabled=$enabled`n" })
        [IO.File]::WriteAllText((Join-Path $root 'config.ini'), $config)
        SessionRecordBackend $root $receipt
        Check ([IO.File]::ReadAllText((Join-Path $root 'config.ini')) -ceq $config) 'Shared admission changed the output policy.'
    }
    foreach ($enabled in @('1','-1','invalid','99999999999999999999999999999999')) {
        [IO.File]::WriteAllText((Join-Path $root 'config.ini'), "[Input]`nBackend=Shared`n[ForceFeedback]`nEnabled=$enabled`n")
        Refuses { SessionRecordBackend $root $receipt } 'Enabled or ambiguous Shared force policy must refuse.'
    }
    [IO.File]::WriteAllText((Join-Path $root 'config.ini'), "[Input]`nBackend=Shared`n")
    $start=[Diagnostics.ProcessStartInfo]::new('fixture-never-started.exe')
    $automationId=[guid]::NewGuid().ToString('N')
    Refuses { SessionAutomatedStart $start $root $receipt $automationId } 'An old Shared receipt cannot gain automated admission.'
    Check ($start.ArgumentList.Count -eq 0) 'Refused automation must not partially configure the child.'
    $receipt.session.automatedRecord=$true
    $receipt.session.automatedRecordBackends=@('Shared')
    $privateRoot=SessionAutomatedStart $start $root $receipt $automationId
    Check ($start.ArgumentList.Count -eq 3 -and $start.ArgumentList[0] -ceq '--mute-outputs' -and
        $start.ArgumentList[1] -ceq '--isolate-owner-state' -and $start.ArgumentList[2] -ceq $privateRoot) 'The actual companion must request separate mute and isolation flags.'
    Check (!(Test-Path -LiteralPath $privateRoot)) 'Native must claim the fresh private root itself.'
    [void][IO.Directory]::CreateDirectory($privateRoot)
    try { Refuses { SessionAutomatedStart ([Diagnostics.ProcessStartInfo]::new()) $root $receipt $automationId } 'An existing private root must not be reused.' }
    finally { [IO.Directory]::Delete($privateRoot) }
    Refuses { SessionCheckAutomatedStatus @{outputMuted=$false} $true } 'Automated recording must require actual muted status.'
    SessionCheckAutomatedStatus @{outputMuted=$true} $true
    SessionCheckAutomatedStatus @{outputMuted=$false} $false # Human output policy stays unchanged.
    [IO.File]::WriteAllText((Join-Path $root 'config.ini'), "[Input]`nBackend=Native`n")
    Refuses { SessionAutomatedStart ([Diagnostics.ProcessStartInfo]::new()) $root $receipt $automationId } 'Private Shared metadata must not admit Native automation.'
    [IO.File]::WriteAllText((Join-Path $root 'config.ini'), "[Input]`nBackend=Shared`n[ForceFeedback]`nEnabled=1`n")
    Refuses { SessionAutomatedStart ([Diagnostics.ProcessStartInfo]::new()) $root $receipt $automationId } 'Automated admission must not bypass Shared force refusal.'
    [IO.File]::WriteAllText((Join-Path $root 'config.ini'), "[Input]`nBackend=Shared`n")
    foreach ($name in @('dbce_shim.dll','shared-input/dbce_shim.h','shared-input/PIN.json','shared-input/PROVENANCE.md')) {
        $path = Join-Path $root $name; $before = [IO.File]::ReadAllBytes($path)
        [IO.File]::WriteAllText($path, 'changed')
        Refuses { SessionRecordBackend $root $receipt } "Changed Shared ownership must refuse: $name"
        [IO.File]::WriteAllBytes($path, $before)
    }
    $receipt.session.recordBackends=@('Native')
    Refuses { SessionRecordBackend $root $receipt } 'Nearby shim bytes must not imply recording capability.'
    $receipt.session.recordBackends=@('Native','Shared')
    $receipt.privateSharedInput.sourceRevision='b' * 40
    Refuses { SessionRecordBackend $root $receipt } 'Metadata/source disagreement must refuse.'
    $receipt.privateSharedInput.sourceRevision=$pin.sourceRevision
    $pin.files.'dbce_shim.dll'='c' * 64
    [IO.File]::WriteAllText($pinPath, ($pin | ConvertTo-Json -Depth 4))
    ($receipt.files | Where-Object name -eq 'shared-input/PIN.json').sha256=SessionHash $pinPath
    Refuses { SessionRecordBackend $root $receipt } 'Receipt-owned pin still must match actual runtime bytes.'
    [IO.File]::WriteAllText((Join-Path $root 'config.ini'), "[ForceFeedback]`nEnabled=1`n")
    SessionRecordBackend $root $receipt
    Check ([IO.File]::ReadAllText((Join-Path $root 'config.ini')) -ceq "[ForceFeedback]`nEnabled=1`n") 'Default Native behavior changed.'
    [IO.File]::WriteAllText((Join-Path $root 'config.ini'), "[Input]`nBackend=unknown`n")
    Refuses { SessionRecordBackend $root $receipt } 'Unknown backend must refuse.'
    Write-Output "F-Zero ordinary companion: $script:checks checks passed; no child launch or device access."
} finally {
    # Only this fresh synthetic fixture directory, under the resolved temp root.
    $resolved = [IO.Path]::GetFullPath($root)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
    if (!$resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase) -or !(Split-Path $resolved -Leaf).StartsWith('fzero-session-fixture-')) { throw 'Fixture cleanup path escaped temp.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
