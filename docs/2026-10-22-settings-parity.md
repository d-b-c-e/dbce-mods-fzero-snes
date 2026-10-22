# Settings parity: launcher, Wheelkit and F6 (2026-10-22)

Documentation only. No source or default changed. Source of truth: `src/fzero_mods.c`
(launcher provider), `src/fzero_video.c` (`fzero-video.ini`), Wheelkit
`src/Wheelkit.App/Data/catalog-v6.json` (`wheelkit.catalog@6`, game `f-zero`), and the
October 18 simple-UI section of `E:\Source\dbce-project-mgmt\docs\agent-work-policy.md`.

## Findings

- Wheelkit catalog-v6 exposes exactly three F-Zero settings, all in `config.ini`
  `[ForceFeedback]`: `Enabled`, `SteeringStrength`, `ImpactStrength`. Everything else
  below is launcher-only today. Wheelkit also writes controller bindings through its
  Game controls binder (not catalog settings); those land in `[Controller.<guid>]`.
- Settings live in two files. **Video/presentation keys** are in `fzero-video.ini`
  `[FZeroVideo]`; **wheel and FFB keys** are in `config.ini`. Wheelkit only reads and
  writes `config.ini`. Video keys therefore have no Wheelkit path.
- Gaps to resolve (not changed here): `[ForceFeedback] Strength` is written by the
  launcher (legacy master, default 40, seeds `SteeringStrength` when absent) but not in
  the catalog; `Device` and `ImpactType` are launcher-only.
- Wheel options are gated: the launcher shows them only with a valid selected
  controller GUID (`wheel_section` set); `AnalogSteering` is the per-controller enable.

"Presence" below means Wheelkit catalog-v6: **yes** (listed) or **no**.

## 1. Presentation (`fzero-video.ini`, section `[FZeroVideo]`)

| Launcher id | ini key | Type / range / default | Wheelkit v6 | Placement | Preset group |
|---|---|---|---|---|---|
| Widescreen feature | `EnhancedRenderer` | 0/1, default 0 | no | Simple | Graphical Tweaks (widescreen on at Low+) |
| `aspect` | `Aspect` | 16:9, 21:9, 32:9, Fit; default Fit | no | Simple (display setup, outside presets) | none: essential display setup |
| Presentation FPS feature | `PresentationEnabled` | 0/1 | no | Simple | Graphical Tweaks (Medium+) |
| `fps` | `PresentationFPS` | Auto, 60, 90, 120, 144, 165, 240, 360; default Auto | no | Advanced | Graphical Tweaks breakdown (High 120, Ultra Auto/240) |
| `ShowFPS` | `ShowFPS` | 0/1, default 0 | no | Advanced | none (diagnostic readout) |
| HD Mode 7 feature | `HDMode7` | 0/1 | no | Simple | Graphical Tweaks (High+) |
| `scale` | `HDMode7Scale` | 2 to 10, default 2 | no | Advanced (warn above 4x) | Graphical Tweaks breakdown (High 2, Ultra 4; never above 4 in a preset) |
| BS Deluxe feature | `BSDeluxe` | 0/1; unavailable without Deluxe build | no | Simple (content, not a visual preset) | none |

Proposed preset (explicit, reversible, shows Custom on any manual divergence; Off disables
only our optional tweaks): Off = EnhancedRenderer 0, PresentationEnabled 0, HDMode7 0.
Low = widescreen only. Medium = Low + PresentationEnabled 1, PresentationFPS Auto.
High = Medium + HDMode7 1, scale 2, FPS 120. Ultra = High + scale 4, FPS Auto.
Levels with identical effect on a given rig should be hidden. Opening a page never
rewrites values.

## 2. Triple screen (`fzero-video.ini`)

Essential rig geometry stays separate from visual presets.

| Launcher id | ini key | Range / default | Wheelkit v6 | Placement |
|---|---|---|---|---|
| Triple Screen feature | `TripleScreen` | 0/1 | no | Simple (experimental label) |
| `TripleOutputMode` | `TripleOutputMode` | `Span` or `Separate`; default Span (UI: Surround / Separate monitors) | no | Simple |
| `TriplePanelWidthMm` | same | 200 to 3000, default 708 | no | Simple (setup) |
| `TripleEyeDistanceMm` | same | 200 to 3000, default 660 | no | Simple (setup) |
| `TripleLeftAngleDeg` | same | 0 to 89, default 70 | no | Simple (setup) |
| `TripleRightAngleDeg` | same | 0 to 89, default 70 | no | Simple (setup) |
| `TripleBezelGapMm` | same | 0 to 100, default 8 | no | Advanced |
| `TripleEyeHeightMm` | same | 0 to 1000, default 0 | no | Advanced |

## 3. Flash reduction and support (`fzero-video.ini`)

