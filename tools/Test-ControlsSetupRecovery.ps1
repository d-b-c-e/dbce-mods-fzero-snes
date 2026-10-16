#Requires -Version 7.0
[CmdletBinding()]
param([switch]$Execute)
if(!$Execute){'AUTHORED/UNRUN: focused inert-file helper rollback and install-receipt bounds; -Execute requires an offline slot.';return}
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$fixture=Join-Path $repo ('build/controls-recovery-'+[guid]::NewGuid().ToString('N'))
$Target=Join-Path $fixture 'target';$backup=Join-Path $fixture 'backup'
[IO.Directory]::CreateDirectory($Target)|Out-Null
[IO.Directory]::CreateDirectory($backup)|Out-Null
. (Join-Path $PSScriptRoot 'Private-ControlsSetup.ps1') # Definitions only.

# Import actual production functions without executing either installer/session
# script entry point. No duplicated restore or JSON-admission implementation.
foreach($entry in @(
    @{file='Install-StreamDeck.ps1';names=@('HashOrNull','Restore-Files')},
    @{file='Recording.ps1';names=@('SessionHash','SessionJson','SessionJsonBounded','SessionInstallReceipt','SessionUnique','SessionBound','SessionCapability')}
)){
    $tokens=$null;$errors=$null
    $ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $PSScriptRoot $entry.file),[ref]$tokens,[ref]$errors)
    if($errors.Count){throw 'Production source parse failed'}
    $definitions=@()
    foreach($name in $entry.names){
        $nodes=@($ast.FindAll({param($n)$n -is [Management.Automation.Language.FunctionDefinitionAst] -and $n.Name -ceq $name},$false))
        if($nodes.Count -ne 1){throw "Production function not unique: $name"}
        $definitions+=@($nodes[0].Extent.Text)
    }
    if($entry.file -ceq 'Recording.ps1'){
        # Preserve a real script origin for the companion self-identity check.
        # Only extracted production function definitions are written/executed.
        $recordingDefinitions=Join-Path $fixture 'recording-functions.ps1'
        [IO.File]::WriteAllText($recordingDefinitions,($definitions -join "`n"),[Text.UTF8Encoding]::new($false))
        . $recordingDefinitions
    }else{
        foreach($definition in $definitions){. ([scriptblock]::Create($definition))}
    }
}
$checks=0
function Check([bool]$Condition,[string]$Message){if(!$Condition){throw $Message};$script:checks++}
function Refuses([scriptblock]$Action,[string]$Message,[string]$Reason=''){
    $failure=$null;try{& $Action|Out-Null}catch{$failure=$_.Exception.Message}
    Check ($null -ne $failure -and (!$Reason -or $failure.Contains($Reason))) $Message
}
function Write-Inert([string]$Path,[string]$Text){
    [IO.Directory]::CreateDirectory((Split-Path $Path -Parent))|Out-Null
    [IO.File]::WriteAllText($Path,$Text,[Text.UTF8Encoding]::new($false))
}
$launcher='FZeroSNESRecomp-wheel-launcher.exe';$receiptName='install-receipt.json'
$restorableFiles=@($launcher,'WheelFfb.dll',$receiptName)
$helper='controls-setup/DbceModSetup.exe'
$originalReceipt='{"schemaVersion":1,"files":[]}'
$state=[pscustomobject]@{schemaVersion=1;privateControlsSetup=@{folder='controls-setup'};files=@()}
foreach($name in $restorableFiles){
    $text=if($name -ceq $receiptName){$originalReceipt}else{'original inert '+$name}
    Write-Inert (Join-Path $backup $name) $text
    Write-Inert (Join-Path $Target $name) ('new inert '+$name)
    $state.files+=@([pscustomobject]@{name=$name;existed=$true;sha256=(HashOrNull (Join-Path $backup $name))})
}
$state.files+=@([pscustomobject]@{name=$helper;existed=$false;sha256=$null})
Write-Inert (Join-Path $Target $helper) 'new inert apphost, never executable'
# Model a final receipt write failing after truncation, after payload copies.
Write-Inert (Join-Path $Target $receiptName) '{"schemaVersion":'
Restore-Files $backup $state -AdmittedCurrentHelperPaths @($helper)
foreach($name in $restorableFiles){Check ((HashOrNull (Join-Path $Target $name)) -ceq (HashOrNull (Join-Path $backup $name))) 'Partial receipt write prevented exact original restoration'}
Check (!(Test-Path -LiteralPath (Join-Path $Target $helper))) 'New helper file was not removed after failed receipt write'

