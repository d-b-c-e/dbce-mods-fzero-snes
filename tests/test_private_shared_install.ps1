# Exercise only the existing installer's pure pin inventory and rollback helpers.
# Never execute its install body, inspect a real game, or load a DLL.
$ErrorActionPreference = 'Stop'
$source = Join-Path (Split-Path $PSScriptRoot -Parent) 'tools/Install-StreamDeck.ps1'
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'tools/Private-ControlsSetup.ps1')
$tokens=$null; $errors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile($source,[ref]$tokens,[ref]$errors)
if ($errors.Count) { throw ($errors | Out-String) }
foreach ($function in $ast.FindAll({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst]},$false)) { Invoke-Expression $function.Extent.Text }
$root=Join-Path ([IO.Path]::GetTempPath()) ('fzero-install-fixture-' + [guid]::NewGuid().ToString('N'))
$script:checks=0
function Check($Condition,[string]$Why) { $script:checks++; if (!$Condition) { throw $Why } }
function Refuses([scriptblock]$Action,[string]$Why) { $failed=$false; try { & $Action | Out-Null } catch { $failed=$true }; Check $failed $Why }
try {
    $nativeCapability=Recording-Capability $false ''
    Check (!$nativeCapability.automatedRecord -and $nativeCapability.recordBackends.Count -eq 1 -and $nativeCapability.recordBackends[0] -ceq 'Native') 'Native capability must preserve human-only admission.'
    Refuses { Recording-Capability $true '[fzero-shim]' } 'Existing Shared image must not gain automation without the native isolation seam.'
    Refuses { Recording-Capability $true '--isolate-owner-state' } 'A CLI token without the actual isolation marker must refuse.'
    $isolatedCapability=Recording-Capability $true '--isolate-owner-state [fzero-isolation]'
    Check ($isolatedCapability.automatedRecord -and (@($isolatedCapability.automatedRecordBackends)-join ',') -ceq 'Shared') 'Only owned Shared isolation may declare automated recording.'
    Check ($isolatedCapability.stopShortcut -ceq 'Ctrl+F11' -and $isolatedCapability.schema -ceq 'fzero.session-capability@1') 'Existing shortcut and capability schema must remain unchanged.'
    $vendor=Join-Path $root 'vendor/dbce-shim'; $build=Join-Path $root 'build'; $Target=Join-Path $root 'target'; $backup=Join-Path $root 'backup'
    foreach ($dir in @($vendor,$build,$Target,$backup)) { [void][IO.Directory]::CreateDirectory($dir) }
    [IO.File]::WriteAllText((Join-Path $build 'dbce_shim.dll'),'fixture ABI2 DLL')
    [IO.File]::WriteAllText((Join-Path $vendor 'dbce_shim.h'),'fixture ABI2 header')
    [IO.File]::WriteAllText((Join-Path $vendor 'PROVENANCE.md'),'private-only fixture')
    $pin=@{schema='dbce.fzero-shared-input-pin';version=1;abi=2;distribution='private-only';sourceRevision=('a' * 40)
        files=@{'dbce_shim.dll'=(HashOrNull (Join-Path $build 'dbce_shim.dll')).ToLowerInvariant();'dbce_shim.h'=(HashOrNull (Join-Path $vendor 'dbce_shim.h')).ToLowerInvariant()}}
    $pinPath=Join-Path $vendor 'PIN.json'
    [IO.File]::WriteAllText($pinPath,($pin | ConvertTo-Json -Depth 4))
    $shared=Private-SharedSources $root $build
    $sharedNames=@('dbce_shim.dll','shared-input/dbce_shim.h','shared-input/PIN.json','shared-input/PROVENANCE.md')
    Check ($shared.files.Count -eq 4 -and @($sharedNames | Where-Object { !$shared.files.ContainsKey($_) }).Count -eq 0) 'Private ownership must contain exactly the existing four paths.'
    Check ($shared.metadata.sourceRevision -ceq $pin.sourceRevision -and $shared.metadata.runtimeSha256 -ceq $pin.files.'dbce_shim.dll') 'Metadata must derive from the verified source pin.'
    Check ($shared.metadata.controls.consumer -ceq 'FZeroSNESRecomp-wheel-launcher.exe') 'Deluxe receipt must name its renamed launcher as the Shared controls consumer.'
    # One partition for both producers: the stock stager's contract with this installer's consumer.
    $compare="import json,sys; sys.path.insert(0, sys.argv[1]); import stage_unified as s; c = s.shared_controls_contract(); c['consumer'] = 'FZeroSNESRecomp-wheel-launcher.exe'; sys.exit(0 if json.loads(sys.stdin.read()) == c else 1)"
    $shared.metadata.controls | ConvertTo-Json -Depth 4 -Compress | & (Get-Command python).Source -I -c $compare (Join-Path (Split-Path $PSScriptRoot -Parent) 'tools')
    Check ($LASTEXITCODE -eq 0) 'Installer controls contract differs from the stock stager partition.'
    foreach ($name in @('dbce_shim.dll','shared-input/dbce_shim.h')) {
        $path=$shared.files[$name]; $old=[IO.File]::ReadAllBytes($path); [IO.File]::WriteAllText($path,'different')
        Refuses { Private-SharedSources $root $build } 'A changed DLL or header must refuse before installation.'
        [IO.File]::WriteAllBytes($path,$old)
    }
    foreach ($mutation in @('abi','distribution','extra')) {
        $copy=$pin.Clone(); $copy.files=$pin.files.Clone() # PS5.1 has no -AsHashtable
        if ($mutation -eq 'abi') { $copy.abi=3 } elseif ($mutation -eq 'distribution') { $copy.distribution='public' } else { $copy.files.'other.dll'='f' * 64 }
        [IO.File]::WriteAllText($pinPath,($copy | ConvertTo-Json -Depth 4))
        Refuses { Private-SharedSources $root $build } 'Unsupported pin metadata/inventory must refuse.'
    }
    [IO.File]::WriteAllText($pinPath,($pin | ConvertTo-Json -Depth 4))
    $provenance=Join-Path $vendor 'PROVENANCE.md'; $old=[IO.File]::ReadAllBytes($provenance); [IO.File]::Delete($provenance)
    Refuses { Private-SharedSources $root $build } 'Missing private provenance must refuse.'
    [IO.File]::WriteAllBytes($provenance,$old)
    # The new candidate owns the additive header; legacy four-file pins above
    # still validate without gaining extras admission.
    $extraHeader=Join-Path $vendor 'dbce_shim_extras.h'
    [IO.File]::WriteAllText($extraHeader,'fixture extras ABI1 header')
    $pin.extrasAbi=1
    $pin.files.'dbce_shim_extras.h'=(HashOrNull $extraHeader).ToLowerInvariant()
    [IO.File]::WriteAllText($pinPath,($pin | ConvertTo-Json -Depth 4))
    $shared=Private-SharedSources $root $build
    Check ($shared.files.Count -eq 5 -and $shared.files.ContainsKey('shared-input/dbce_shim_extras.h') -and $shared.metadata.extrasAbi -eq 1) 'Extras pin must own five paths and exact extension metadata.'
    $descriptor=Shared-ExtrasDescriptor
    Check ($shared.metadata.extras.declarationSha256 -ceq 'c4ffc4c85f1b7f72437e5871bdec57f9d5b997a62a9fdd3fd2208fefa0d98fb1' -and $descriptor.presentation.Count -eq 4) 'Private extras receipt must match exact four-button declaration.'
    Refuses { Assert-SharedExtrasConsumer 'old consumer' } 'Older linked consumer must not gain a descriptor from new DLL/header alone.'
    Assert-SharedExtrasConsumer ($descriptor.declaration+[char]0+'SharedExtras'+[char]0)
    Check $true 'Actual declaration and opt-in marker must admit descriptor publication.'
    $old=[IO.File]::ReadAllBytes($extraHeader);[IO.File]::WriteAllText($extraHeader,'changed')
    Refuses { Private-SharedSources $root $build } 'Changed additive header must refuse.'
    [IO.File]::Delete($extraHeader)
    Refuses { Private-SharedSources $root $build } 'Missing additive header must refuse.'
    [IO.File]::WriteAllBytes($extraHeader,$old)
    $pin.extrasAbi=2;[IO.File]::WriteAllText($pinPath,($pin | ConvertTo-Json -Depth 4))
    Refuses { Private-SharedSources $root $build } 'Unknown extras ABI must refuse.'
    $pin.extrasAbi=1;[IO.File]::WriteAllText($pinPath,($pin | ConvertTo-Json -Depth 4))
    $hosts=Shared-ExtrasDescriptor $true
    Check ($hosts.declarationSha256 -ceq '761789a517b35b88395d6b3ba06e2d2ba60628168abda6637e69d581a7a51cf7') 'Host declaration hash differs.'
    Check ($hosts.presentation.Count -eq 6 -and $hosts.optIn.key -ceq 'SharedHostControls') 'Host capability must remain explicit.'
    $hostLinked=$hosts.declaration+[char]0+'SharedExtras'+[char]0+'SharedHostControls'+[char]0
    Assert-SharedExtrasConsumer $hostLinked $true
    Refuses { Assert-SharedExtrasConsumer ($hosts.declaration+[char]0+'SharedExtras'+[char]0) $true } 'Host capability cannot be published without linked opt-in.'
    $hostSource=Private-SharedSources $root $build $true
    Check ($hostSource.metadata.extras.declarationSha256 -ceq $hosts.declarationSha256 -and $hostSource.files.Count -eq 5) 'Host inventory reuses the same qualified five files.'
    $hostReceipt=[ordered]@{privateSharedInput=$hostSource.metadata} | ConvertTo-Json -Depth 4 | ConvertFrom-Json
    Check ($hostReceipt.privateSharedInput.controls.unsupported.gear8 -ceq 'F-Zero has no gearbox' -and $hostReceipt.privateSharedInput.extras.presentation[5].id -ceq 'fzero-snes.rewind') 'The receipt writer depth must keep the controls and host contracts intact.'
    $hostPrevious=@{privateSharedInput=@{extras=@{declarationSha256=$hosts.declarationSha256}};files=@()}
    Refuses { Assert-SharedOwnershipTransition $true $hostPrevious } 'A host capability cannot be silently withdrawn on upgrade.'
    Assert-SharedOwnershipTransition $true $hostPrevious $null $true
    $sharedNames+= 'shared-input/dbce_shim_extras.h'
    $legacyNames=@($sharedNames | Where-Object { $_ -cne 'shared-input/dbce_shim_extras.h' })
    $fiveReceipt=@{privateSharedInput=@{abi=2;extrasAbi=1};files=@($sharedNames | ForEach-Object { @{name=$_} })}
    Refuses { Assert-SharedOwnershipTransition $true $fiveReceipt $legacyNames } 'Five-file ownership must refuse legacy source downgrade before mutation.'
    Refuses { Assert-SharedOwnershipTransition $true @{privateSharedInput=@{abi=2;extrasAbi=1};files=@()} $legacyNames } 'Extras metadata alone must not lose ownership through a legacy downgrade.'
    Assert-SharedOwnershipTransition $true $fiveReceipt $sharedNames
    Check $true 'Five-file source must preserve five-file ownership.'
    $fourReceipt=@{privateSharedInput=@{abi=2};files=@($legacyNames | ForEach-Object { @{name=$_} })}
    Assert-SharedOwnershipTransition $true $fourReceipt $legacyNames
    Check $true 'Legacy source must preserve legacy ownership.'
    Assert-SharedOwnershipTransition $true $fourReceipt $sharedNames
    Check $true 'Legacy ownership may upgrade to the additive five-file source.'
    $launcher='FZeroSNESRecomp-wheel-launcher.exe'; $receiptName='install-receipt.json'
    $restorableFiles=@($launcher,'WheelFfb.dll','tools/Recording.ps1',$receiptName)+$sharedNames
    $state=@{files=@()}
    foreach ($name in $restorableFiles) {
        $destination=Join-Path $Target $name; $saved=Join-Path $backup $name
        [void][IO.Directory]::CreateDirectory((Split-Path -Parent $destination)); [void][IO.Directory]::CreateDirectory((Split-Path -Parent $saved))
        $existed=$name -cne 'shared-input/PROVENANCE.md'
        if ($existed) { [IO.File]::WriteAllText($saved,$(if($name -ceq $receiptName){'{"schemaVersion":1,"files":[],"fixture":"old"}'}else{'old ' + $name})) }
        $state.files+=@{name=$name;existed=$existed;sha256=(HashOrNull $saved)}
        [IO.File]::WriteAllText($destination,$(if($name -ceq $receiptName){'{"schemaVersion":1,"files":[],"fixture":"new"}'}else{'new ' + $name}))
    }
    [IO.File]::WriteAllText((Join-Path $Target 'config.ini'),'owner settings fixture')
    Restore-Files $backup $state
    foreach ($entry in $state.files) { Check ((HashOrNull (Join-Path $Target $entry.name)) -ceq $entry.sha256) ('Rollback must restore exact prior ownership: ' + $entry.name) }
    Check ([IO.File]::ReadAllText((Join-Path $Target 'config.ini')) -ceq 'owner settings fixture') 'Rollback touched configuration.'
    $state.files+=@{name='../outside';existed=$false;sha256=$null}
    Refuses { Restore-Files $backup $state } 'Unowned rollback member must refuse before mutation.'
    # Execute the real entry point against a disposable synthetic target. The
    # transition refusal must precede even build validation and backup creation.
    $transition=Join-Path $root 'transition-target'
    [void][IO.Directory]::CreateDirectory($transition)
    foreach ($name in @($launcher,'config.ini')+$sharedNames) {
        $path=Join-Path $transition $name; [void][IO.Directory]::CreateDirectory((Split-Path -Parent $path))
        [IO.File]::WriteAllText($path,'retained ' + $name)
    }
    foreach ($metadataOnly in @($false,$true)) {
        $previous=@{schemaVersion=1;files=@(if (!$metadataOnly) { $sharedNames | ForEach-Object { @{name=$_;sha256=(HashOrNull (Join-Path $transition $_))} } })}
        if ($metadataOnly) { $previous.privateSharedInput=@{abi=2;distribution='private-only'} }
        [IO.File]::WriteAllText((Join-Path $transition $receiptName),($previous | ConvertTo-Json -Depth 4))
        $before=@(Get-ChildItem -LiteralPath $transition -File -Recurse | Sort-Object FullName | ForEach-Object { $_.FullName + ':' + (HashOrNull $_.FullName) })
        $ErrorActionPreference='Continue'; & pwsh -NoProfile -File $source -Target $transition -BuildDir $build -Label native-transition *> (Join-Path $root 'transition.log'); $ErrorActionPreference='Stop' # PS5.1 raises native stderr as errors
        Check ($LASTEXITCODE -ne 0 -and (Get-Content (Join-Path $root 'transition.log') -Raw).Contains('Keep -PrivateSharedInput')) 'Real installer did not refuse the exact Shared-to-Native ownership transition.'
        $after=@(Get-ChildItem -LiteralPath $transition -File -Recurse | Sort-Object FullName | ForEach-Object { $_.FullName + ':' + (HashOrNull $_.FullName) })
        Check (($before -join "`n") -ceq ($after -join "`n")) 'Refused transition changed target bytes or membership.'
        Check (!(Test-Path -LiteralPath (Join-Path $transition 'deployment-backups'))) 'Refused transition created a backup or began mutation.'
        Assert-SharedOwnershipTransition $true ($previous | ConvertTo-Json -Depth 4 | ConvertFrom-Json)
    }
    Write-Output "F-Zero private Shared installer: $script:checks checks passed; synthetic inventory/rollback only."
} finally {
    $resolved=[IO.Path]::GetFullPath($root)
    $temp=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
    if (!$resolved.StartsWith($temp,[StringComparison]::OrdinalIgnoreCase) -or !(Split-Path $resolved -Leaf).StartsWith('fzero-install-fixture-')) { throw 'Fixture cleanup escaped temp.' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
