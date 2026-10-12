# Shared shim: private F-Zero diagnostic

Built from the active `dbce-mods-fzero-snes` source, starting at `e77d0f99`.
This adds an **off-by-default diagnostic**, not a replacement player control
backend. No installed file, accepted tune, geometry, binding or recording was
changed. Private title/attract recording and cold replay are now checked below;
no physical force test or player-controlled race is claimed.

The capture-only first slice is `eca6ac4`. The follow-up adds latched command
admission and exact private Deluxe replay identities. Its first private
title/attract runtime evidence is recorded below.

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

This first-slice plan is retained for context. The **staged first latched pilot**
below supersedes its ordinary-launch and capture-only steps; no extra baseline
launch or private tune rewrite is needed for that pilot.

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

Start with the neutral batch below; once Core is ready, publish batches with a
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

## Reviewed private-copy wrapper

`tools/Invoke-ShimDiagnosticCopy.ps1` prepares and supervises one fresh private
copy. The fresh retry is `E:/fz-probe-1012b`, with 70 app files, 16 copied
helper files and 77 original-install file/absence pins in `prepared.json`.
The installed config, controls, video geometry, FFB tuning and music are copied
unchanged. Saves and old recordings are excluded. The private Deluxe data is
copied from the source archive; the installed build embeds its own data and
does not have that file. Nothing is written in the owner install.

Candidate binaries remain the clean `4f43f1a7a2224d06c4d3020a624f371dd89d2579`
build, tree `3561a32c32deaae879ded0f435746e8c49fd4615`:

| File | SHA-256 |
| --- | --- |
| FZeroSNESRecomp.exe | `0a37843731a672e305040ef673fc4c7618ce81f61637017ba54b55f840b2cef5` |
| FZeroSNESRecompHeadless.exe | `e55c48f39cc43fb5c9d7a832698d97a67a24e4ad41f7a8a5d740426574e01103` |
| dbce_shim.dll | `9b8a036f664cf8f45cccf82b98e5e4376f93215b60eac50b6db965bf498015d8` |
| WheelFfb.dll | `3f6cc51444879d01feb900098b2de94242594d093f523e3c34bd511d69c0d946` |
| mods/bs-deluxe.dat | `a9e31fd3a32103cc6591e70c31e4a182b631989b7ab31b21cb5aec6c5c1bc02b` |
| P21Finalizer.dll | `dbc88857794be0b6f1ce5afa3c356497533abe4b2f43578cf57f71f4acb4d08e` |

After the coordinator grants the rig slot, start **Run in a separate supervised
PowerShell 7 process/tool session**, so the reviewer can capture and inspect
frames while it runs:

```powershell
$runner = 'E:/Source/games/.worktrees/fzero-shim-source/tools/Invoke-ShimDiagnosticCopy.ps1'
$probe = 'E:/fz-probe-1012b'
pwsh -NoProfile -File $runner -Action Check -Probe $probe # read-only
pwsh -NoProfile -File $runner -Action Run -Probe $probe
```

Run requires five owner-idle minutes and acquires the shared lease itself. It
never changes display topology or sends OS input. `process.json` identifies the
exact child. The run starts with neutral batch 1 (`inject action start 0 ms=50`); **no menu route is built
in**. In the reviewer's other tool session:

```powershell
& $runner -Action Status -Probe $probe
& $runner -Action Capture -Probe $probe # lists this child's windows
& $runner -Action Capture -Probe $probe -WindowHandle <centre-window-decimal-handle>
# Inspect the returned PNG before sending one scene-appropriate action:
& $runner -Action Command -Probe $probe -Commands 'inject action start 1 ms=250' `
  -InspectedFrame '<returned-PNG-path>'
# Repeat Capture + visual inspection before each subsequent action.
# Only after a race is actually visible, a bounded moving diagnostic may use:
& $runner -Action Command -Probe $probe `
  -Commands @('inject action throttle 0.65 ms=1000','inject action steer 0.10 ms=1000') `
  -InspectedFrame '<fresh-returned-PNG-path>'
