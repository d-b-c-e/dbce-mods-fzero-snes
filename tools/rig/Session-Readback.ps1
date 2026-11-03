# An identity-bound startup record is not a replay-readiness receipt.
function Get-FzeroReplayAdmission($Status,[string]$Request,$Identity,[string]$Rom,[long]$Total,[bool]$ReadySeen=$false) {
    if($Status.schema -cne 'fzero.session-status@1' -or $Status.requestId -cne $Request -or $Status.processId -ne $Identity.pid -or [long]$Status.processCreationFileTime -ne ($Identity.birth-504911232000000000) -or $Status.action -cne 'replay' -or $Status.outputMuted -ne $true){throw 'Session identity/schema/mute mismatch'}
    if($Status.state -ceq 'starting'){
        if($ReadySeen -or $Status.frames -ne 0 -or $Status.total -ne 0 -or $Status.effectiveRomSha256 -cne ''){throw 'Invalid or regressed starting status'}
        return @{ready=$false;phase='starting'}
    }
    if($Status.state -cnotin @('playing','verified') -or $Status.effectiveRomSha256 -cne $Rom -or $Status.total -ne $Total -or $Status.frames -lt 0 -or $Status.frames -gt $Total){throw 'Replay state/ROM/frame contract mismatch'}
    @{ready=$true;phase=$Status.state;frames=$Status.frames}
}
