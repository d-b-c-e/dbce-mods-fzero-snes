# 2026-10-10: rig-profile controls installed and applied on the rig (STD-033)

Source `af2ffe6` (`src/fzero_controls.cpp`, `src/fzero_controls_sdl.c`, README "Rig-profile controls"). Built in a
separate tree (`build-claude-merge`, BS Deluxe embedded, SDL3 sources copied from `build-merge`). All 18 ctest suites
pass there; the new `fzero_controls` has 102 checks. In the first full run `fzero_gamepad` hung once (stopped after 10
minutes). It passed three reruns, and 18/18 passed on the next full run. That test leaves SDL's hardware joystick
drivers on, so the rig's real devices are enumerated during it. `fzero_controls` turns them off.

## Install

`tools/Install-StreamDeck.ps1 -BuildDir build-claude-merge -Label profile-controls`: launcher `79667FC2...` from
`af2ffe6`, `WheelFfb.dll` unchanged (`3F6CC514...`). Backup `deployment-backups\2026-10-10-profile-controls`. Only
documentation changed between the previous install (`4ac6f6b`) and `af2ffe6`'s parent.

## The owner's profile

The owner's active Wheelkit profile ("Moza KS Pro Triple", the same section V-Rally 4 received on 2026-10-09) was added
to `config.ini` as `[Controls]` by a test helper. That is helper provisioning, not Wheelkit's production Apply. Backup:
`E:\Source\_archive\2026-10-10\fzero-owner-controls-provisioning-005018` (`config.ini.before`, `.provisioned`, the
section).

## Launch check, 00:50-00:51 CT

Direct start with the ROM (no launcher), no input, force feedback off for the run. The game's log:

```
[fzero-controls] profile 'Moza KS Pro Triple' revision owner-provisioned-2026-10-09 applied to 0300102d6e3400000600000000000000 (MOZA R12 Base): 13 keys
[fzero-input] ignoring raw Gudsen MOZA Multi-function Stalk guid=030063096e3400002400000000000000 (wanted 0300102d6e3400000600000000000000)
[fzero-input] raw Gudsen MOZA R12 Base guid=0300102d6e3400000600000000000000 steering-axis=0 gas-axis=2 brake-axis=5
```

- It reached the attract demo ("PUSH START") and closed normally.
- `config.ini` changed only where intended: in `[Controller.0300102d...]`, `ButtonStart` 36 -> 35 and `ButtonSelect`
  23 -> 22; a new `[ControlsApplied]` (Revision, Device); `config.ini.before-profile-controls` created.
- Every other key already matched the profile (steering 0, accelerator 2, brake 5, A 31, B 18, hat 128-131), and the
  owner's own X/Y/L/R, save-state, rewind, dead zone and pedal threshold stay.
- The `[ForceFeedback] Enabled` line went back to the owner's `1`. The other 97 owner files were byte-exact; one new
  diagnostics file was archived and removed.
- Not applied (logged): clutch, handbrake, shifts, reverse, gears, camera, look back, reset, horn. F-Zero has no such
  controls.
- Evidence: `E:\Source\_archive\2026-10-10\fzero-controls-apply-005025` (stderr, frames t10-t40, `config.ini.after`).

## Still open

- **Start and Select now follow the profile** (35 and 22) instead of the owner's earlier launcher capture (36 and 23).
  If either does nothing at the wheel, check those buttons in the Wheelkit profile. Every game follows the profile.
  Undo: copy `config.ini.before-profile-controls` back, or rebind on the launcher's wheel page (it stands until the
  profile's revision changes).
- Observed input on the real wheel: F-Zero reads the wheel through SDL inside the process, where the toolkit's
  DirectInput injection does not reach. So "observed" rests on the virtual-wheel ctest, and owner acceptance is the
  next step.

## 00:54: reinstall with Astra's review fixes (95d34e4)

Two attached devices with the profile's vendor/product are refused, and only qualified devices (so far the R12)
carry DirectInput indexes over to SDL (`fzero_controls`: 108 checks; 18/18 suites). Installed with label
`profile-controls-refusals` (backup `deployment-backups\2026-10-10-profile-controls-refusals`). Launch check 00:54:
the revision was already recorded, so nothing was rewritten (`config.ini` byte-identical across the run). The game
opened the R12 with steering 0, accelerator 2, brake 5, and the other 98 owner files were byte-exact. Evidence:
`E:\Source\_archive\2026-10-10\fzero-controls-refusals-005400`.

## 01:07: the reviewed transaction build (06ef443)

Astra reviewed 3cfaa00 (3487): both blockers from their 3459 are closed. Button pedals are refused, and the edit is
one read-back document moved over `config.ini` in one step, with fault-injection tests. Remaining limits as stated
there: R12 correlation only, the owner's threshold and rest behaviour kept, and a revision gate rather than
continuous readback. 06ef443 adds the toolkit 65c686d header (one-line bindings). 18/18 suites. Installed as
`controls-transaction` (backup `deployment-backups\2026-10-10-controls-transaction`). Launch check 01:07: nothing
pending, `config.ini` unchanged, the R12 opened with steering 0 / accelerator 2 / brake 5, the other 98 owner files
byte-exact. Evidence: `E:\Source\_archive\2026-10-10\fzero-controls-transaction-010755`.
