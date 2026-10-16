# Prepared focused build route — not executed

This is a recipe for the next named serialized slot, following coordinator
source review `169195`. No CMake configure, compiler, fixture or executable
was invoked to prepare it. Game changes remain `149d914` and `1cd8b02`.

## Existing dependencies

The retained `build-dine-extras-graphical-796cfeb/CMakeCache.txt` records
Release, Ninja, MSVC 14.44.35207 x64 and SDL 3.4.10. Existing paths verified
read-only on October 16:

| Input | Exact existing route |
| --- | --- |
| Source | `E:/Source/games/.worktrees/dine-fzero-bounded-profile-20261015` |
| Canonical private inputs | `E:/Source/games/dbce-mods-fzero-snes` |
| MSVC environment | `C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat` |
| CMake / CTest | `C:/Program Files/CMake/bin/{cmake,ctest}.exe` |
| Ninja | `C:/Users/antho/AppData/Local/Microsoft/WinGet/Links/ninja.exe` |
| Framework | Canonical `snesrecomp`, clean exact `59d2966fa71d0346f92c43753e09b1ba7a1956f5` |
| Launcher | Canonical `recomp-ui`, clean exact `5abcff892f8765bc85f91077552386a3faf41552` |
| Private generated stock code | Canonical `src/gen`: 20 C files, 22 total files, 9,682,437 bytes |
| SDL package | Canonical `build-dine-extras-graphical-796cfeb/_deps/sdl3-build` |
| Reviewed private shim | `E:/Source/_archive/2026-10-14/fzero-shim-ea568c2-bf6455e6/dbce_shim.dll` |

The source worktree already contains the tracked WheelFfb DLL; it needs no
dependency staging or owner-file write. The private stock generated code
is consumed read-only through existing `FZERO_GEN_DIR`. No ROM regeneration,
Deluxe import or binary redistribution is part of this batch.

The retained SDL package exports an imported **Release static library** and
headers, so the graphical configure can reuse it without rebuilding SDL or
fetching anything. Its header target reads the existing `include-revision`
and sibling `sdl3-src/include` trees. The source archive's pinned hash is
`12b34280415ec8418c864408b93d008a20a6530687ee613d60bfbd20411f2785`.
This new consumer/import route is prepared from actual package files, not
yet configure-tested.

| Dependency file | SHA256 |
| --- | --- |
| `SDL3-static.lib` | `A643CCC19E36BEA3FC3511CE5DFF2D645FD6EE4077E3C19BEE958C9AA549F403` |
| `SDL3Config.cmake` | `12A035C0F5893F851BAF5C2DD9FF121EFD8851DDA072AA79ED9373D538B2BAAF` |
| `SDL3staticTargets.cmake` | `62CC8F8F7F5B8D56114D6CFAA3CB78FD8787C1782D3DEB3CA8248373C8BA39EC` |
| `SDL3headersTargets.cmake` | `DC608E10EAEBB5D267F878AACEF1C46CE9A1DAEE5C0E399EE994C3B704A71995` |
| Tracked `lib/toolkit/native/WheelFfb.dll` | `3F6CC51444879D01FEB900098B2DE94242594D093F523E3C34BD511D69C0D946` |
| Archived `dbce_shim.dll` | `BF6455E63696A93B37C19DABF09B1E0606F26B0534BCE828417B1DF602DD8A7A` |

## Commands after a fresh grant

Use the existing pinned `Stage-RigLease.ps1` / `Stage-OwnedProcess.ps1` /
`Stage-ProcessIdentity.ps1` custody path for the whole batch, including
compiler environment setup, both configurations and fixtures. Retain stdout,
stderr, exact process identities, input/output hashes and independent closure.
Do not execute this document or the older immutable graphical runner as a
shortcut around that handoff. Stop at the first concrete configure/build/test
failure; preserve it rather than widening or silently retrying.

Recheck the clean source/dependency commits and pins above, record the private
22-file generated-source and SDL header inventories as read dependencies, and
require both new build paths absent. Use fresh paths under this worktree;
leave every existing canonical build and dependency tree unchanged.

The following commands belong in an owned `cmd.exe /d` child with the
MSVC x64 environment initialized by the exact `vcvars64.bat` above. Variables
are local to that child. These are CMake commands, not a new build framework.

```bat
set "FZ_SRC=E:\Source\games\.worktrees\dine-fzero-bounded-profile-20261015"
set "FZ_INPUT=E:\Source\games\dbce-mods-fzero-snes"
set "FZ_OFF=%FZ_SRC%\build-counter-offline-01"
set "FZ_GUI=%FZ_SRC%\build-counter-graphical-01"
set "FZ_SDL=%FZ_INPUT%\build-dine-extras-graphical-796cfeb\_deps\sdl3-build"
set "FZ_SHIM=E:\Source\_archive\2026-10-14\fzero-shim-ea568c2-bf6455e6\dbce_shim.dll"

cmake -S "%FZ_SRC%" -B "%FZ_OFF%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DFZERO_BUILD_GAME=OFF -DBUILD_TESTING=ON
cmake --build "%FZ_OFF%" --target fzero_frame_stats_tests fzero_video_tests fzero_shim_ordinary_tests --parallel 2
ctest --test-dir "%FZ_OFF%" -R "^(fzero_frame_stats|fzero_video|fzero_shim_ordinary_(configlimit|configoversize|extrasconfiglimit|extrasconfigoversize|normal|extras|extrasbom))$" --output-on-failure

cmake -S "%FZ_SRC%" -B "%FZ_GUI%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DFZERO_BUILD_GAME=ON -DBUILD_TESTING=ON "-DSNESRECOMP_ROOT=%FZ_INPUT%/snesrecomp" "-DRECOMP_UI_ROOT=%FZ_INPUT%/recomp-ui" "-DFZERO_GEN_DIR=%FZ_INPUT%/src/gen" -DFZERO_DELUXE_GEN_DIR= -DFZERO_DELUXE_DATA_FILE= "-DFZERO_SHIM_DLL=%FZ_SHIM%" -DSNESRECOMP_SDL_BACKEND=SDL3 "-DSDL3_DIR=%FZ_SDL%" -DSNESRECOMP_SDL3_FETCH=OFF
cmake --build "%FZ_GUI%" --target FZeroSNESRecomp fzero_mods_tests --parallel 2
ctest --test-dir "%FZ_GUI%" -R "^fzero_mods$" --output-on-failure
```

Expected focused set: **nine ROM-free cases plus one provider case**. Each
command must exit zero with the intended tests actually selected; retain the
specific compiler warning status. The desktop target stages its normal
dependencies/assets only inside the fresh graphical build directory. It is
not launched. No headless target, broad test suite, native toolkit rebuild,
new package, install or display change is required for this source check.

Graphical compilation checks both conditional presentation branches against
the configured SDL3 compatibility layer. It does not prove counter pixels,
center placement, scanout or physical acceptance. Those remain a later named
bounded observation using an isolated copy and the accepted muted harness.
