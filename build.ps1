<#
.SYNOPSIS
    Configures and builds F-Zero (Release, Ninja, MSVC x64); -Deluxe builds the integration launcher with BS Deluxe.

.DESCRIPTION
    Plain `.\build.ps1` keeps the original behaviour: the `build` folder, stock game only. `-Deluxe` configures the
    integration build the Stream Deck copy runs (`build-merge` by default, BS Deluxe embedded from the private
    generated sources under captures\bs-deluxe; see tools/regen_bs_deluxe.py). `-Test` runs the ROM-free ctest suites
    afterwards. The MSVC x64 environment is set up here when cl.exe is not already on PATH.
    Install the Deluxe build with `.\tools\Install-StreamDeck.ps1 -Label <name>`.
#>
[CmdletBinding()]
param([switch]$Deluxe, [string]$BuildDir, [switch]$Test)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $BuildDir) { $BuildDir = if ($Deluxe) { 'build-merge' } else { 'build' } }
$Build = Join-Path $Root $BuildDir
$configure = @('-S', "`"$Root`"", '-B', "`"$Build`"", '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release')
if ($Deluxe) {
    $gen = Join-Path $Root 'captures\bs-deluxe\gen'
    if (-not (Test-Path -LiteralPath $gen)) { throw "BS Deluxe sources not found at $gen; run tools/regen_bs_deluxe.py first." }
    $configure += "-DFZERO_DELUXE_GEN_DIR=`"$($gen.Replace('\', '/'))`""
}
$steps = @("cmake $($configure -join ' ')", "cmake --build `"$Build`" --config Release --parallel")
if ($Test) { $steps += "ctest --test-dir `"$Build`" --output-on-failure" }

if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    foreach ($s in $steps) { cmd /c $s; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }
    exit 0
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'cl.exe is not on PATH and vswhere.exe was not found; open a Developer prompt.' }
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path -LiteralPath $vcvars)) { throw "vcvars64.bat not found under $vs" }
cmd /c "call `"$vcvars`" >nul 2>nul && $($steps -join ' && ')"
exit $LASTEXITCODE
