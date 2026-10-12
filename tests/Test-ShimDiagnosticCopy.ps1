#Requires -Version 7.0
# Exercises actual Check on an already prepared private copy. No lease/game/device.
param([Parameter(Mandatory)][string]$Probe)
$ErrorActionPreference='Stop'
$Probe=[IO.Path]::GetFullPath($Probe)
if($Probe -notmatch '^[A-Za-z]:[\\/]fz-probe-[a-zA-Z0-9-]+$'){throw 'Only a private probe can be used'}
if(Test-Path -LiteralPath (Join-Path $Probe 'run')){throw 'Never tamper a runtime probe; use an unlaunched copy'}
$runner=Join-Path $PSScriptRoot '../tools/Invoke-ShimDiagnosticCopy.ps1'
& $runner -Action Check -Probe $Probe
foreach($name in @('app/config.ini','tools/read_journal.py')){
    $path=Join-Path $Probe $name
    $original=[IO.File]::ReadAllBytes($path)
    try {
        [IO.File]::WriteAllBytes($path,($original+[byte[]]@(10,35,116,97,109,112,101,114,10)))
        $refused=$false
        try{& $runner -Action Check -Probe $Probe}catch{if($_.Exception.Message -match 'Hash mismatch'){$refused=$true}else{throw}}
        if(!$refused){throw "Changed pin accepted: $name"}
    } finally {[IO.File]::WriteAllBytes($path,$original)}
}
$extra=Join-Path $Probe 'app/unlisted-test.dll'
if(Test-Path -LiteralPath $extra){throw 'Test file already exists'}
try {
    [IO.File]::WriteAllText($extra,'not a DLL')
    $refused=$false
    try{& $runner -Action Check -Probe $Probe}catch{if($_.Exception.Message -match 'inventory changed'){$refused=$true}else{throw}}
    if(!$refused){throw 'Additional runtime file accepted'}
} finally {Remove-Item -LiteralPath $extra}
& $runner -Action Check -Probe $Probe
# Exercise the actual bounded-helper function with harmless PowerShell children;
# no game, lease, device or desktop input is involved.
$syntaxErrors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($runner,[ref]$null,[ref]$syntaxErrors)
if($syntaxErrors){throw $syntaxErrors[0]}
foreach($name in @('Write-Json','Invoke-BoundedHelper')){
    $definition=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name},$false)
    . ([scriptblock]::Create($definition.Extent.Text))
}
$utf8=[Text.UTF8Encoding]::new($false)
$retainedBefore=$script:retain
try {
    $script:retain=$false
    $text=Invoke-BoundedHelper 'offline-helper-ok' (Get-Process -Id $PID).Path @('-NoProfile','-NonInteractive','-Command','[Console]::WriteLine("ok")') 10
    if($text.Trim() -cne 'ok' -or $script:retain){throw 'Normal helper result lost'}
    $refused=$false
    try{Invoke-BoundedHelper 'offline-helper-timeout' (Get-Process -Id $PID).Path @('-NoProfile','-NonInteractive','-Command','[Threading.Thread]::Sleep(2000)') 1}catch{if($_.Exception.Message -match 'exceeded its deadline'){$refused=$true}else{throw}}
    if(!$refused -or !$script:retain){throw 'Timed-out helper did not refuse and retain'}
    $identity=Get-Content -LiteralPath (Join-Path $Probe 'offline-helper-timeout-process.json') -Raw | ConvertFrom-Json
    $child=Get-Process -Id $identity.pid -ErrorAction SilentlyContinue
    if($child){try{if(!$child.WaitForExit(5000)){throw 'Harmless test child failed to finish on its own'}}finally{$child.Dispose()}}
}finally{$script:retain=$retainedBefore}
'PASS production Check: changed config/helper and additional file refused; bounded helper exits or retains unresolved identity; owner pins unchanged; no game launch'
