# DBC-328 isolated SDL dependency

Retained integration baseline: `09afbf538724b4fa202135c6cc274fa582c71c44`,
branch `egon/fzero-cardinal-integration-20261026`. Local source work only.

## Dependency and reproduction

SDL cached release `release-3.4.10-0-g8e37db5e7`, self-reported commit
`8e37db5e797b6167f3a00d697d816a684bd259c7`. This is an extracted cache,
not a standalone Git checkout: `git rev-parse` there resolves the enclosing
F-Zero repository and must not be reported as SDL provenance.
The recipe records the complete copied source SHA256 manifest as well as both
original and altered CMake hashes. No network download or upstream submission.

```powershell
./tools/sdl-virtual-only/Build.ps1 -SdlSource E:/Source/games/dbce-mods-fzero-snes/build-dine-extras-graphical-796cfeb/_deps/sdl3-src
```

Use a fresh output directory for every invocation (`-Output` overrides default
`.local/sdl-virtual-only`). GCC MinGW UCRT POSIX 16.2.0, CMake 4.4.3, Ninja.
The recipe preserves the cache and copies it into a disposable directory.
Its two explicitly marked, test-dependency-only CMake edits:

- Exclude the entire Windows physical joystick source/driver setup block.
- Make the virtual joystick option depend on joystick support instead of HIDAPI,
  allowing the HIDAPI subsystem itself to be disabled.

Exact options are in `Build.ps1` and generated `provenance.json`: static Release,
joystick/virtual on; HIDAPI, HIDAPI joystick, XInput, haptics, video, renderer,
GPU, audio, sensors, camera, power and tray off; SDL tests/examples/install off.
Generated configuration must have VIRTUAL and no DINPUT/XINPUT/RAWINPUT/WGI/
GAMEINPUT/HIDAPI joystick define. SDL also supplies its inert dummy fallback:
`GetCount` returns zero and `Detect` is empty. No production reader was edited.

The small CMake consumer harness reuses exactly the existing `test_gamepad.c`
and production sources, links `SDL3::SDL3-static` from the isolated build, and
deliberately registers no test. After the dependency build, compile it with:

```powershell
cmake -S tools/sdl-virtual-only -B .local/sdl-virtual-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DSDL3_DIR:PATH="$PWD/.local/sdl-virtual-only/build" -DRUNNER_ROOT:PATH=E:/Source/games/dbce-mods-fzero-snes/snesrecomp/runner -DRECOMP_UI_ROOT:PATH=E:/Source/games/dbce-mods-fzero-snes/recomp-ui
cmake --build .local/sdl-virtual-consumer --parallel 2
```

## Initialization verdict: do not execute

Two isolation hypotheses fail the strict task gate:

1. DBC-325: runtime hints leave the production Windows backend registered and
   its notification/detection initialization reachable.
2. DBC-328: excluding physical joystick backends removes that backend path,
   but SDL core still creates a Windows helper window. `SDL.c:484` recursively
   initializes joystick for GAMEPAD, then `SDL.c:365-369` calls
   `SDL_HelperWindowCreate`. `src/core/windows/SDL_windows.c:97-131` registers
   a class with `DefWindowProc` and creates a message-only `HWND_MESSAGE` window.
   It is not a visible game window or proof of physical enumeration, but it is
   still desktop/window-system access forbidden by this task. No third local
   workaround or initialization execution is warranted before approach review.

Virtual driver `Init` returns true and `Detect` is empty. Event initialization
does not initialize video; event pumping delegates to the video backend only
if one exists. `SDL_InitJoysticks` also initializes Steam virtual metadata:
`SteamVirtualGamepadInfo` can select a file; unset it before any future run.
These source facts are narrower than a proven inert runtime.

Archie review: [DBC-330](/DBC/issues/DBC-330). Review a dependency-only core
guard and remaining startup/event paths; do not alter production guards.

Review returned during this heartbeat: backend exclusion is sound, immediate
runtime remains unqualified. A helper-window guard alone is insufficient.
SDL dynapi can load `SDL3_DYNAMIC_API` even in a static build; future dependency
work must prevent that override before any SDL call. Steam metadata and
gamepad mapping/VID-PID `@file` environment paths also need a bounded child
environment or fixture-private compile guards. Require an exact allowed
joystick macro/source list and resolved library/provenance before linking.
The next bounded milestone is those dependency-only protections and compile/link
recheck; the review does not grant runtime, rig or DBC-65 recovery permission.
This task delivers its conditional compile/evidence/approach-review outcome,
without treating the inert consumer assertions as passed.

## Compile result

PASS isolated SDL static build and production-consumer compile/link, exit 0.
No fixture execution. No claim of a warning-free build: GCC16.2 Release emits
`-Wstringop-overflow` at `fzero_gamepad.c:308` on the hat loop despite the
preceding `hat_count` clamp to 0..4. No demonstrated production defect or
behavior correction follows from that diagnostic; it is included for review.

The initial consumer link failed on duplicate `WinMain`: the shared compatibility
header includes SDL_main.h in both the fixture and reader translation units.
The harness sets `SDL_MAIN_NOIMPL` only for `fzero_gamepad.c`, leaving SDL's entry
point in the fixture. The corrected link passes; no production source edit.
The original failure log is retained separately from the final build log.

