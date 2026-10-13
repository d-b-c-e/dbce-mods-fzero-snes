# Record and replay a drive

Use a candidate with the explicit recording CLI. The helper never edits settings;
ordinary recording retains the game's normal settings/save behavior. It starts
at boot, includes menu navigation,
and stores the exact SNES input consumed by each emulated frame.

Create a recording folder, then run from the extracted candidate in PowerShell:

```powershell
New-Item -ItemType Directory -Path ./recordings
./tools/Recording.ps1 Record -Rom ./fzero.sfc -Tape ./recordings/my-drive.fzpt
```

Drive normally. **REC** appears above the centre view. Press **Ctrl+F11** to stop
recording and keep playing, or close the game normally. **Saved** means the
recorder closed successfully. Plain F11 retains its existing save-slot behavior.
Loading a save state, resetting or using rewind invalidates a recording; an
**Error** banner and a nonzero process exit identify the failure. A crash or
forced close does not produce a usable tape.

Keep `my-drive.fzpt` and `my-drive.fzpt.state` together. Use a fresh filename for
each drive; neither file is overwritten. Record uses your configured input and
force settings: it is not an unattended force-muting mode.

To watch the drive:

```powershell
./tools/Recording.ps1 Replay -Rom ./fzero.sfc -Tape ./recordings/my-drive.fzpt
```

Replay consumes the saved input, checks game memory after every frame and shows
**REPLAY** progress followed by **Replay verified**. The game closes after the
recording ends. Early close or a mismatch fails the run. Replay opens no force
device, sends no UDP telemetry, ignores live wheel/keyboard gameplay input and
does not write settings or saves.

Keep the original ROM, game build, Deluxe/MSU data and presentation settings.
The tape validates its ROM and initial snapshot; automated qualification uses
the separately pinned case adapter to check the remaining identities. A tape
does not replace a backup of your configuration. These files remain private and
are not bundled with the public package.

The helper is a convenience entry point for the existing executable arguments:
`--record <tape> <rom>` and `--replay <tape> <rom>`. It never rewrites an INI or
changes the input backend. Ctrl+F11 routing still needs an attended check; the
private runtime evidence so far used bounded recordings and normal close.

## Shared input preview

**Native remains the default.** The provisional Shared runtime is a separately
pinned, private dependency and is not included in the public package. A normal
Native recording does not require it.

Private Shared candidates read `[Input] Backend=Shared` and the existing shared
`[Controls]` profile from `config.ini`. They require an explicit ROM at launch;
`--launcher` is currently refused. Shared plus enabled FFB is refused until the
single-owner input/force bridge is qualified. This restriction does not alter
Native's existing force behavior or tune.

Core provides steering, throttle, brake, Confirm, Back, Start, Select and menu
navigation. Legacy wheel-only SNES X/Y/L/R and save/rewind buttons are not read
by this Shared candidate; keyboard SNES bindings remain available. A visible
limitation banner discloses this. Existing legacy bindings stay in the file for
Native; there is no silent fallback to another device reader.

Private Deluxe checks on October 12 verified a 900-frame ordinary Shared
at-rest original and fresh graphical replay, with visible REC/Saved/REPLAY and
all memory checkpoints matching. A separate injected Practice drive has 2,351
exact cold frames. These are distinct proofs: physical owner driving, complete
wheel-button parity, launcher integration and physical Shared FFB are pending.
Wheelkit's recording entries are also pending; the CLI is ready for that
integration without a new recording format.

October 13 internal source update: a receipt-owned ordinary Wheelkit companion is
implemented and passes offline fixtures, including Stop while the game stays open.
It requires a newly built/installed session capability; the current owner receipt
does not qualify. Optional private `-PrivateSharedInput` installation now owns the
existing DLL/header/PIN/provenance inventory and can declare Shared recording.
Admission independently verifies those bytes and still requires force feedback
off; it never changes the backend, profile or force setting. Native remains the
default. Automated-original state isolation remains pending; the existing
direct-ROM Shared CLI and prior diagnostic evidence are unchanged. See
[the source checkpoint](2026-10-12-recording-cli.md#october-13-ordinary-companion-source-checkpoint).
