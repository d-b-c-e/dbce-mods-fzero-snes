# Shared shim: private F-Zero diagnostic

Built from the active `dbce-mods-fzero-snes` source, starting at `e77d0f99`.
This adds an **off-by-default diagnostic**, not a replacement player control
backend. No installed file, accepted tune, geometry, binding or recording was
changed. No game-window launch or physical device test was performed.

The capture-only first slice is `eca6ac4`. The follow-up below adds latched
command admission and exact private Deluxe replay identities. Neither slice
has been launched at the rig.

## First slice (eca6ac4)

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


## Latched follow-up: same mapper, no physical or network delivery

Set all three additional variables before launch to opt in:

- `FZERO_SHIM_SESSION_NONCE`: exact 8–64 letter/digit nonce.
- `FZERO_SHIM_SESSION_FILE`: absolute path to an existing `inject.on`.
- `FZERO_SHIM_BATCH_FILE`: absolute path to a distinct existing `inject.txt`.

A partial set refuses. Without these, ordinary capture-only diagnostics remain
available with keyboard guest input retained. With them, `open2` uses
`LATCHED|CAPTURE`; the shared worker validates the actual session, expiry,
nonce, ordered batches and owned devices. There is no secondary input-script
path and no synthesized action frame. Latched input suppresses physical and
keyboard guest actions for the whole admitted lane, including waiting and
terminal frames. Only Core Live values marked injected may reach the mapper.
Escape/window close remains available, but an interrupted latched run fails.
An explicit `end` batch produces one recorded neutral simulation frame and
then a normal exit. Expired/refused/failed/Inert sessions, unresolved stops and
recording failures abort the original and exit nonzero.

The following are actual code gates before command admission, not flags
asserted independently of implementation:

| Path | Diagnostic behavior |
| --- | --- |
| Native FFB initialization | Process no-force latch held in Prepare; host skips Init; Start rechecks latch. |
| UDP telemetry/motion delivery | Process delivery latch before Init; no socket or WSA startup; Frame and Shutdown cannot send. |
| Debug server | No initialization; no pause/input/save/load callback dispatch. |
| SRAM migration/read | `RtlReadSram` skipped because it can migrate/write legacy files. Fresh originals start from guest boot state. |
| SRAM final write | `RtlWriteSram` skipped. At pinned engine `59d2966`, `RtlRunFrame` has no autosave or SRAM file write callback. Guest cartridge writes stay in emulated memory. |
| Save/rewind/reset keys and gestures | Host hotkeys blocked except Escape; diagnostic actions cannot open save/rewind; scripted state modes refused. |
| Config/profile/video writes | Startup writer and launcher skipped; direct-launch video persistence skipped; missing keybinds are not generated. |
| Save directory | Always fresh `<run>/saves`, ignoring inherited save-root choice; Start checks the actual engine root including exact Deluxe/MSU suffixes. Base path limited to 68 bytes for engine fixed buffers. Only explicit original `.state` is written. |
| Engine extra callbacks | Builds with netplay, co-sim or engine external mods cannot admit this diagnostic. Current build has all three off. |

The diagnostic freezes `config.ini`, the exact `controls.ini`, and `origin.txt`
inside its fresh output. The latter distinguishes `diagnostic-injected` from
`diagnostic-owner`; the shared capture context does too. PDM and force settings
come from the frozen config. The software observation runs the existing
`FzeroFfbComputeSteering` and `FzeroTelemetryBuild` after each actual simulation
frame, with the recorded tune. Channels include the final input word, WRAM
scene state, constant fallback, spring/damper/road requests, crash trigger and
requested magnitude/duration/frequency, speed and distance. A spring coefficient
is software intent, not measured torque. No force model or accepted tune changed.
Opposing direction pairs are reduced exactly as `RtlRunFrame` does before both
recorders, so conflicted navigation still records what was consumed.

Offline evidence: 23 targeted CTest suites passed, including the production
host against a fake ABI library (latched terminal success, expired session,
missing injected provenance, partial session arguments and wrong save-root
refusal). No fake library is staged beside the game. Nine Python adapter tests
passed. The private Deluxe payload was read, reconstructed and hashed without
launching a game: `552159a19955e88a8337f7c473ccc53e5dcef15b87daab8e89e1894694542fe6`.
Graphical and matching headless builds pass. These are source/offline results.

## Bounded copy-only runtime recipe (pending source review and rig slot)

Do not run this in the Stream Deck install or overwrite it. Use a fresh short
private directory, for example `E:/fz-probe-<nonce>`, with subdirectories `app`
and `control`; leave `run` absent so Prepare can create it. Copy only runtime
assets, shaders, ROM and configs from the accepted install, the reviewed
candidate graphical/headless executables and exact shared shim. Stage the
private Deluxe `.dat` under `app/mods`. Copy music files when used, never a
writable junction back into the owner install. Exclude saves, backups and old
recordings. Hash the installed source config/video/keybinds/ROM/executable
before and after the run; keep those hashes and staged file hashes in a receipt.
Owner files, FFB tune, geometry, launcher and game selection stay unchanged.
Check `[Controls]` exists in the private config; do not invent unbound actions
or silently repurpose an SNES extra. The binding's exact device must be available.

