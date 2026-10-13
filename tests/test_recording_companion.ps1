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
    Check ($identity.Count -eq 4) 'Expected ROM plus three configuration members.'
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
    Check ($msu.Count -eq 5) 'Selected MSU files must be in the identity.'
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
    Write-Output "F-Zero ordinary companion: $script:checks checks passed; no child launch or device access."
} finally {
    # Only this fresh synthetic fixture directory, under the resolved temp root.
    $resolved = [IO.Path]::GetFullPath($root)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar
    if (!$resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase) -or !(Split-Path $resolved -Leaf).StartsWith('fzero-session-fixture-')) { throw 'Fixture cleanup path escaped temp.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
