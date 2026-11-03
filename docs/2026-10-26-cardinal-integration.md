# DBC-322 isolated source integration

The requested composition already exists in exact ancestry. No cherry-pick,
merge, duplicate fix or new test is needed. Isolated review branch:
`egon/fzero-cardinal-integration-20261026`, rooted at
`0df0e81f86799954cedfc47ac6ab4a7f8bcb90fa`.

| Commit | Parent | Purpose |
| --- | --- | --- |
| 83b678be061d3f08d55eb5b1985072dde4c824dd | b61e413980f80a688de5b82b4406ca7468f0c712 | Instance/product correction |
| 607157220817eed722fef8a4610b1eb52ba881a8 | 83b678be061d3f08d55eb5b1985072dde4c824dd | Reviewed product-refusal wording |
| f28453aaaa7dceeaeb4be11f09f8299ffdd3f621 | 607157220817eed722fef8a4610b1eb52ba881a8 | Producer/helper reconciliation |
| 0df0e81f86799954cedfc47ac6ab4a7f8bcb90fa | f28453aaaa7dceeaeb4be11f09f8299ffdd3f621 | Cardinal/neutral coverage and CMake include wiring |

Current primary main is a03c722; it is an ancestor, 36 commits behind the
candidate. The retained integration base b61e413 is 32 commits ahead of main.
Review `b61e413..0df0e81` for this bounded change, not an implied promotion of
all 36 retained commits. The bounded diff contains CMakeLists.txt,
TOOLKIT-ADOPTION.md, docs/2026-10-26-cardinal-consumer-contract.md,
src/fzero_controls.cpp and tests/test_controls_contract.cpp.
All original branches and worktrees remain intact.

## Existing build and consumer checks

Fresh isolated Ninja/GCC 16.2 build on October 26:

```powershell
cmake -S . -B .local/contract-build -G Ninja -DFZERO_BUILD_GAME=OFF -DRECOMP_UI_ROOT=E:/Source/games/dbce-mods-fzero-snes/recomp-ui
cmake --build .local/contract-build --target fzero_controls_contract_tests
ctest --test-dir .local/contract-build -R '^fzero_controls_contract$' -V
```

PASS: 207 vectors, 1,534 checks, zero failures, CTest 1/1. This target includes
the actual production planner and actual raw-hat launcher decoder, including
identity/refusal coverage. No tests were added for this integration milestone.
The launcher checkout is clean at pinned
6d9b71526f82c0182f85ec47610eb559303a77b8.

The production SDL consumer translation unit also compiles successfully:

```powershell
gcc -std=c11 -DSNESRECOMP_SDL3=1 -DLNG_SDL3=1 -Isrc -IE:/Source/games/dbce-mods-fzero-snes/snesrecomp/runner/src -IE:/Source/games/dbce-mods-fzero-snes/recomp-ui/src/common -IE:/Source/games/dbce-mods-fzero-snes/build-dine-extras-graphical-796cfeb/_deps/sdl3-src/include -c src/fzero_gamepad.c -o .local/contract-build/fzero_gamepad-production.obj
```

Framework checkout is clean at the pinned
59d2966fa71d0346f92c43753e09b1ba7a1956f5. Cached SDL headers identify 3.4.10.
This is compile integration only, not full game linking or execution of the
SDL loop. The existing fzero_gamepad test attaches virtual controllers and
was deliberately not executed under the task's no-input restriction.
Scratch build outputs remain under .local/contract-build, outside the commit.

No source conflict or additional product defect was found. Standards ledger
still has 40 represented IDs; STD-019 unchecked, STD-021/031 pending and
STD-037 partial remain unchanged. No changed player-facing text requires a
new Darlene review. This document is the only new tracked change.

## Independent review boundary

Sandy can review the exact bounded diff above plus this integration record.
Source review is the next useful outcome; no review verdict is claimed here.
DBC-65's blocked preview/Apply/restore is preserved, neither retried nor
replaced. No live config/save, install, package, game launch, input, physical
acceptance or public push occurred. No Wheelkit source was edited.