Exact SHA256 witnesses:

| Item | SHA256 |
|---|---|
| Original SDL CMakeLists.txt | DFEE8B830D8B80456A71BD5E91F69C2EFAC391024D8F65ADB6E8714AE694298C |
| Altered dependency CMakeLists.txt | F949BD60E365A710459E2407C591CD690A5E65153E7F6EA481B163E2F6E20E60 |
| Original copied-source manifest | 60D70E4B3F22C55D3187E4DC3FCA5C66CCE6999F24D6A1DA71FE179DF248A90E |
| libSDL3.a | 18A2E142263466CD4AF74063556597B71CA7EC869C4991F0B3212F884B781548 |
| fzero_virtual_consumer.exe | 5EEFD38AABF89027D2102C7F404C8484D0EB5FB5D8E16D6636A74A89029082A0 |

`nm --defined-only libSDL3.a` shows VIRTUAL/DUMMY joystick drivers, no physical
joystick driver symbols, and core helper-window plus device-notification
functions. The latter notification function is in an unreferenced archive member:
`nm fzero_virtual_consumer.exe` shows VIRTUAL/DUMMY and SDL_HelperWindowCreate,
but no WIN_InitDeviceNotification or physical joystick driver symbol.
Do not confuse archive membership with a reachable notification call.
Generated config explicitly defines AUDIO/VIDEO/HAPTIC/HIDAPI/SENSOR disabled.

## Impact and limits

Compile only: local CPU, two compiler jobs, scratch files; no OS focus/input,
window execution, GPU runtime, physical device access, live config/save write,
restore, installation, game load, public push, or rig lease. Fixture runtime is
not authorized by the compiled artifact. Any future run needs disposable cwd
and LOCALAPPDATA, cleared inherited SDL/FZERO/Steam metadata overrides, exact
linked dependency admission, and an inspected window/device-free startup path.
No planner tests repeated. DBC-65 preview/Apply/restore recovery gate remains
unchanged. Sandy owns the rig. No player-facing meaning changed; Darlene gate
is preserved. Ledger STD-001..040 represented; STD-019 unchecked;
STD-021/031/038 pending and STD-037 partial. No adoption promoted.

Next useful feature: execute the retained selected-wheel/cardinal/center-release
production-consumer fixture after the dependency startup boundary is resolved.
This can qualify process-local selected-device delivery, not physical driving,
force, installed behavior or game acceptance.

## Reviewed dependency follow-up (DBC-330)

The new recipe applies only to a copied SDL tree. `expected-source.json` pins
SHA256 for CMake, core, dynapi, Steam metadata and the joystick driver table.
These hashes identify this retained cache snapshot, not independently authenticated
upstream contents. Each edit requires exactly one match before replacement.
`DBCE_SDL_VIRTUAL_FIXTURE` guards helper create/destroy, rejects enabled video,
haptics or known physical drivers, disables dynapi inside its internal header,
and returns before Steam metadata initialization. Event pumping is unchanged.
Generated enabled joystick macros must be exactly DUMMY and VIRTUAL; compile
commands must contain exactly the six recorded joystick source files.

Consumer CMake reads adjacent provenance, requires runtimeAllowed=false, verifies
the archive and generated config SHA256, and compares the resolved imported
Release target to the recorded archive path before creating the consumer target.
It records that resolved path/hash in consumer-link-provenance.txt. This is
local artifact admission, not an authenticity or runtime claim. No add_test or
runtime launcher exists.

Future environment boundary (unimplemented; runtime remains forbidden): create
an explicit child environment without inherited SDL*, FZERO*, Steam* or
SteamVirtualGamepadInfo entries; supply only reviewed fixture-owned overrides.
Use disposable cwd and LOCALAPPDATA. Recheck environment before the first SDL
call. Clearing Steam metadata alone is insufficient: mapping-file and @file
hints can read files. This build does not claim those runtime prerequisites pass.

Reproduce with a fresh output:

```powershell
./tools/sdl-virtual-only/Build.ps1 -SdlSource E:/Source/games/dbce-mods-fzero-snes/build-dine-extras-graphical-796cfeb/_deps/sdl3-src -Output .local/sdl-reviewed
cmake -S tools/sdl-virtual-only -B .local/sdl-reviewed-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DSDL3_DIR:PATH="$PWD/.local/sdl-reviewed/build" -DRUNNER_ROOT:PATH=E:/Source/games/dbce-mods-fzero-snes/snesrecomp/runner -DRECOMP_UI_ROOT:PATH=E:/Source/games/dbce-mods-fzero-snes/recomp-ui
cmake --build .local/sdl-reviewed-consumer --parallel 2
```

Follow-up validation: fresh isolated SDL static build PASS (207 build steps),
existing production-consumer compile/link PASS (8 steps). Preprocessed core has
helper create/destroy declarations only, no calls; dynapi macro is zero. Existing
GCC hat-loop bounds warning remains recorded, production reader unchanged.
Runtime NOT executed. Exact hashes/link/config/source evidence are attached to
DBC-328. Next step is source recheck; no runtime retry is authorized.