| Launcher id | ini key | Default | Wheelkit v6 | Placement | Preset group |
|---|---|---|---|---|---|
| Reduce crash flashes | `ReduceCrashFlash` | 0 | no | Simple (accessibility) | none (never overwritten by presets) |
| Diagnostics | `Diagnostics` | 0 | no | Developer opt-in | none |

## 4. Force feedback (`config.ini`, section `[ForceFeedback]`)

| Launcher id | ini key | Range / default | Wheelkit v6 | Placement |
|---|---|---|---|---|
| Force feedback feature | `Enabled` | 0/1, default 0 | yes | Simple (stays off until the user enables) |
| `SteeringStrength` | `SteeringStrength` | 0 to 100, default 40 (inherits `Strength` if absent) | yes | Simple |
| `ImpactStrength` | `ImpactStrength` | 0 to 100, default 20 | yes | Simple |
| `Device` | `Device` | device name string; empty = none | no | Simple (needed to choose the wheel) |
| `ImpactType` | `ImpactType` | `Constant` or `Sine`; default Constant | no | Developer opt-in (comparison tuning) |
| (legacy) | `Strength` | 0 to 100, default 40 | no | Hidden; preserve only |

FFB is never part of the Graphical Tweaks preset.

## 5. Wheel (`config.ini`, section `[Controller.<SDL GUID>]`)

All keys are per controller. All are launcher-only in the catalog; Wheelkit's Game
controls binder writes the same section.

| ini key | Range / default | Placement |
|---|---|---|
| `AnalogSteering` | 0/1, default 1 on load | Simple (enable wheel) |
| `SteeringAxis` | 0 to 15, default 0 | Simple (bind, raw-axis capture) |
| `AcceleratorAxis`, `BrakeAxis` | -1 to 15, default -1 | Simple (bind) |
| `SteeringInvert`, `AcceleratorInvert`, `BrakeInvert` | 0/1, default 0 | Advanced |
| `Deadzone` | 0 to 20, default 0 | Advanced |
| `SteeringRangePercent` | 10 to 100, default 100 | Simple (steering range) |
| `SteeringResponsePercent` | 25 to 200, default 50 | Advanced |
| `PedalThreshold` | -32768 to 32767, default 0 | Advanced |
| `ButtonA`, `ButtonB`, `ButtonX`, `ButtonY`, `ButtonL`, `ButtonR`, `ButtonSelect`, `ButtonStart` | -1 to 127, default -1 | Simple (bind) |
| `ButtonUp`, `ButtonDown`, `ButtonLeft`, `ButtonRight` | -1 to `RECOMP_RAW_HAT_MAX`, default -1 | Simple (bind) |
| `ButtonSaveStateMenu`, `ButtonRewind` | -1 to 127, default -1 | Advanced |

Controls and calibration are outside visual presets.

## 6. Single key set for F6

F6 controls must not define new names. Each F6 control reads and writes the key listed
here, in the file and section below, and preserves unknown keys and values:

| Area | File | Section | Keys |
|---|---|---|---|
| Presentation, triple, flash, support | `fzero-video.ini` (exe-anchored) | `[FZeroVideo]` | `EnhancedRenderer`, `Aspect`, `PresentationEnabled`, `PresentationFPS`, `ShowFPS`, `HDMode7`, `HDMode7Scale`, `BSDeluxe`, `Diagnostics`, `TripleScreen`, `TripleOutputMode`, `TriplePanelWidthMm`, `TripleEyeDistanceMm`, `TripleLeftAngleDeg`, `TripleRightAngleDeg`, `TripleBezelGapMm`, `TripleEyeHeightMm`, `ReduceCrashFlash` |
| Force feedback | `config.ini` | `[ForceFeedback]` | `Enabled`, `Strength` (legacy, read only), `SteeringStrength`, `ImpactStrength`, `ImpactType`, `Device` |
| Wheel and buttons | `config.ini` | `[Controller.<SDL GUID>]` | `AnalogSteering` plus every key in section 5 |

Value forms: booleans are `0`/`1` in the ini (launcher UI shows true/false);
`TripleOutputMode` is `Span`/`Separate`; `PresentationFPS` is `Auto` or a listed rate;
`Aspect` is `16:9`, `21:9`, `32:9` or `Fit`.

## Open items for the owner or a follow-up task

1. Wheelkit v6 covers 3 of about 70 keys and reads only `config.ini`; showing video
   keys in Wheelkit needs a catalog entry that points at `fzero-video.ini`
   (`[FZeroVideo]`) or a launcher-only decision.
2. Decide whether `Device` should enter the catalog (needed to apply a force setup).
3. "F6" is taken here to mean the F6 settings surface; no F6 source exists in this repo
   yet (the only F6 hit is Ctrl+F6 aspect cycling in `ADAPTIVE_RENDERER.md`), so the
   table above is the contract for it.
