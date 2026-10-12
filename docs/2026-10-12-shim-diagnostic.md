# Shared shim: first private F-Zero source slice

Built from the active `dbce-mods-fzero-snes` source, starting at `e77d0f99`.
This adds an **off-by-default diagnostic**, not a replacement player control
backend. No installed file, accepted tune, geometry, binding or recording was
changed. No game-window launch or physical device test was performed.

## What this slice does

- `FZERO_SHIM_DIAGNOSTIC_RUN` must name a fresh absolute run directory. Without
  it the existing launch/input/FFB path is unchanged and the shared DLL is never
  loaded. A diagnostic requires an explicit ROM argument and refuses the
  launcher, other recording/replay modes and state/viewport/overlay scripts.
- A diagnostic request latches physical force off before admission, launcher
  code or FFB initialization. Even a refused request cannot fall into normal
  force-enabled play. The existing process latch is reused; force settings and
  force-model files are untouched.
- The shared ABI 2 provider owns controller reading. The diagnostic does not
  initialize SDL's controller subsystem, open an SDL pad/wheel, run the old raw
  test-injection reader, or merge debug-server input. Keyboard guest bindings
  are retained. Host save/rewind/reset/video hotkeys are excluded; Escape closes.
- The exact `[Controls]` bytes are copied to `controls.ini` in the new run.
  Core owns identity, per-device resolution, calibration, availability and
  rearm. There is no DI-to-SDL product/name join. No game config is rewritten.
- `fzero_shim_map.c` consumes one shared frame per **emulated simulation frame**,
  independent of presentation FPS. Finite Live values alone reach the game.
  Missing/stale/rearming input is neutral, with no fallback to old wheel input;
  duplicate/out-of-order Core frames refuse instead of advancing steering twice.
- Steering reuses `FzeroAnalogSteeringReadTuned`: Core supplies calibrated
  normalized steering, then F-Zero's existing range/response and digital pulse
  modulator apply. Core's deadzone is not applied a second time. Range/response
  and pedal threshold are read from the selected existing Controller section;
  nothing is written. This is not yet an owner-accepted feel comparison.
- Basic controls are steer, throttle, brake, confirm, back, start, select and
  four navigation directions. The other generic actions have explicit reasons
  in the native unsupported inventory. Throttle maps to SNES B, brake to Y,
  confirm to A and back to B, matching the existing reader. **ABI 2 has no
  independent SNES X/Y/L/R, save or rewind binding actions.** No unrelated
  racing action is repurposed for them. The
  diagnostic must not be advertised as complete F-Zero wheel controls.
- Both recordings use the **final consumed input**: `original.fzpt` plus its
  initial state and WRAM checks, and the shared capture's `game.inputWord` and
  `game.simulationFrame` after `RtlRunFrame`. A pending input must be consumed
  before the next tick. Record/stop failures exit nonzero and do not finalize a
  successful original. The diagnostic suppresses final SRAM writes.
- The first slice opens **capture only**, not ABI 2's latched command worker.
  It cannot yet perform an unattended injected drive. The capture is bounded
  to 240 seconds (the native recorder's maximum is 300), checked on input ticks;
  a runtime wrapper must independently bound a minimized/suspended process.

The recorded-playback startup fix is separate and narrow: neither synthetic
nor `.fzpt` playback runs the pending controls-profile writer or legacy test
injection initializer. Use an explicit ROM without `--launcher` for read-only
playback; interactive launcher edits are not blocked by this change.

## Offline validation and build

The ROM-free `fzero_shim_map` test compares the production PDM at every frame,
checks every unavailable status and finite-value handling, exercises monotonic
sequence admission, and writes/replays 120 final consumed words through the
existing `.fzpt` codec with keyboard bits included.

The eight `fzero_shim_host_*` tests compile the **production host** against a
separate fake ABI DLL. They check off/default behavior, pre-launch no-force
latching, preserved config bytes, exact profile handoff, ABI refusal, missing
consumption, refused record and unresolved stop. The fake has no device code
and lives in `build-*/shim-fixture`, away from the game DLL.

Validation here: all **13 targeted CTest suites passed**, including 783 mapper
checks, 132 production-host checks and the existing controller, profile
(187 checks), analog and recording suites. Private graphical and matching
headless Deluxe builds both succeeded. The desktop translation unit retains
the existing `HIBYTE` macro-redefinition warning between Windows and runner
headers; no force/geometry source was changed to address it in this slice.

```powershell
cmake -S . -B build-shim-unit -G Ninja -DFZERO_BUILD_GAME=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-shim-unit --target fzero_shim_map_tests fzero_shim_host_tests
ctest --test-dir build-shim-unit -R fzero_shim --output-on-failure
```

For the private game candidate, use the existing pinned SNESRecomp/recomp-ui
and private generated stock/Deluxe sources. Add
`-DFZERO_SHIM_DLL=<reviewed f49cfc5 DLL>` to the normal configure command.
CMake checks the DLL's exact SHA-256. The default is empty: normal builds and
public package allowlists do not gain a provisional binary dependency.
See `vendor/dbce-shim/PROVENANCE.md`.

## Next private muted Deluxe qualification

1. Stage a **fresh private copy**, retaining the accepted config/video/keyboard
   settings and private ROM/music/save inputs. Use this candidate graphical
   executable, matching headless executable, existing WheelFfb and the pinned
   shim. Pin source tree, both executable hashes, ROM/effective-cartridge
   identity, generated Deluxe module/data, MSU patch when used and viewport.
   Do not overwrite the installed Stream Deck copy.
2. First inspect an ordinary no-shim launch of that copy with force muted in
   the copy, then an opt-in short at-rest shim capture. Use the shared rig
   lease and owner-idle policy. The child environment is:
   `FZERO_SHIM_DIAGNOSTIC_RUN=<fresh absolute output>` and
   `SNESRECOMP_SAVE_ROOT=<private save directory>`. Launch
   `FZeroSNESRecomp.exe <private fzero.sfc>` without `--launcher`.
   Do not also set `FZERO_RECORD_PLAYTHROUGH`; the diagnostic owns
   `<run>/original.fzpt` and `<run>/original.fzpt.state`.
3. Verify no native force initialization, expected device/action availability,
   no SDL controller lane, exact config preservation and normal exit. The
   shared native capture requires its existing out-of-process P21 finalizer
   with the observed process-exit evidence; do not infer a seal from exit 0.
4. For a moving original, either drive attended with force muted, or add the
   shared latched command-session admission as the next source slice, using
   this no-force/save fence. A synthetic input script bypasses the mapper and
   cannot qualify shim delivery. Missing SNES extras remain a separate scope.
5. Add separately pinned Deluxe data to `tools/fzero_replay_adapter.py` rather
   than removing its current refusal. Keep the tape/state immutable. Cold
   headless replay must match every WRAM checksum; cold rendered replay must
   use the same effective cartridge, snapshot and centre viewport. Compare
   each `.fzpt` input word with `game.inputWord` at the same simulation frame.
   Then use the existing device-free force model export; a shared capture or
   condition coefficient alone is not physical FFB normalization evidence.

No Deluxe replay, full action coverage, rendered validation, physical wheel
feel, Wheelkit Apply closed loop, release capability or public distribution
claim follows from these source/offline checks.
