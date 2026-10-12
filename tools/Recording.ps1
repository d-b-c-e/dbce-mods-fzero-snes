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
    [Parameter(Mandatory=$true, Position=0)][ValidateSet('Record','Replay')][string]$Action,
    [Parameter(Mandatory=$true)][string]$Rom,
    [Parameter(Mandatory=$true)][string]$Tape
)
$ErrorActionPreference = 'Stop'
try {
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
