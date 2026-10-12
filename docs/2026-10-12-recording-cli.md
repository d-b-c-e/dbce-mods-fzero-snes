# Recording controls and shared-profile preview

Source candidate, October 12. No owner install or launch for this UI change yet.
The [earlier player Practice original](2026-10-12-shim-diagnostic.md) remains
immutable: all 2,351 final consumed input words matched its shared capture,
and the pinned headless runner matched all 2,351 WRAM checkpoints. That proof
uses the earlier runtime; it does not qualify these new UI paths.

## Explicit launch options

From a candidate folder, with the same ROM, Deluxe/MSU mode and display settings
that will be used for replay:

```powershell
./FZeroSNESRecomp.exe --record E:/my-recordings/first-drive.fzpt ./fzero.sfc
./FZeroSNESRecomp.exe --replay E:/my-recordings/first-drive.fzpt ./fzero.sfc
./FZeroSNESRecomp.exe --shared-profile-preview E:/fz-owner-preview-01 ./fzero.sfc
```

`--record` uses the existing ordinary input backend and force settings. It records
from boot, including menus. It is **not a muted unattended test mode**. Choose a
fresh `.fzpt` path inside an existing directory; its `.fzpt.state` sibling must
also be absent. Neither file is overwritten. Keep the pair together.

The REC banner is at the top of the centre game view, on both SDL and OpenGL
presentation paths. **Ctrl+F11 stops an active recording** and shows Saved only
after the recorder closes successfully. Ordinary play can continue. Plain F11
and Shift+F11 retain their existing save-slot behavior. Loading a state, resetting
or opening the state/rewind overlay while recording invalidates the tape and
shows Error; this also makes the final process exit nonzero.

`--replay` consumes the recorded final SNES word and checks every post-frame WRAM
hash. Its REPLAY banner shows progress. It closes at the end and shows Replay
verified only after a complete close. Closing early or diverging is an error.
It initializes neither the wheel-force device nor the telemetry transport and
does not apply the controls profile, create missing keybinds, migrate SRAM or
save video defaults. A CLI replay goes directly to its explicit ROM, so no
launcher settings page is opened or saved. ROM identity and the snapshot are
validated; the caller must retain the original presentation, Deluxe and MSU
configuration. For automated evidence, the existing pinned case adapter remains
the path that also verifies module/data/config identities.

All three options require an explicit ROM and reject `--launcher`, duplicates,
combinations, missing arguments and pre-existing recording/replay/injection
environment requests. Relative paths resolve against the caller's directory
before the host anchors its own configuration directory. Startup failures print
an error and return nonzero without falling back to ordinary play. Errors after
the game window exists show the Error banner. Normal launch without these flags
continues to use the existing backend and settings.

## Shared profile preview

`--shared-profile-preview` is a bounded, explicitly muted preview of physical
wheel input through the existing production shared ABI2 provider and F-Zero
mapper. It uses `[Controls]` in the candidate's `config.ini`, keeps keyboard SNES
input available, and has **no session/batch injection**. The fresh short directory
contains `original.fzpt`, its state, the frozen controls/config and the existing
shared capture with computed force/telemetry signals. It is not a new format.

The preview banner says MUTED PREVIEW. It blocks physical FFB and UDP/motion
delivery while retaining computed signals, uses private save storage and avoids
owner configuration writes. Ctrl+F11, Esc or normal window close stops the shared
provider and both recorders before showing Saved. It closes automatically after
230 seconds, before the diagnostic provider's 240-second hard limit. A provider
or recorder failure prevents successful sealing/exit. Startup may wait for the
configured physical devices; this mode does not fabricate missing input.

The current shared mapper can represent steering, throttle, brake, Confirm,
Back, Start, Select and menu navigation. ABI2 has no independent SNES X/Y/L/R or
host save/rewind actions, so those wheel actions are explicitly unsupported
here; they are not aliased onto unrelated generic actions. Existing keyboard
SNES bindings remain usable in this preview. This action-vocabulary limitation
is separate from the unimplemented ordinary shared backend. Multi-device
normalization is provided by Core, but a real owner wheel+button-device drive
has not yet qualified this consumer.

These diagnostic restrictions are **not permanent product requirements**.
The next ordinary shared-backend slice must separate input ownership from the
preview's output/persistence/deadline policy, retain the complete owner control
set (including SNES extras), and qualify a real unlatched owner gameplay
recording and its cold replay. It must not silently replace current bindings
with this partial preview. Launcher/Wheelkit record/replay buttons are also
still unimplemented; the new CLI gives them a direct entry point.

## Source validation and next runtime check

`fzero_session_ui` exercises the production parser and real `.fzpt` recorder:
95 checks cover CLI ambiguity, unchanged F11, clean nonempty sealing, verified
replay, early replay close, empty recordings, shared-provider failure, sticky
invalidation and a real read-only stdio close failure. The targeted 16-test
recorder/shared-provider suite and the graphical build pass. The only compiler
warning is the existing Windows `HIBYTE` macro redefinition.

Next qualification is a fresh private candidate copy under the usual lease:
brief physical-at-rest `--shared-profile-preview`, window-captured REC and Saved,
then its explicit `--replay`, recorded input/WRAM verification and unchanged
owner file hashes. A separate ordinary attended `--record` checks Ctrl+F11
continuing normal play. No synthetic title/attract capture can establish wheel
driving acceptance. Rendering, Ctrl+F11 routing and the new CLI startup path
remain source/test evidence until those actual runs occur.
