# F-Zero (SNES) fork, branch `wheelkit-runtime`: agent guide

This branch is upstream's `main` plus a loader and call sites for the Wheelkit runtime (`wheelkit.dll`, from the private
`wheelkit` repository, `runtime/native/sdk`). The rules, the plan and this game's descriptor and launch check are
there (`AGENTS.md`, `docs/plan.md`, `games/f-zero/`). Nothing of the runtime is in this repository.

- Upstream: `RetroPortingToolKit/FZeroSNESRecomp` (`mstan/FZeroSNESRecomp` redirects there), remote `upstream`. This
  branch sits on its `main` at c446305 (0.18.1, 2026-11-04). Keep it current: `git fetch upstream`, then
  `git rebase upstream/main` (ours is two source files, five call sites in `src/sdl_main.c` and one CMake block).
- Ours: remote `origin` (`d-b-c-e/dbce-mods-fzero-snes`). Push only there. Do not open pull requests or issues upstream
  from this branch.
- The fork's `main` is the old integration (346 commits on upstream 1.8.3, MSVC, the old toolkit copies). It is the
  fallback and is not merged here; see "Not carried over" below.

## What is ours on this branch

| File | What |
|---|---|
| `src/fzero_wheelkit.c`, `src/fzero_wheelkit.h` | Loads `wheelkit.dll` beside the program at run time; the wheel as player 1's controller, the F6 panel on the OpenGL frame, telemetry from WRAM, force feedback (the old model's spring, damper and road texture, which the wheel runs itself, and its collision knock) |
| `src/sdl_main.c` | Five call sites and the include (`FzeroWheelkit*`) |
| `CMakeLists.txt` | The `WHEELKIT_SDK` block. Without `-DWHEELKIT_SDK` the build is upstream's, byte for byte in source |
| `tools/build-wheelkit.ps1`, `AGENTS.md` | Build script and this page |

The submodules are upstream's own (`.gitmodules` unchanged); nothing of ours is in them.

## Toolchain (what upstream's releases are built with)

MSYS2, MINGW64 environment (msvcrt), GCC 15.2.0, Ninja, SDL3 as a DLL. On this PC it is a private MSYS2 in
`E:\Source\temp\f-zero\msys64` (not on PATH, not `C:\msys64`):

- `mingw-w64-x86_64-gcc 15.2.0-9` and `mingw-w64-x86_64-gcc-libs 15.2.0-9`, from
  `https://repo.msys2.org/mingw/mingw64/` (MSYS2's current GCC is 16.2, so these two are installed with
  `pacman -Udd`; MSYS2's own `cmake` then no longer starts, which is why CMake is the Windows one).
- From `pacman -S`: `mingw-w64-x86_64-` `ninja 1.13.2`, `sdl3 3.4.20`, `libxml2 2.15.4`, `libpng 1.6.59`,
  `giflib 6.1.3`, `rapidjson`, `libarchive`, `freetype`, `harfbuzz`, `pkgconf`, `python 3.14.10`; `binutils 2.47`,
  `crt` and `headers` 14.0.0.r598.
- CMake 4.4.3 (`C:\Program Files\CMake`). Rust's `cargo` (`%USERPROFILE%\.cargo\bin`) for the recompiler's analyzer.

Upstream's exact package revisions are not published; a build here is not byte-identical to a release.

## Build

```powershell
git submodule update --init --recursive   # in a worktree of the old checkout, add:
#   -c submodule.snesrecomp.url=https://github.com/RetroPortingToolKit/snesrecomp.git
#   -c submodule.recomp-ui.url=https://github.com/RetroPortingToolKit/recomp-ui.git
Copy-Item <your F-Zero (USA) ROM> fzero.sfc   # git-ignored; never committed, never in a build or package
# once, and after a submodule update (MSYS2 MINGW64 shell, cargo on PATH):
#   PYTHON=/mingw64/bin/python bash tools/regen.sh
pwsh tools/build-wheelkit.ps1 -WheelkitSdk <a wheelkit checkout after `pwsh build.ps1 -Only runtime/native/sdk`>
```

The script runs `cmake -S . -B E:\Source\temp\f-zero\build-wheelkit-runtime -G Ninja -DCMAKE_BUILD_TYPE=Release
-DCMAKE_PREFIX_PATH=<msys64>/mingw64 -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DWHEELKIT_SDK=<checkout>`, builds,
and copies the MinGW DLLs the program imports beside it. Without `-WheelkitSdk` (and with `-Build`) it builds the stock
upstream program the same way. Tests without a ROM: `ctest --test-dir <build folder>` with `<msys64>\mingw64\bin` on PATH
(18 pass).

## Run

`"F-Zero Forever.exe" <path to the ROM>` from the build folder (a ROM on the command line skips the launcher). Settings
and saves are beside the program. This build has no BS Deluxe module, and upstream's default car packs need it: the
build folder's `fzero-video.ini` has `CGPCars=0` and `CGPStockRebalance=0`, or the program exits at start.

Launch check, from the wheelkit checkout: `pwsh tools/launch-check/Invoke-LaunchCheck.ps1 -Game f-zero`.

## Not carried over from the fork's `main`

Deleted, because the runtime replaces them: the input shim and its host, the native wheel path, the old panel, the
telemetry sender, the WheelFfb path, recording and replay, the launcher's wheel pages, the toolkit copies. Force feedback is the old model's, through the
runtime's spring, damper and texture entries; its fallback for a wheel without a spring (a pulsed constant force) and
its separate strength settings were not carried. Still to do here, each on the runtime: the three-screen Mode 7 picture
(upstream's renderer was rewritten since), and the BS Deluxe build (private generated sources; upstream's importer
changed).
