# Ordinary shared input candidate

The corrected `7187c57` private build passed ordinary Shared startup, a 900-frame
recording, and a fresh-process graphical replay with all 900 WRAM checkpoints
exact. REC/Saved/Replay/Verified and the missing-wheel-extras warning were seen.
This is a title/menu test with the wheel at rest, not a driven controls or release
qualification. Native remains the default; no owner install, settings, FFB model
or geometry changed.

The earlier `9790788` run failed inside SDL device acquisition. Its incomplete
original and diagnostic dump remain unchanged. Do not deploy or retry that path.
The corrected path uses Core plus keyboard and explicitly lacks legacy wheel
extras, launcher integration and the force/read ownership bridge.

## Opt-in and ownership

In a private candidate copy only:

```ini
[Input]
Backend=Shared

[ForceFeedback]
Enabled=0
```

Missing `Backend` means `Native`, which retains the existing controller, profile
startup writer and force lifecycle. Unknown backend values refuse startup rather
than silently choosing one. Shared takes the exact existing `[Controls]` bytes
from `config.ini` and passes them to the same pinned compiled Core. Core validates
the profile. Shared skips the legacy profile-to-controller writer; it does not
rewrite `[Controller.*]`, `[ControlsApplied]` or the user's settings.

**Shared plus FFB enabled currently refuses.** The legacy `FzeroFfbInit` opens its
own exclusive force device, while Core already owns its input device. That
combination has no qualified ownership bridge. Refusal occurs before startup
outputs, and the setting is checked again after the launcher. An admitted Shared
process holds the no-force latch and bypasses legacy force initialization; a later
force initialization request cannot open a second owner. No stored setting or gain
is changed. Native remains the way to play with the existing force support.

This is a temporary implementation gap, not a desired permanent restriction on
ordinary shared input.

## Input and recording boundary

| Mode | Input | Saves/telemetry | Recording |
|---|---|---|---|
| Native default | Existing SDL/controller and keyboard | Existing behavior | Optional ordinary `.fzpt` |
| Ordinary Shared | Core actions and keyboard; legacy wheel extras explicitly unavailable | Ordinary SRAM/host settings and configured telemetry; FFB enabled is refused | Optional `.fzpt`; no shared diagnostic recorder |
| Capture-only preview | Core actions and keyboard | Private saves, no force/UDP delivery | Mandatory `.fzpt` plus shared computed-signal capture, bounded |
| Latched diagnostic | Core's journalled injected actions; keyboard suppressed | Private saves, no force/UDP delivery | Same pair with injected provenance and completed terminal required |
| Recorded replay | Final input words from tape only | No input/force owner, UDP, config/profile or SRAM writes | Verify every recorded WRAM checkpoint |

Ordinary Shared calls ABI2 `open2` with flags **0**. It has no injection files,
control worker, diagnostic capture, 240-second deadline or save redirect. A small
unique folder under `diagnostics/` receives the shared host's shutdown report.
An explicitly supplied `.fzpt` is still optional, using the existing CLI/recorder.

At each emulated gameplay frame, Core runs once and the existing steering PDM
maps its normalized steering. The saved native range/response/pedal threshold
still shape this game adapter. Keyboard bits are merged with Core before the normal host filters. Opposing directions are
reduced using the guest's rule, then the **same final word** goes to `RtlRunFrame`
and the existing `.fzpt` writer. No parallel recording container is added.
The `.fzpt` snapshot/hash path can reconstruct software force/telemetry through
the existing replay adapter; this ordinary slice does not add a live shared
computed-signal trace. Preview retains that trace for diagnostic comparisons.

Ctrl+F11 stops an ordinary recording without stopping Core/gameplay. Save/load,
reset and state/rewind interactions keep their existing recording-invalidating
rules. Opening a host overlay cancels an unconsumed gameplay tick and resets the
PDM. Overlay polls accept Core's digital navigation without advancing PDM or
creating a guest recording row; pedals do not become overlay Confirm/Back.
Recorded replay bypasses both input backends and host gestures before reading the
tape, even if its config selects Shared.