Hold the existing shared rig lease and apply the owner-idle policy before any
launch. Clear inherited `FZERO_*` and `SNESRECOMP_*` in the **child environment**,
then supply the four shim variables plus explicit `FZERO_DELUXE_DATA` when used.
Launch `app/FZeroSNESRecomp.exe app/fzero.sfc`, without `--launcher`, with its
working directory set to the private app. Capture stdout/stderr and exact PID,
start time and executable hash. The game itself records to `run/original.fzpt`.
No `FZERO_RECORD_PLAYTHROUGH` or synthetic `SNESRECOMP_INPUT_SCRIPT` is allowed.

Immediately before launch, write `control/inject.on` as UTF-8 without BOM:

```text
nonce=<the exact nonce>
expires=<current Unix seconds plus 210>
```

Start with an empty `inject.txt`; once Core is waiting, publish batches with a
temporary sibling followed by atomic replace. The first command file is:

```text
nonce=<the exact nonce>
batch=1
inject action start 1 ms=250
```

Inspect the actual game-window frame before the next menu command; a route has
not yet been qualified and must not be described as unattended race entry.
Only supported, profile-bound actions are usable. Each new file increments
`batch`; simultaneous commands belong in the same file. For an observed race,
a first modest diagnostic segment can use `inject action throttle 0.65 ms=1000`
and `inject action steer 0.10 ms=1000`. Do not write guest poses or mutate WRAM.
Finish the short pilot with a new batch containing **only** `end` after the two
header lines. Keep the first attempt under 90 seconds. The generic ABI grammar
and limits are the pinned shim's, not the former raw F-Zero injector's grammar.

The external supervisor must have a 220-second wall deadline (the internal
240-second bound advances only when input ticks run). At deadline it requests
`CloseMainWindow` on the exact child and waits up to 10 seconds. If it remains,
report unresolved and retain the lease for explicit recovery; do not kill a
replacement, reseal evidence or mark timeout as success. Normal completion
requires exit 0, explicit Core Completed, successful recorder/host stop, no
native FFB initialization or UDP delivery, and unchanged owner pins. Finalize
the shared native capture only through the pinned P21 out-of-process finalizer
with actual process-exit evidence. An `.fzpt` complete flag alone does not seal
the shared native capture.

## Exact Deluxe replay admission

`tools/fzero_replay_adapter.py create` now accepts a Deluxe case only with all
of `--deluxe-data`, `--deluxe-data-sha256`, `--deluxe-runner` and
`--deluxe-runner-sha256`. Pins are exact reviewed candidate hashes, not a
`--skip-check` option. It validates the stock ROM identity, full BSDELX1 payload,
reconstructed Deluxe cartridge and any MSU IPS against the original tape's
effective cartridge digest. It snapshots the data and a module-identity artifact
which pins the exact capture executable, exact headless executable (the native
Deluxe module is statically linked), original ROM, data and effective ROM.
Stock cases remain compatible. No ROM, generated code, data or binary should
be committed or publicly distributed by this diagnostic work.

Place the immutable tape/state, ROM and snapshots inside one fresh private
case root. Use `--capture-receipt` only with an actual clean build receipt;
otherwise provenance remains conservatively dirty/unverified. An example,
after inserting the actual hashes and paths:

```powershell
py -3 tools/fzero_replay_adapter.py create --case E:/fz-case/case.json `
  --drive E:/fz-case/original.fzpt --state E:/fz-case/original.fzpt.state `
  --config E:/fz-case/config.ini --video E:/fz-case/fzero-video.ini `
  --rom E:/fz-case/fzero.sfc --msu-pack E:/fz-case/music `
  --capture-exe <exact-graphical-exe> --source-revision <exact-commit> `
  --case-id fzero-shim-deluxe-private-01 `
  --deluxe-data <exact-private-dat> --deluxe-data-sha256 <reviewed-sha256> `
  --deluxe-runner <exact-headless-exe> --deluxe-runner-sha256 <reviewed-sha256>
py -3 tools/fzero_replay_adapter.py observe --case E:/fz-case/case.json `
  --toolkit E:/Source/toolkits/dbce-wheel-mod-toolkit --runner <exact-headless-exe> `
  --rom E:/fz-case/fzero.sfc --output E:/fz-case/force.jsonl --timeout 300
```

Observe revalidates all pins, sets the exact Deluxe data path, isolates the
headless save root, has a bounded child deadline and rehashes the artifacts
again after replay. The actual game still has to match every WRAM checksum.
A real moving producer, shared final-consumed-word equality, cold WRAM proof,
rendered Deluxe replay and complete SNES controls remain unqualified.