& $runner -Action Command -Probe $probe -Commands 'end'
```

Command admission requires the same leased child and a capture receipt less
than 30 seconds old. It checks the PNG hash, PID and creation time, bounds each
command to three seconds, and publishes one nonce-matched monotonic batch by
atomic rename. Commands go only to production Core's existing action injector,
then through the production F-Zero mapper/PDM and actual emulated game frames.
There is no synthetic guest-input or WRAM write path in this runner.

Run sends only `end` automatically at 75 seconds. At 90 seconds it requests
normal window close from the exact child if needed; at 100 seconds an unresolved
child is reported and its lease retained. The outer budget is 220 seconds,
including bounded journal validation and P21 helpers. A helper timeout also
retains its identity and the lease; neither a helper nor game is killed. The
coordinator must resolve such a process before releasing that lease. Nonzero
exit, missing Completed, unresolved host or rejected P21 is a failed pilot.
Sealing happens only after actual observed exit 0, valid Completed journal,
resolved host and unchanged owner pins. `result.json` still labels
`runtimeQualified=false`: a seal alone does not establish moving gameplay or
cold WRAM equality. Finalizer validation output is `validated.tsv`; raw original
and initial WRAM are `run/original.fzpt` and `run/original.fzpt.state`.

Keyboard audit: the diagnostic KEYDOWN guard runs before save/rewind, pause,
reset, F-key save/load, video/FPS and fullscreen shortcuts. The latched guest
keyboard lane is also suppressed. **Escape and window-close are deliberate
emergency exits**; an unfinished latched run then fails rather than becoming a
successful recording.

Offline wrapper validation (`tests/Test-ShimDiagnosticCopy.ps1 -Probe ...`)
exercised production Check: exact pins pass, changed private config and helper
bytes refuse, an extra runtime file refuses, and restored bytes pass. Harmless
PowerShell helper children verified normal output and timeout/identity retention.
No game, device, lease or desktop input was involved. The copied finalizer also
read an already sealed shared fake-provider fixture: five rows, 217 checks.
That fixture result is tool compatibility evidence, not F-Zero runtime evidence.

### First pilot and neutral-start correction

Private pilot `E:/fz-probe-1012a` launched PID 77464 and exited 9 after two
emulated frames. The exact R12 was admitted in epoch 1 with 11 usable bindings,
but Core correctly refused the existing empty `inject.txt`. The invalid original
and unresolved host report remain untouched and unsealed; no menu action was
sent. The supervisor observed exit and rechecked all owner pins before releasing
the lease. Current 2560x1440 display produced the logged stock-view fallback;
no display or geometry was changed.

The wrapper now publishes nonce-matched batch 1 with only
`inject action start 0 ms=50`, then manual commands start at batch 2. The actual
pinned production parser requires at least 50 ms (a 1 ms proposal was refused by
the offline fixture before retry). `tests/shim_neutral_batch.cpp`, linked against
f49cfc5 production Core/parse and its fake provider, consumes the wrapper's exact
emitted bytes. Nine checks show admission with a physically held fake Start,
zero value and no down/up edge on activation and expiry, a working press/release
positive control, and explicit Completed. No DirectInput or game is in that test.
The existing wrapper test can run it using `-NativeCoreFixture <neutral.exe>`.
Private compile evidence is `E:/fz-neutral-check` (build.cmd, neutral.txt, exe).

## Private runtime result: 1012b, 2026-10-12

**PASS for the title/attract diagnostic and cold game replay only.** Graphical
PID 73552 started at 08:57:48 UTC from the exact reviewed `4f43f1a` executable.
The wrapper was `b35e100`; native runtime bytes did not change for the retry.
It opened the actual R12 through Core, epoch 1, 11 usable bindings. Initial batch
1 held Start at zero for 50 ms. The observed title offered Grand Prix/Practice;
by the next frame the game had entered its attract demo. An inspected NavDown
100 ms and Start 150 ms reached Core and the final game input. No throttle was
sent and **no player-controlled race entry is established**. The final capture
after Start was a black transition. Automatic batch 4 ended at the 75-second
bound; the process exited 0, without forced close, and the host resolved.

Evidence is private at `E:/fz-probe-1012b`:

- `result.json`: exit observed, code 0, no errors, sealed, owner pins unchanged,
  lease not retained; all 77 original-install file/absence pins checked.
- `frames/085806781.png`, `085822205.png`, `085844507.png`: title, attract demo,
  and transition respectively; each published user action has a recent exact
  child/capture receipt. The menu timed out between screenshots, so these
  commands are not a qualified navigation recipe.
- `journal-result.json`: valid complete journal, exactly one terminal event,
  `completed: end command in batch 4`. `run/host-shutdown.json` reports resolved,
  recorder and control worker joined, input released, no unresolved entries.
- `seal.log`: P21 validated 4,467 shared rows with 71,751 checks. A later strict
  read after cold replay passed another 35,879 checks; the seal stayed intact.
- `consumed-input-check.json`: **4,466 valid game rows match every simulation
  index and final input word in `.fzpt` exactly**, zero mismatches. The extra
  shared row is the explicit shutdown tick with `game.valid=0`; it is not an
  emulated frame. Recorded words include seven NavDown frames and nine Start
  frames, with the remaining 4,450 carrying no guest control button.
- `runtime-observed.json`: during the owned process, WheelFfb was absent from
  its module list and the process had no UDP endpoint. Admission required the
  actual no-force and socket-absent latches; origin records software-only force
  and telemetry. This is not a numeric delivery-call counter or torque test.
- The source recording and state are unchanged after cold replay:
  `.fzpt` SHA-256 `9935ba483eb61b284a60a6509dcbc5829a835127352ec03ac85ea15f8bbd4d26`;
  state SHA-256 `18bda07046cd36c3dceca4ca26d38fa0d7317ed325ff29390af34c54dc8042db`.
  Shared source SHA-256 is
  `d9fb661c2445852fd05f2625e878cbfa626ef2e3c301138b41acc43950c1fae3`.

Cold case `E:/fz-cold-1012b/case.json` copies the immutable original, state,
config, video, stock ROM and music, pins the clean capture build receipt and
exact Deluxe data/headless executable, and reconstructs the MSU-patched cartridge
identity `419eed641cfa20013d1e9b8270062d90155051a441441a2118bf23846ac8d747`.
`fzero_replay_adapter.py observe --timeout 60` exited 0 and verified **all 4,466
per-frame WRAM checksums**, producing `force.jsonl`. Case SHA-256:
`dd0af71ad733de8c26f984eccdaba2f494d76fa868bd369f9e05484d38b39c8d`.
Both graphical and headless processes were gone and the lease absent before
the explicit handoff to the next rig test.

Remaining qualification: a fresh scene-inspected original that actually enters
Practice and applies short throttle/steering while the player is in control,
then the same cold equality checks. Independent SNES X/Y/L/R remain unsupported
by generic ABI 2. The current desktop was 2560x1440; the accepted span-only triple
setting deliberately fell back to stock view. This run does not qualify triples,
physical wheel feel, FFB normalization, complete controls, or a public release.
