#Requires -Version 7.0
param([Parameter(Mandatory)][string]$WheelFfbDll,
      [Parameter(Mandatory)][string]$ShimDll,
      [Parameter(Mandatory)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$pinPath=Join-Path $repo 'vendor/dbce-shim/SHARED-FORCE-PIN.json'
$pin=Get-Content -LiteralPath $pinPath -Raw | ConvertFrom-Json -AsHashtable
if($pin.schema -cne 'dbce.fzero-shared-force-dependencies.v1'){throw 'Unknown dependency pin schema'}
if(![IO.Path]::IsPathFullyQualified($OutputDirectory)){throw 'An absolute fresh output directory is required'}
$destination=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $destination){throw 'Output directory already exists; preserve the existing candidate'}
$inputs=[ordered]@{'WheelFfb.dll'=[IO.Path]::GetFullPath($WheelFfbDll);'dbce_shim.dll'=[IO.Path]::GetFullPath($ShimDll)}
foreach($name in $inputs.Keys){
    if((Get-FileHash -LiteralPath $inputs[$name] -Algorithm SHA256).Hash.ToLowerInvariant() -cne $pin.files[$name]){throw "Unqualified dependency: $name"}
}
[IO.Directory]::CreateDirectory($destination)|Out-Null
foreach($name in $inputs.Keys){
    $target=Join-Path $destination $name
    Copy-Item -LiteralPath $inputs[$name] -Destination $target
    if((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant() -cne $pin.files[$name]){throw "Copied dependency failed verification: $name"}
}
$receipt=[ordered]@{schema=$pin.schema;stagedUtc=[DateTime]::UtcNow.ToString('O');inputs=$inputs;
    files=$pin.files;pinSha256=(Get-FileHash -LiteralPath $pinPath -Algorithm SHA256).Hash.ToLowerInvariant();
    gameRuntimeSourceCommit=$pin.gameRuntimeSourceCommit;native=$pin.native;shim=$pin.shim;
    dllsLoaded=$false;gameStarted=$false;physicalAcceptance=$false;ownerInstalled=$false}
$receipt|ConvertTo-Json -Depth 12|Set-Content -LiteralPath (Join-Path $destination 'PAIR.json') -Encoding utf8NoBOM
$destination
