# Pure invocation description, shared by the runner and the production-parser probe.
function Get-FzeroReplayInvocation([string]$Mode,[string]$App,[string]$Request,[string]$StatusPath) {
    $tape=Join-Path $App diagnostic.fzpt;$rom=Join-Path $App fzero.sfc
    if($Mode -eq 'Live'){
        return @{arguments=@('--replay',$tape,'--session-id',$Request,'--session-status',$StatusPath,$rom);replayEnvironment=$null}
    }
    if($Mode -eq 'Rehearse'){return @{arguments=@($rom);replayEnvironment=$tape}}
    throw 'Unknown replay invocation mode'
}