## Temporary missing wheel extras

The first ordinary slice tried preserving native digital bindings through SDL.
The failed runtime below showed why even a digital-only reader cannot open an
independent SDL wheel. Current source does not initialize the SDL controller
subsystem, refresh/open a pad, process gamepad hotplug events (including inside
an overlay), or call the SDL supplemental reader in Shared. Native remains
unchanged. Replay likewise never initializes a controller.

ABI2 has no independent SNES X/Y/L/R or host save/rewind actions. These **wheel**
bindings are explicitly unavailable in this opt-in interim backend, including
legacy generic buttons when the Core action is unbound. No saved bindings are
deleted or reinterpreted. Native is still the complete existing wheel path;
keyboard SNES/host controls remain available in ordinary Shared. A persistent
on-screen warning and the startup log disclose the missing wheel extras.

This private candidate temporarily requires a direct ROM path without the
launcher: the existing launcher's steering preview can also open SDL devices.
This is an integration gap to remove when the launcher shares the input owner,
not a desired permanent user restriction.

An additive optional raw-digital observation API is a possible follow-up. Core
already retains `Frame::samples` (device index/epoch/read sequence/time, validity,
physical and delivered values), but the pinned ABI2 only exposes mapped actions
and identity reports. The new API would copy one exact last-tick sample, never
poll/open again, with frame sequence and epoch checks. Exposing that API alone
does not prove which old SDL controller section belongs to that Core instance;
that identity/binding migration must be exact rather than product/name matching.
Coordinate this with the shared provider/force bridge instead of opening another
handle or extending the ABI independently.

## Offline validation and next runtime slice

The actual `fzero_shim.cpp` host runs against a fake ABI DLL, with no device or
UDP transport. Fixtures cover default Native, replay bypass, ordinary flags0,
exact profile bytes, no capture/deadline arguments, no save-root/recording path,
initial and launcher-time Shared+FFB refusal, actual force-latch refusal,
malformed Core profile, stray injection request and an unexpected latched frame.
They also cover canceled overlay ticks, no legacy fallback for missing or unbound buttons,
real `.fzpt` consumed-word equality and shutdown. The SDL virtual-wheel fixture
proves held steering/pedals do not leak into the supplement while saved SNES and
host buttons still work. Existing preview, latched capture, session UI and codec
fixtures remain in the targeted suite.

Validation: graphical and headless targets build; **27 targeted CTests pass**,
including 124 checks across ten ordinary-host cases, 827 mapper checks, 108
session-UI/recorder checks and the actual SDL virtual-wheel fixture. The build's
only warning is the existing `HIBYTE` macro redefinition. Run the scope with:

```powershell
ctest --test-dir build-shim-game -R 'fzero_session_ui|fzero_playthrough|fzero_shim|^fzero_gamepad$' --output-on-failure
```

Next runtime check is one private copy with `Backend=Shared`, FFB/telemetry off,
unchanged owner hashes, and ordinary `--record`; then replay the sealed pair.
This should establish actual unlatched Core startup and title/menu behavior
before a real attended wheel/extra-button drive. Existing injected Practice
evidence does not establish this new ordinary backend's device coexistence.

## Concrete force-ownership follow-up

Keep game-specific spring/damper/road/crash calculation in `fzero_ffb.cpp`. Replace
only its native initialization/delivery ownership when the shared layer supports
it. The needed reusable seam is an epoch-bound force lease on the **same exact
device identity** as the input owner, with explicit cooperative-level transition,
acknowledged zero/stop and force cleanup before that input epoch closes. Either
Core's owner lends an effect-capable device to the toolkit sink, or the toolkit's
single force owner supplies copied input snapshots to the existing copied-provider
bridge; neither option may silently open another DirectInput device for forces.
The latter needs snapshots from the actual force-owned handle, not another read
slot. Identity generation, loss/reopen, stop ordering and unresolved ownership
need native fixtures before choosing the backend. No F-Zero model or tuning change
is required by that seam.


## First private runtime: blocked by legacy SDL acquisition

