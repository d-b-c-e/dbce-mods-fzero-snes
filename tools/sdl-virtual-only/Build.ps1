param(
    [Parameter(Mandatory=$true)][string]$SdlSource,
    [string]$Output = "$PSScriptRoot/../../.local/sdl-virtual-only"
)
$ErrorActionPreference = 'Stop'
function Replace-Once([string]$Text, [string]$Old, [string]$New) {
    if ([regex]::Matches($Text, [regex]::Escape($Old)).Count -ne 1) { throw "Expected one patch match: $Old" }
    return $Text.Replace($Old, $New)
}
$SdlSource = (Resolve-Path -LiteralPath $SdlSource).Path
$Output = [IO.Path]::GetFullPath($Output)
if (Test-Path -LiteralPath $Output) { throw 'Use a fresh output directory.' }
if ((Get-Content "$SdlSource/.git-hash" -Raw).Trim() -ne '8e37db5e797b6167f3a00d697d816a684bd259c7') {
    throw 'This dependency recipe requires SDL release-3.4.10.'
}
$expected = Get-Content "$PSScriptRoot/expected-source.json" -Raw | ConvertFrom-Json
foreach ($entry in $expected.PSObject.Properties) {
    if ((Get-FileHash "$SdlSource/$($entry.Name)").Hash -ne $entry.Value) { throw "Pre-patch hash mismatch: $($entry.Name)" }
}
New-Item -ItemType Directory -Path $Output | Out-Null
$copy = "$Output/source"
Copy-Item -LiteralPath $SdlSource -Destination $copy -Recurse
Get-ChildItem -LiteralPath $copy -File -Recurse | Sort-Object FullName | ForEach-Object {
    '{0}  {1}' -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash, $_.FullName.Substring($copy.Length + 1).Replace('\','/')
} | Set-Content "$Output/source-sha256.txt"
# Local fixture dependency only: never alter the cached or production SDL.
$cmake = [IO.File]::ReadAllText("$copy/CMakeLists.txt")
$originalHash = (Get-FileHash "$copy/CMakeLists.txt" -Algorithm SHA256).Hash
$needle = '  if(SDL_JOYSTICK)' + "`n" + '    sdl_glob_sources(' + "`n" + '      "${SDL3_SOURCE_DIR}/src/joystick/windows/*.c"'
$cmake = $cmake.Replace("`r`n", "`n")
if (-not $cmake.Contains($needle)) { throw 'Windows joystick block changed.' }
$cmake = Replace-Once $cmake $needle ($needle.Replace('if(SDL_JOYSTICK)', 'if(FALSE) # DBCE isolated fixture: exclude all physical Windows joystick drivers'))
$virtual = 'dep_option(SDL_VIRTUAL_JOYSTICK    "Enable the virtual-joystick driver" ON SDL_HIDAPI OFF)'
if (-not $cmake.Contains($virtual)) { throw 'Virtual joystick dependency changed.' }
$cmake = Replace-Once $cmake $virtual ($virtual.Replace('ON SDL_HIDAPI OFF', 'ON SDL_JOYSTICK OFF'))
[IO.File]::WriteAllText("$copy/CMakeLists.txt", $cmake)
# DBCE altered copied sources, never cached/production SDL.
$core = [IO.File]::ReadAllText("$copy/src/SDL.c").Replace("`r`n", "`n")
foreach ($call in @('    if (flags & (SDL_INIT_HAPTIC | SDL_INIT_JOYSTICK))', '    SDL_HelperWindowDestroy();')) {
    $core = Replace-Once $core ("#ifdef SDL_PLATFORM_WINDOWS`n" + $call) ("#if defined(SDL_PLATFORM_WINDOWS) && !defined(DBCE_SDL_VIRTUAL_FIXTURE)`n" + $call)
}
$guard = @'
/* DBCE altered source: private compile-only fixture. */
#ifdef DBCE_SDL_VIRTUAL_FIXTURE
#if !defined(SDL_JOYSTICK_VIRTUAL) || !defined(SDL_JOYSTICK_DUMMY) || !defined(SDL_VIDEO_DISABLED) || !defined(SDL_HAPTIC_DISABLED)
#error Invalid DBCE fixture configuration
#endif
#if defined(SDL_JOYSTICK_DINPUT) || defined(SDL_JOYSTICK_XINPUT) || defined(SDL_JOYSTICK_RAWINPUT) || defined(SDL_JOYSTICK_WGI) || defined(SDL_JOYSTICK_GAMEINPUT) || defined(SDL_JOYSTICK_HIDAPI)
#error Physical backend in DBCE fixture
#endif
#endif
'@
$core = Replace-Once $core '#include "SDL_internal.h"' ('#include "SDL_internal.h"' + "`n" + $guard)
[IO.File]::WriteAllText("$copy/src/SDL.c", $core)
$dyn = [IO.File]::ReadAllText("$copy/src/dynapi/SDL_dynapi.h")
$dyn = Replace-Once $dyn '#if defined(SDL_PLATFORM_PRIVATE)' "/* DBCE altered source: forbid replacement SDL loading. */`n#if defined(DBCE_SDL_VIRTUAL_FIXTURE)`n#define SDL_DYNAMIC_API 0`n#elif defined(SDL_PLATFORM_PRIVATE)"
[IO.File]::WriteAllText("$copy/src/dynapi/SDL_dynapi.h", $dyn)
$steam = [IO.File]::ReadAllText("$copy/src/joystick/SDL_steam_virtual_gamepad.c").Replace("`r`n", "`n")
$steam = Replace-Once $steam "void SDL_InitSteamVirtualGamepadInfo(void)`n{" "void SDL_InitSteamVirtualGamepadInfo(void)`n{`n#ifdef DBCE_SDL_VIRTUAL_FIXTURE`n    /* DBCE altered source: no inherited Steam metadata file. */`n    return;`n#endif"
[IO.File]::WriteAllText("$copy/src/joystick/SDL_steam_virtual_gamepad.c", $steam)
$options = @('-G','Ninja','-DCMAKE_C_FLAGS=-DDBCE_SDL_VIRTUAL_FIXTURE=1','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','-DCMAKE_BUILD_TYPE=Release','-DSDL_SHARED=OFF','-DSDL_STATIC=ON',
    '-DSDL_TEST_LIBRARY=OFF','-DSDL_TESTS=OFF','-DSDL_EXAMPLES=OFF','-DSDL_INSTALL=OFF',
    '-DSDL_JOYSTICK=ON','-DSDL_VIRTUAL_JOYSTICK=ON','-DSDL_HIDAPI=OFF',
    '-DSDL_HIDAPI_JOYSTICK=OFF','-DSDL_XINPUT=OFF','-DSDL_HAPTIC=OFF',
    '-DSDL_VIDEO=OFF','-DSDL_RENDER=OFF','-DSDL_GPU=OFF','-DSDL_AUDIO=OFF',
    '-DSDL_SENSOR=OFF','-DSDL_CAMERA=OFF','-DSDL_POWER=OFF','-DSDL_TRAY=OFF')
@{ sourceRevision=(Get-Content "$copy/REVISION.txt" -Raw).Trim(); sourceCommit=(Get-Content "$copy/.git-hash" -Raw).Trim();
   sourcePath=$SdlSource; originalCmakeSHA256=$originalHash;
   patchedCmakeSHA256=(Get-FileHash "$copy/CMakeLists.txt" -Algorithm SHA256).Hash;
   options=$options; runtimeAllowed=$false;
   runtimeBlocker='Compile/link only; source recheck and recovery/rig gates remain.'
} | ConvertTo-Json -Depth 5 | Set-Content "$Output/provenance.json"
& cmake -S $copy -B "$Output/build" @options *> "$Output/configure.log"
if ($LASTEXITCODE) { throw "SDL configure failed; see $Output/configure.log" }
$config = [IO.File]::ReadAllText("$Output/build/include-config-release/build_config/SDL_build_config.h")
if ($config -match '(?m)^#define SDL_JOYSTICK_(DINPUT|XINPUT|RAWINPUT|WGI|GAMEINPUT|HIDAPI)\s+1') { throw 'Physical joystick backend remains enabled.' }
if ($config -notmatch '(?m)^#define SDL_JOYSTICK_VIRTUAL\s+1') { throw 'Virtual joystick backend absent.' }
$enabled = @([regex]::Matches($config, '(?m)^#define (SDL_JOYSTICK_\w+)\s+1') | ForEach-Object { $_.Groups[1].Value } | Sort-Object)
if (($enabled -join ',') -ne 'SDL_JOYSTICK_DUMMY,SDL_JOYSTICK_VIRTUAL') { throw "Unexpected joystick macros: $enabled" }
$commands = Get-Content "$Output/build/compile_commands.json" -Raw | ConvertFrom-Json
$drivers = @($commands.file | ForEach-Object { $_.Replace('\','/') } | Where-Object { $_ -match '/src/joystick/' } | ForEach-Object { ($_ -split '/src/joystick/')[1] } | Sort-Object)
$allowed = @('SDL_gamepad.c','SDL_joystick.c','SDL_steam_virtual_gamepad.c','controller_type.c','dummy/SDL_sysjoystick.c','virtual/SDL_virtualjoystick.c') | Sort-Object
if (Compare-Object $allowed $drivers) { throw "Unexpected joystick sources: $drivers" }
$drivers | Set-Content "$Output/joystick-sources.txt"
& cmake --build "$Output/build" --target SDL3-static --parallel 2 *> "$Output/build.log"
if ($LASTEXITCODE) { throw "SDL build failed; see $Output/build.log" }
Get-FileHash "$Output/build/libSDL3.a" -Algorithm SHA256 | Format-List | Out-String | Set-Content "$Output/library-sha256.txt"
$provenance = Get-Content "$Output/provenance.json" -Raw | ConvertFrom-Json
$hashes = @{}
foreach ($entry in $expected.PSObject.Properties) { $hashes[$entry.Name] = (Get-FileHash "$copy/$($entry.Name)").Hash }
$provenance | Add-Member patchedSourceHashes $hashes
$provenance | Add-Member configSHA256 (Get-FileHash "$Output/build/include-config-release/build_config/SDL_build_config.h").Hash
$provenance | Add-Member archiveSHA256 (Get-FileHash "$Output/build/libSDL3.a").Hash
$provenance | Add-Member archivePath "$Output/build/libSDL3.a"
$provenance | ConvertTo-Json -Depth 8 | Set-Content "$Output/provenance.json"
Write-Output "Built compile-only dependency in $Output; runtime remains gated."
