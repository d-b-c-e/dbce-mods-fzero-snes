# Authored inert-file checks only. Run in a future named offline slot; no DLL/game loads.
[CmdletBinding()]
param([string]$PythonExecutable='python')
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'Private-ControlsSetup.ps1')
$repo=Split-Path $PSScriptRoot -Parent
$fixture=Join-Path $repo ('build/install-controls-setup-'+[guid]::NewGuid().ToString('N'))
$bundle=Join-Path $fixture 'controls-setup'
[IO.Directory]::CreateDirectory($bundle)|Out-Null
$checks=0
function Check([bool]$Condition,[string]$Message){if(!$Condition){throw $Message};$script:checks++}
function Refuses([scriptblock]$Action,[string]$Message){$refused=$false;try{& $Action|Out-Null}catch{$refused=$true};Check $refused $Message}
foreach($name in @('controls-setup/DbceModSetup.exe','controls-setup/ja/Runtime.resources.dll')){Check (Test-PrivateControlsOwnedPath $name) 'Safe portable path refused'}
foreach($name in @('config.ini','controls-setup/../config.ini','controls-setup//x','controls-setup/NUL.txt','controls-setup/x.','controls-setup/x ','controls-setup/x:y','controls-setup\x')){Check (!(Test-PrivateControlsOwnedPath $name)) 'Unsafe owned path admitted'}
$prior=[pscustomobject]@{files=@([pscustomobject]@{name='controls-setup/old.dll'});privateControlsSetup=@{folder='controls-setup'}}
Refuses {Assert-PrivateControlsTransition $false $prior} 'Implicit helper ownership withdrawal admitted'
Refuses {Assert-PrivateControlsTransition $true $prior @('controls-setup/new.dll')} 'Removed helper path admitted'
Assert-PrivateControlsTransition $true $prior @('controls-setup/old.dll','controls-setup/new.dll');$checks++
Assert-PrivateControlsTransition $false $null;$checks++
$exe=Join-Path $bundle 'DbceModSetup.exe'
[IO.File]::WriteAllText($exe,'Inert apphost fixture, never executable',[Text.UTF8Encoding]::new($false))
$manifest=[ordered]@{schema=1;kind='private-standalone-helper';sourceCommit=('a'*40);rid='win-x64';selfContained=$true;supportedGames=@('f-zero');waitForProcessContract='fzero-exact-process-exit-1';waitForProcessTimeoutMs=60000;files=@(@{path='DbceModSetup.exe';bytes=(Get-Item -LiteralPath $exe).Length;sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()})}
$manifestPath=Join-Path $bundle 'bundle-manifest.json'
[IO.File]::WriteAllText($manifestPath,($manifest|ConvertTo-Json -Depth 6),[Text.UTF8Encoding]::new($false))
$manifestHash=(Get-FileHash -LiteralPath $manifestPath).Hash.ToLowerInvariant()
$exeHash=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
$linked=$manifestHash+[char]0+$exeHash+[char]0
$result=Private-ControlsSetupSources $repo $fixture $linked $PythonExecutable
Check ($result.files.Count -eq 2 -and $result.expected.Count -eq 2) 'Exact whole ownership inventory missing'
Check ($result.metadata.manifestSha256 -ceq $manifestHash -and $result.metadata.exeSha256 -ceq $exeHash) 'Bound runtime identities differ'
Refuses {Private-ControlsSetupSources $repo $fixture 'not the compiled helper identity' $PythonExecutable} 'Unbound helper admitted'
[IO.File]::AppendAllText($exe,'changed')
Refuses {Private-ControlsSetupSources $repo $fixture $linked $PythonExecutable} 'Changed payload admitted'
"PASS $checks inert controls-setup ownership/admission checks; fixture retained at $fixture"