# A transaction without helper files also supplies an explicit empty inventory;
# it must not accidentally fall back to reading damaged final output.
$nativeState=[pscustomobject]@{schemaVersion=1;files=@($state.files|Where-Object {$_.name -cne $helper})}
Write-Inert (Join-Path $Target $launcher) 'another failed payload'
Write-Inert (Join-Path $Target $receiptName) '{"incomplete":'
Restore-Files $backup $nativeState -AdmittedCurrentHelperPaths @()
Check ((HashOrNull (Join-Path $Target $launcher)) -ceq (HashOrNull (Join-Path $backup $launcher))) 'Empty admitted transaction inventory did not restore the launcher'
Check ([IO.File]::ReadAllText((Join-Path $Target $receiptName)) -ceq $originalReceipt) 'Empty inventory did not restore the original receipt'

# External/manual rollback still refuses a backup that predates currently
# owned helper paths, before changing any payload.
$manual=[ordered]@{schemaVersion=1;files=@(@{name='controls-setup/newer.dll';sha256=('A'*64)})}
Write-Inert (Join-Path $Target $receiptName) ($manual|ConvertTo-Json -Depth 4)
$before=HashOrNull (Join-Path $Target $launcher)
Refuses {Restore-Files $backup $state} 'Manual rollback bypassed the current helper inventory guard' 'predates'
Check ((HashOrNull (Join-Path $Target $launcher)) -ceq $before) 'Refused manual rollback changed payload'
Refuses {Restore-Files $backup $state -AdmittedCurrentHelperPaths @('controls-setup/../foreign.dll')} 'Internal transaction accepted an unsafe helper path' 'Invalid admitted'

# Structurally valid full ownership receipt, with actual inert member files.
# Read via the production install-only JSON reader, not a copied validator.
$files=@()
foreach($name in @($launcher,'WheelFfb.dll','tools/Recording.ps1')){
    if($name -ceq 'tools/Recording.ps1'){
        [IO.Directory]::CreateDirectory((Split-Path (Join-Path $Target $name) -Parent))|Out-Null
        [IO.File]::WriteAllBytes((Join-Path $Target $name),[IO.File]::ReadAllBytes($recordingDefinitions))
    }else{Write-Inert (Join-Path $Target $name) ('inert member '+$name)}
    $files+=@([ordered]@{name=$name;sha256=(HashOrNull (Join-Path $Target $name))})
}
foreach($index in 1..529){
    $name=('controls-setup/resources/fixture-library-{0:D4}.resources.dll' -f $index)
    Write-Inert (Join-Path $Target $name) 'inert payload, never loaded'
    $files+=@([ordered]@{name=$name;sha256=(HashOrNull (Join-Path $Target $name))})
}
$receipt=[ordered]@{schemaVersion=1;controlsProfileSchema=1;controlsProfileAdapter='fzero-raw-wheel-1'
    launcher=$launcher;launcherSha256=(HashOrNull (Join-Path $Target $launcher));wheelFfbSha256=(HashOrNull (Join-Path $Target 'WheelFfb.dll'))
    sourceCommit=('a'*40);files=$files;session=@{schema='fzero.session-capability@1';protocol='fzero.session-control@1';record=$true;replay=$true;stopShortcut='Ctrl+F11';companion='tools/Recording.ps1'}
    privateControlsSetup=@{folder='controls-setup';game='f-zero';manifestSha256=('a'*64);exeSha256=('b'*64)}}
$path=Join-Path $Target $receiptName
Write-Inert $path ($receipt|ConvertTo-Json -Depth 8)
Check ((Get-Item -LiteralPath $path).Length -gt 65536) 'Large install-receipt regression fixture is not above 64 KiB'
$loaded=SessionCapability $Target
Check ($loaded.schemaVersion -eq 1 -and $loaded.files.Count -eq $files.Count) 'Existing session capability validator rejected a valid large ownership receipt'
Refuses {SessionJson $path} 'Ordinary session/status metadata silently gained the install-only larger bound' 'oversized'
$receipt['padding']='x'*2097152
Write-Inert $path ($receipt|ConvertTo-Json -Depth 8)
Refuses {SessionInstallReceipt $Target} 'Install receipt exceeded its explicit 2 MiB bound' 'oversized'
Write-Inert $path '{"schemaVersion":1,"schemaVersion":1}'
Refuses {SessionInstallReceipt $Target} 'Larger install reader bypassed the existing duplicate-field validator' 'Duplicate'
"PASS $checks focused inert-file recovery/receipt checks; fixture retained at $fixture"
