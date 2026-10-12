# Ordinary shared input candidate

The first private ordinary run at `9790788` FAILED during legacy SDL device
startup on October 12. Do not install, repeat this path, or claim ordinary Shared
qualification until the supplemental reader is replaced. The earlier private CLI
proof at `26a3f0e` remains intact; the new backend has not passed an owner drive. The default backend, owner configuration, FFB model/tunes and
display geometry remain unchanged.

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
| Ordinary Shared | Core actions, keyboard, explicit legacy digital supplement | Ordinary SRAM/host settings and configured telemetry; FFB enabled is refused | Optional `.fzpt`; no shared diagnostic recorder |
| Capture-only preview | Core actions and keyboard | Private saves, no force/UDP delivery | Mandatory `.fzpt` plus shared computed-signal capture, bounded |
| Latched diagnostic | Core's journalled injected actions; keyboard suppressed | Private saves, no force/UDP delivery | Same pair with injected provenance and completed terminal required |
| Recorded replay | Final input words from tape only | No input/force owner, UDP, config/profile or SRAM writes | Verify every recorded WRAM checkpoint |

Ordinary Shared calls ABI2 `open2` with flags **0**. It has no injection files,
control worker, diagnostic capture, 240-second deadline or save redirect. A small
unique folder under `diagnostics/` receives the shared host's shutdown report.
An explicitly supplied `.fzpt` is still optional, using the existing CLI/recorder.

At each emulated gameplay frame, Core runs once and the existing steering PDM
maps its normalized steering. The saved native range/response/pedal threshold
still shape this game adapter. Keyboard bits and the allowed legacy digital
buttons are merged before the normal host filters. Opposing directions are
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

## Preserved legacy buttons, explicitly supplemental

ABI2 has no independent SNES X/Y/L/R or host save/rewind actions. Those existing
bindings remain in the selected SDL controller's digital reader. Generic Confirm,
Back, Start, Select and navigation buttons also retain their legacy mapping only
when Core explicitly reports that action **Unbound**. A bound action that is
opening, missing, mismatched, invalid, stale or failed never falls back to its old
generic mapping. Keyboard controls remain available.

This supplemental reader reads configured buttons/POVs, not legacy steering or
pedal axes, advances no PDM and does not run the old test-injection path. SNES Y
shares the guest brake bit, but the preserved native **ButtonY** is an independent
digital binding, not a substitute for a missing Core pedal. A gamepad trigger
explicitly used in a digital button combination remains that saved combination.

The supplement still uses the existing selected SDL GUID and its handle; it is
**not** claimed as Core-owned or multi-device parity for SNES extras. Keeping it
avoids silently dropping the owner's existing controls while the shared action
vocabulary is incomplete. Native SDL analog steering/pedals and mapped generic
buttons cannot override the corresponding Core actions.

## Offline validation and next runtime slice

The actual `fzero_shim.cpp` host runs against a fake ABI DLL, with no device or
UDP transport. Fixtures cover default Native, replay bypass, ordinary flags0,
exact profile bytes, no capture/deadline arguments, no save-root/recording path,
initial and launcher-time Shared+FFB refusal, actual force-latch refusal,
malformed Core profile, stray injection request and an unexpected latched frame.
They also cover canceled overlay ticks, missing-bound versus unbound buttons,
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
