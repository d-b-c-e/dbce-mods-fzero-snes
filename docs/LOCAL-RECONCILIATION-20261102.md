# Local branch reconciliation, November 2, 2026

Local source baseline: main `6950c26`; isolated worktree
`E:\Source\_work\soba-fzero-reconcile` (`soba/fzero-reconcile`).
Public origin remains untouched. No game, owner files, ROM, device, or installation
was opened or changed. This is source/fixture evidence, not a playable release.

## Retained branches and disposition

- `egon/fzero-cardinal-integration-20261026` at `8a23fcf`: bounded source port
  `39eef84`, using the three-way delta from shared base `a03c722` for CMake,
  README, toolkit ledger, docs, src, tests and tools only. Its 4,366 tracked
  `.local/` build/dependency files are excluded. Original branch ref retained.
  Current main's F6 ledger and F7 `g_compare_view != 1` behavior are preserved;
  the incoming unsupported-span warning is retained behind that F7 condition.
- `lamo/fzero-deluxe-telemetry-integration-20261023` at `35b7607`: its source
  history is covered by the above port. The final exact-inventory exception for
  the private helper's format-recognition literal is present, including its
  tamper/unlisted-file regression. Cherry-pick was empty and skipped. No ancestry
  merge or private helper binaries admitted; ref retained.
- `dine/fzero-bounded-profile-20261015` at `43acdd6`: intentionally unmerged.
  Its only 54 changed paths are `private-controls-stage-01/` payload artifacts;
  there is no independent product-source delta to port. Ref retained.

`/.local/` is ignored for this worktree's disposable native build/logs. HEAD has
zero tracked `.local/` or `private-controls-stage*` entries. No original branch,
worktree, package, rollback artifact or private payload was deleted.

## Focused validation

Coordinator GO #497896; all build leases released (final lease
`9e57ca0680cb41289e805b7674aaeba7`). ROM-free configuration:

```
cmake -S . -B .local/reconcile/build -G "MinGW Makefiles" -DFZERO_BUILD_GAME=OFF -DBUILD_TESTING=ON -DRECOMP_UI_ROOT=E:/Source/game-modding/dbce-mods-fzero-snes/recomp-ui
cmake --build .local/reconcile/build -j 4
ctest --test-dir .local/reconcile/build --output-on-failure
python -m unittest discover -s tests -p test_unified_package.py
python -m unittest discover -s tests -p test_setup_bundle.py
```

Existing primary recomp-ui headers were read only; submodules were not reset or
initialized. GCC 16 defaults to C++20, exposing the existing fixture's char8_t
mismatch. CMake now explicitly selects C++17. MinGW also prefixed the fixture DLL
with `lib`; the fake-shim target now emits exact `dbce_shim.dll` on Windows.
Initial 52/77 native tests passed; all 25 naming-related failures passed after
that target correction. Combined result: 77/77 native cases. Controls contract:
207 vectors, 1,534 checks, zero failures. Python: 41 unified-package + 10 setup
bundle tests passed. Warnings about existing NOMINMAX redefinitions remain.

Evidence retained under `.local/reconcile/`: initial/final configure and build
logs, `ctest.log`, `ctest-rerun.log`, `python-unified.log`, `python-setup.log`.
Device-free fixture identities (not distributable player candidates):

- controls contract EXE SHA256 `9FFDD9EAEA61C1A6EB73A0F8187434A090E01E6C5FEA08748C5AEC092012FF2A`
- fake shim DLL SHA256 `1AA1C7896AC7D0133D49AB82B450FE5288C54C77FE9EE13F5759AB6FA39A1C50`

## Remaining gates

The full game/SDL hosts and ROM-dependent targets were not built. No player ZIP,
install, game launch, current-device cardinal input, telemetry in a race, FFB,
three-view gameplay or owner acceptance is established. Build a fresh private
candidate using the qualified current dependency/private-input receipts before
runtime qualification. Public distribution remains subject to
`docs/PUBLIC-PACKAGING.md`; this reconciliation grants no publication clearance.
