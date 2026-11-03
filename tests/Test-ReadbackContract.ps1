# File/identity fixtures only. No native calls, hooks, lease, game or DLL load.
[CmdletBinding()]
param([string]$EvidenceRoot='')
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot ../tools/rig/Readback-Contract.ps1)
if(-not $EvidenceRoot){$EvidenceRoot=Join-Path $PSScriptRoot ('../artifacts/readback-contract-'+[Guid]::NewGuid().ToString('N'))}
if(Test-Path -LiteralPath $EvidenceRoot){throw 'Fresh fixture directory required'}
[void][IO.Directory]::CreateDirectory($EvidenceRoot)
$script:checks=0
function Check($Value,[string]$Message){if(-not $Value){throw $Message};$script:checks++}
$before='HWND:0x123;PID:4;birth:100;image:foreign'
Check ($null -eq (Get-ReadbackAbortReason -Before $before -Now $before)) 'Unchanged foreign foreground was refused'
foreach($case in @(@{Physical=1},@{Injected=1},@{TickChanged=$true},@{IdentityMatches=$false},@{Now='changed'})){
    $args=@{Before=$before;Now=$before};foreach($key in $case.Keys){$args[$key]=$case[$key]}
    Check ([bool](Get-ReadbackAbortReason @args)) 'Unsafe identity/input case admitted'
}
$owner=Join-Path $EvidenceRoot owner;$missing=Join-Path $EvidenceRoot originally-absent
[void][IO.Directory]::CreateDirectory((Join-Path $owner empty-dir))
[IO.File]::WriteAllBytes((Join-Path $owner empty.bin),[byte[]]@())
[IO.File]::WriteAllBytes((Join-Path $owner settings.bin),[byte[]]@(0,255,13,10,65))
$winter=[DateTimeOffset]::Parse('2026-11-02T12:34:56.1234567Z').UtcDateTime
$summer=[DateTimeOffset]::Parse('2026-07-02T12:34:56.7654321Z').UtcDateTime
[IO.File]::SetLastWriteTimeUtc((Join-Path $owner settings.bin),$winter)
[IO.File]::SetLastWriteTimeUtc((Join-Path $owner empty.bin),$summer)
$emptyAttributes=[int][IO.File]::GetAttributes((Join-Path $owner empty.bin))
[IO.File]::SetAttributes((Join-Path $owner settings.bin),[Enum]::ToObject([IO.FileAttributes],524321)) # cloud-pinned + ReadOnly
$expected=Get-ChildItem -LiteralPath $owner -File|ForEach-Object {@{path=$_.FullName;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash;utcTicks=[string]$_.LastWriteTimeUtc.Ticks;attributes=[int]$_.Attributes}}
$expected|ConvertTo-Json|Set-Content (Join-Path $EvidenceRoot expected.json)
$snapshot=Save-ReadbackSnapshot @(@{path=$owner;kind='directory'},@{path=$missing;kind='directory'}) (Join-Path $EvidenceRoot before)
Check (Test-ReadbackRestored $snapshot).pass 'Immediate independent comparison failed'
Check ((Convert-ReadbackUtc $winter.ToLocalTime()).Ticks -eq $winter.Ticks) 'Typed Local DateTime conversion shifted instant'
Check ((Convert-ReadbackUtc ([DateTimeOffset]::Parse('2026-07-02T07:34:56.7654321-05:00'))).Ticks -eq $summer.Ticks) 'Offset DateTime conversion shifted instant'
try{Convert-ReadbackUtc ([DateTime]::SpecifyKind($winter,[DateTimeKind]::Unspecified))|Out-Null;throw 'Ambiguous timestamp admitted'}catch{Check ($_.Exception.Message -like 'Snapshot timestamp must retain*') 'Ambiguous timestamp did not refuse'}
[IO.File]::SetLastWriteTimeUtc((Join-Path $owner empty.bin),$summer.AddHours(1))
Check (-not(Test-ReadbackRestored $snapshot).pass) 'Timestamp-only drift was missed'
[IO.File]::SetLastWriteTimeUtc((Join-Path $owner empty.bin),$summer)
[IO.File]::SetAttributes((Join-Path $owner empty.bin),[IO.FileAttributes]::ReadOnly)
Check (-not(Test-ReadbackRestored $snapshot).pass) 'Attribute-only drift was missed'
[IO.File]::SetAttributes((Join-Path $owner empty.bin),[Enum]::ToObject([IO.FileAttributes],$emptyAttributes))
# Exercise both string snapshots and PowerShell JSON's typed-date materialization.
$snapshot=Get-Content (Join-Path $EvidenceRoot before/manifest.json) -Raw|ConvertFrom-Json
Check (Test-ReadbackRestored $snapshot).pass 'JSON timestamp materialization lost ticks'
[IO.File]::SetAttributes((Join-Path $owner settings.bin),[IO.FileAttributes]::Normal)
[IO.File]::WriteAllText((Join-Path $owner settings.bin),'changed')
[IO.File]::Delete((Join-Path $owner empty.bin));[IO.Directory]::Delete((Join-Path $owner empty-dir))
[IO.File]::WriteAllText((Join-Path $owner new.txt),'new')
[void][IO.Directory]::CreateDirectory((Join-Path $owner new-empty-dir))
[void][IO.Directory]::CreateDirectory($missing);[IO.File]::WriteAllText((Join-Path $missing new.txt),'new')
Check (-not (Test-ReadbackRestored $snapshot).pass) 'Independent comparison missed mutations'
Restore-ReadbackSnapshot $snapshot (Join-Path $EvidenceRoot after)
$result=Test-ReadbackRestored $snapshot
Check $result.pass ('Restore differs: '+($result.errors -join ', '))
Check ((Get-Item -LiteralPath (Join-Path $owner settings.bin)).Attributes -band [IO.FileAttributes]::ReadOnly) 'Original attributes were lost'
Check ((Get-Item -LiteralPath (Join-Path $owner empty.bin)).Length -eq 0) 'Empty file changed'
& pwsh -NoProfile -File (Join-Path $PSScriptRoot Verify-ReadbackFiles.ps1) -Expected (Join-Path $EvidenceRoot expected.json) -Report (Join-Path $EvidenceRoot independent-child.json)|Out-Null
Check ($LASTEXITCODE -eq 0) 'Independent child found byte/tick/attribute differences'
Check (-not(Test-ReadbackWithin ($owner+'-sibling') $owner)) 'Sibling prefix escaped containment'
try{Save-ReadbackSnapshot @(@{path=$owner;kind='directory'}) (Join-Path $owner forbidden)|Out-Null;throw 'Overlap admitted'}catch{Check ($_.Exception.Message -eq 'Evidence and owner root overlap') 'Unexpected overlap result'}
# Corrupted backup must stop before overwriting an owner file.
[IO.File]::SetAttributes($snapshot.roots[0].files[0].backup,[IO.FileAttributes]::Normal)
[IO.File]::WriteAllText($snapshot.roots[0].files[0].backup,'tampered')
try{Restore-ReadbackSnapshot $snapshot (Join-Path $EvidenceRoot tamper-after);throw 'Tamper admitted'}catch{Check ($_.Exception.Message -like 'Original backup changed*') 'Unexpected tamper result'}
Check (Test-ReadbackRestored $snapshot).pass 'Tamper refusal changed restored files'
[ordered]@{schema='fzero-readback-contract-test@1';status='PASS';checks=$script:checks;restore=$result;root=$EvidenceRoot;nativeCalls=$false}|ConvertTo-Json -Depth 6