`E:/fz-probe-ordinary-01` retained the exact `9790788` candidate, game SHA256
`78e58c341689a6ea6e60eb39e02d09fae85cee41ae6ed1c001c14236f97ec8ed`.
The copied configuration selected Shared, FFB off and telemetry off. At 06:43:26 CT
PID 72244 created the game window and original snapshot, but never presented a
game/REC frame. The tape remained zero bytes. WGC retained one black frame then
reported no new frames. The exact child ignored normal close at 70 seconds; the
90-second supervisor retained the lease and marked the original unsealed.

The preserved minidump is `startup-hang.dmp`, SHA256
`a93c817b6ac944c03d7f93ad2c7a2793061dc623c84281c4883c34ed84cc39dc`.
No shared or WheelFfb module had loaded, and no shared host directory existed.
Main-thread stack candidates plus a private symbol map from the same frozen
objects locate `main -> FzeroGamepadRefresh -> SDL_OpenJoystick_REAL ->
SDL_DINPUT_JoystickOpen+0x379`. The original executable's return address
`+0x488179` follows a DirectInput vtable `+0x38` call: `Acquire`, retried after
`Poll` returns input-lost/not-acquired. All 128-byte instruction windows around
the relevant return addresses match the privately relinked symbol-map binary.
The entire relink is not hash-identical (13 data-reference bytes differ), so it
is supporting symbol evidence, not a replacement runtime. Files are under
`E:/fz-ordinary-research/{map.rsp,fzero.map,map-hang.py,inspect-hang.py}`.

Two source defects are now explicit:

- `FzeroGamepadRefresh` opens candidates before checking their SDL GUID, so even
  an unrelated device can be acquired during this selected-wheel scan.
- The pinned SDL DirectInput backend requests **exclusive background** access.
  Its joystick-open path can reset FFB actuators and enable auto-centering when
  the device reports force capability. The mod's no-force latch does not fence
  these SDL-internal calls. No physical torque was measured, and the dump does
  not identify which device was being opened; do not infer that no hardware
  output occurred merely from the disabled mod FFB setting.

Therefore the digital supplement must use the existing Core-owned reader, with
an additive raw button/POV observation seam and exact epoch identity, rather
than a second SDL wheel handle. Preserve explicit legacy X/Y/L/R bindings and
Native behavior. Do not patch the owner's SDL build, drop bindings, weaken the
Shared+FFB refusal, or invent a separate force lease to bypass this failure.

The parent authorized scoped termination after the dump and failed normal close.
Only exact PID 72244 (creation time `134362790062457165`) was terminated after
path/hash readback. At 06:50:48 CT both child and capture helper were gone, all
77 owner pins independently matched, and the matching lease was released.
`recovery.json` records `forced=true`, `runtimeQualified=false`; the initial
failed `result.json` and incomplete original remain unchanged. No replay was
started and no owner install/configuration changed. The wrapper also needs
continuous file-backed stdout/stderr: its in-memory redirected output was lost
when the supervisor exited while the child remained alive.


## Source-only follow-up after the failed run

The small follow-up gates controller initialization and every gameplay/hotplug/
overlay device path on `!FzeroShimUsingInput()` (plus the recorded-replay guard),
and removes both supplemental reads. The header's old mask seam now returns zero.
The Shared direct-ROM admission check precedes the launcher. Recorder input still
combines Core with keyboard, then records exactly the final consumed word. The
persistent limitation notice is separate from REC/Saved/Replay state.

Clean compilation and 28 targeted CTests pass, including the actual host's new
launcher refusal and no-fallback cases, Native/replay controls, actual `.fzpt`
round trip with a keyboard SNES-L input, plus the unchanged SDL virtual-device
fixture. These fixtures do not reproduce the Windows driver's blocking Acquire;
the startup/hotplug/overlay wiring was source-reviewed and the replacement path
was then checked by the bounded private pair below; broader controls/feature qualification remains open.


## Corrected private original and fresh replay: passed narrowly

