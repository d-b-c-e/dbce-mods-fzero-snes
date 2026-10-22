# Game-action binding coverage: Native vs Shared (2026-10-22)

Documentation only, read from source on `main` `799cdd3`. No source, default or
installed file changed. Nothing here was run in game or on a physical device:
**in-game and physical delivery of every action below is unverified.**

SNES bits: 0 B, 1 Y, 2 Select, 3 Start, 4 Up, 5 Down, 6 Left, 7 Right, 8 A, 9 X,
10 L, 11 R; 24/25 are the host save-state browser and rewind.

## Coverage

| Action | Native profile (`[Controls]` -> `[Controller.<guid>]`) | Native raw reader (`[Controller.<guid>]` keys) | Shared (Core ABI) |
|---|---|---|---|
| steer | `SteeringAxis` + `SteeringInvert` (required) | `SteeringAxis` | slot 0, gameplay only |
| throttle | `AcceleratorAxis`/`AcceleratorInvert`, axis only | `AcceleratorAxis` -> B | slot 1 -> B over `PedalThreshold` |
| brake | `BrakeAxis`/`BrakeInvert`, axis only | `BrakeAxis` -> Y | slot 2 -> Y over `PedalThreshold` |
| confirm | `ButtonA` | `ButtonA` -> A | slot 9 -> A |
| back | `ButtonB` | `ButtonB` -> B | slot 10 -> B |
| start / select | `ButtonStart` / `ButtonSelect` | same keys | slots 11 / 12 |
| navUp/Down/Left/Right | `ButtonUp`..`ButtonRight` | same keys (hat or button) | slots 13..16 |
| SNES X / Y / L / R | not translated ("no F-Zero control") | `ButtonX`, `ButtonY`, `ButtonL`, `ButtonR` | declared `fzero-snes.x/y/l/r`, only with `[Input] SharedExtras=1` |
| save-state browser / rewind | not translated | `ButtonSaveStateMenu`, `ButtonRewind` -> 24/25 | declared `fzero-snes.saveMenu/rewind`, only with `SharedExtras=1` + `SharedHostControls=1` |
| clutch, handbrake, shiftUp/Down, neutral, reverse, gear1-8, camera, lookBack, reset, horn, modMenu, muteFfb | noted as "no F-Zero control" | none | declared unsupported with a reason |

Native profile rules: every bound action must be on the steering device (F-Zero
reads one controller), only the qualified MOZA R12 (`346E:0006`) is accepted, button
pedals are refused, and explicit empty entries unbind (steer stays). So X/Y/L/R and
save/rewind reach Native only through the launcher's direct keys, not a profile.

Shared rules: only `Status::Live` (12) values count; injected extras are refused;
outside gameplay steer is neutral and only digital actions reach the overlay.

## Sources

- Native profile translation: `src/fzero_controls.cpp:41-46` (digital table),
  `:238-300` (`FzeroControlsPlanLines`: steer, pedals, digital, unbound).
- Native raw reader: `src/fzero_gamepad.c:20-25` (key -> bit table),
  `:124-129` (axis keys), `:293-301` (pedals -> B/Y).
- Shared mapping: `src/fzero_shim_map.c:5-38` (actions, unsupported reasons,
  both declarations), `:51` (extras slots -> 9, 1, 10, 11, 24, 25),
  `:93` (digital slots 9..16), `:98-115` (steer gating, pedals).
- Shared opt-ins: `src/fzero_shim.cpp:231-239` (`SharedExtras`,
  `SharedHostControls`), `:368` (declaration choice).
- Prior handoffs: `docs/2026-10-14-shared-extras.md`,
  `docs/2026-10-16-shared-host-controls.md`, `docs/2026-10-10-profile-controls.md`.
