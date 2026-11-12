<# Builds this branch as upstream builds its releases (MSYS2 MINGW64, GCC 15.2.0, Ninja, dynamic SDL3) and puts the
   MinGW DLLs the programs import beside them, so the build folder runs as it is. AGENTS.md has the toolchain.

   pwsh tools/build-wheelkit.ps1 -WheelkitSdk <built wheelkit checkout> [-Build <folder>] [-Msys <msys64 folder>]
   Without -WheelkitSdk the result is the stock upstream program. Needs fzero.sfc and `bash tools/regen.sh` first. #>
[CmdletBinding()]
param(
    [string]$WheelkitSdk = '',
    [string]$Build = 'E:\Source\temp\f-zero\build-wheelkit-runtime',
    [string]$Msys = 'E:\Source\temp\f-zero\msys64',
    [string]$CMake = 'C:\Program Files\CMake\bin\cmake.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$mingw = Join-Path $Msys 'mingw64'
$gcc = (& "$mingw\bin\gcc.exe" -dumpfullversion).Trim()
if ($gcc -ne '15.2.0') { throw "GCC $gcc in $mingw; upstream's releases are built with 15.2.0 (AGENTS.md)" }
$env:PATH = "$mingw\bin;$env:PATH"
$options = @('-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_PREFIX_PATH=$($mingw -replace '\\', '/')",
    '-DCMAKE_C_COMPILER=gcc', '-DCMAKE_CXX_COMPILER=g++', "-DCMAKE_MAKE_PROGRAM=$($mingw -replace '\\', '/')/bin/ninja.exe")
if ($WheelkitSdk) { $options += "-DWHEELKIT_SDK=$($WheelkitSdk -replace '\\', '/')" }
& $CMake -S $root -B $Build @options
if ($LASTEXITCODE) { throw 'configure failed' }
& $CMake --build $Build
if ($LASTEXITCODE) { throw 'build failed' }
# The MinGW DLLs the desktop program imports, and theirs, from the toolchain (upstream's tools/make_release.py does the
# same for a release).
$seen = @{}
$queue = [Collections.Generic.Queue[string]]::new()
$queue.Enqueue((Join-Path $Build 'F-Zero Forever.exe'))
while ($queue.Count) {
    foreach ($line in (& "$mingw\bin\objdump.exe" -p $queue.Dequeue() | Select-String 'DLL Name: (\S+)')) {
        $name = $line.Matches[0].Groups[1].Value
        $source = Join-Path "$mingw\bin" $name
        if ($seen[$name] -or -not (Test-Path -LiteralPath $source)) { continue }
        $seen[$name] = $true
        Copy-Item -LiteralPath $source -Destination $Build -Force
        $queue.Enqueue($source)
    }
}
"built $Build with GCC $gcc; $($seen.Count) MinGW DLL(s) beside the program"