On October 12, 07:03-07:05 CT, the reviewed source `7187c57f8731423c6318088d21b4f4fa5bf13fdc`
(tree `e0f3bdcce14cef35e60828315f54d128a4d34bae`) ran from two fresh private copies.
The exact game SHA256 was
`177fe4329a67b69dbf90986c7b8be5fb8de85646fb33382344efee9885e459ed`;
the pinned shared DLL remained
`9b8a036f664cf8f45cccf82b98e5e4376f93215b60eac50b6db965bf498015d8`.

- Original: `E:/fz-probe-ordinary-02`, actual ordinary ABI2 flags0 backend;
  900 frames, normal exit 0. Host report resolved, `released=1`, `held=0`,
  `inputReleased=true`, no control worker. SDL device opens were disabled.
- Fresh replay: `E:/fz-probe-ordinary-replay-02`, same frozen config/video/keybinds,
  ROM and Deluxe data. It did not open Core, replayed all 900 frames, verified
  every WRAM checkpoint, and exited normally with code 0.
- Each wrapper checked all 77 owner pins and released its lease. A separate
  post-pair hash pass again found no changed owner pins; both exact PIDs were
  gone and the lease absent at 07:05:36 CT. No owner install was performed.

Tape SHA256:
`0142307c378f6a9c09a193c99d33e0876d465937e1ac066d79a14287fc568880`.
Snapshot SHA256:
`18bda07046cd36c3dceca4ca26d38fa0d7317ed325ff29390af34c54dc8042db`.
Both original and replay copies match exactly. These are **at-rest inputs, not
all-neutral inputs**: 563 words are `0x40000000`, 337 are `0x40000080` (right
steering PDM pulses). They match the earlier nonlatched preview tape, including
the same at-rest steering offset. There were no injected actions and no observed
player driving. Do not call the title animation a driven workload.

Viewed native-window frames:

- REC plus the explicit wheel-extras limitation:
  `E:/fz-probe-ordinary-02/frames/burst/frame-0001.png`.
- Saved 900 frames plus the same limitation: `frame-0158.png` in that folder.
- Replay 714/900: `E:/fz-probe-ordinary-replay-02/frames/burst/frame-0001.png`.
- Replay verified 900: `frame-0166.png` in that folder.

The passive WGC bursts retained 158 and 166 frames respectively, then reported
exit 4 when the target closed. Their service status remains failed/partial; the
individual inspected images establish only the displayed states. They are not
presentation/performance measurements. Ctrl+F11 key routing remains untested;
these sessions used the existing frame-bounded normal-close path.

The wrapper is `E:/fz-ordinary-research/Invoke-OrdinaryCopy02.ps1`, SHA256
`8b31b28b90e5da196bdfe116b93e01ea1c04c213020534edfb654128add034ad`.
It preserves logs continuously rather than buffering until process exit. A dummy
waiting child proved that stdout and stderr are readable while it is still alive;
readers use `FileShare.ReadWrite` to coexist with the writer. The old wrapper and
failed original were not changed. Derived checks are in `verify-ordinary02.py` and
`ordinary02-evidence.json`, SHA256
`e199db0616057b2ee239c9d4cff3d84c55d3f631e8555511e95db62ea54ef544`.
The generic result's `runtimeQualified=false` is deliberately retained: passing
this narrow check does not qualify the entire mod for release.

## Next owner-facing gaps

1. Reuse the reviewed shared provider/force ownership work for physical FFB;
   do not add a competing SDL/native owner. Preserve the accepted model/tunes.
2. Obtain explicit extra-button bindings from that same owned snapshot, with
   exact device/epoch identity, then integrate the existing launcher. The current
   missing-extras/direct-ROM limitations are temporary and disclosed.
3. After the ordinary backend is complete, add Wheelkit discovery for the
   existing `--record` / `--replay` entry points and `.fzpt` + `.state` pair. Do not
   introduce another recording container or silently select Shared on install.
4. Qualify a moving ordinary original and the remaining feature/release checklist.
   The previously qualified injected Practice drive and this ordinary startup
   pair are separate evidence. Public Deluxe packaging still follows the existing
   game-data/licensing review; no default/install/publication decision follows
   from these private checks.
