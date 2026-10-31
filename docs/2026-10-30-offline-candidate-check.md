# Offline candidate, recording and replay check — 2026-10-30 (vemi)

No game window, launcher, desktop input, wheel, force or Steam. The owner install was not touched.

## Candidate (clean-install plan step 1)

Source `9dc40e9` (curved center warp) in a clean worktree, submodules at the pinned commits, private generated
sources read from the canonical checkout (`-DFZERO_GEN_DIR`; never copied). Built with the
`docs/UNIFIED-PRODUCT.md` recipe: `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`, no Deluxe. The run plan's build
line omitted the static runtime; a plain `/MD` build stages nothing (`Runtime import absent from build:
vcruntime140.dll`), as intended.

- `stage_unified.py` (stock-only), `package_unified.py`, `--verify`: pass.
  `DBCE-FZeroSNES-unified-preview-1.8.3-g9dc40e99f5d3-windows-x64.zip`, 5,472,631 bytes, SHA-256
  `99566EA51E556C81FC0A727F80DEFE98432A928C6F5B8CDB9C3DFDD82F1A49D1` (kept under `E:\Source\temp\fzero\candidate-2b088a0`).
- 64 files; no ROM, `.ini` or `.cfg`. Imports: the game, headless runner and WheelFfb.dll load only Windows system
  DLLs (dumpbin), so no Visual C++ runtime install is needed.
- `tests/test_unified_package.py`: 35 pass. ctest on the `/MD` build: 82/82. On the static build 74/82: the eight
  `fzero_shim_host_*`/`fzero_shim_ordinary_*` tests set variables with `_putenv_s`, which a statically linked fake
  shim DLL cannot see (separate CRT environment). Harness limit only; the stock package ships no shim.
- Fixed on main first (2b088a0): `fzero_mods` still expected 7 triple options after c587f1b added
  `TripleCurveRadiusMm`.

Not done: steps 2-7 of the run plan (Setup.cmd, ROM selection, restart, removal); they need the shared slot.

## Recording and replay (drive_rec / drive_play)

Headless runner from the same source (`/MD` build, `FZeroSNESRecompHeadless.exe` SHA-256 prefix `B61CB3D7E490003C`),
stock ROM read in place from the sealed case `fzero-stock-synthetic-20261008-0346`, on copies in
`E:\Source\temp\fzero-replay-check`:

- Replay of the October 8 tape: exit 0, `PASS frames=3600` (replay exits nonzero on any WRAM checkpoint mismatch);
  the tape copy is unchanged.
- Fresh recording with the same scripted input, then its replay: both exit 0, 3,600 frames. The new `.fzpt` and
  `.fzpt.state` are byte-identical to the October 8 pair, so recording is deterministic across three weeks of
  changes.

Synthetic scripted input only: not an owner drive, physical binding, force or visual acceptance.
