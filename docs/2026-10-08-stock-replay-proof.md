# Fresh stock recording and steering trials

An isolated **synthetic stock** capture at source
`40275142710711f33dd076fceb235da817770e68` recorded 3,600 frames, then replayed
all 3,600 through the verified headless player three times. The emulator's
recorded state checks passed in the baseline and independent steering 0/80
trials. No game window, controller, wheel acquisition or physical force was used.

This replaces the missing historical case as a current proof that stock
recording and model replay work. It is not an owner drive, an attended feel
test, or evidence for the installed BS Deluxe data. Keep it out of Art
normalization workloads: the spring output is a condition coefficient, not a
measured or constant wheel torque.

Private artifacts are in
`%LOCALAPPDATA%/Dbce/StagePlayback/SessionEvidence/fzero-stock-synthetic-20261008-0346`:
the frozen headless executable and clean capture receipt, original tape/state,
configuration, ROM, case manifest, three observation streams and
`steering-trials-summary.json`. Do not publish the ROM or private case bundle.

| Evidence | Result |
|---|---|
| Case SHA-256 | `4230ca346b980583cff9d4941db59b98d724a4375e02c23af2d0f565e9550e6b` |
| Frozen headless executable SHA-256 | `13af0f8a928a2fe952a02110617e4d715dd64b00af5f91e88a75d068b0be9a68` |
| Baseline / trial steering | 40 / 0 / 80; model 3 |
| Auxiliary / crash | 40 / 20 in every run |
| Per-stream observations | 3,600 each for steering, spring, damper and road; 13 impacts; one final stop |
| Independent zero | Zero steering and spring throughout; 952 baseline spring rows were nonzero |
| Auxiliary invariance | Every damping, road, impact and stop request identical, including timing |
| Baseline peaks | Spring coefficient .40, damping .16, road .16, impact software request .20 |

The automated script was
`240:8,400:256,600:256,800:256,1100-3599:1,1500-1530:64,1800-1830:128`.
It produced a brief race/collision workload and later stopped moving; it is
not a representative normalization drive. The capture ran with FFB and telemetry
disabled, MSU disabled, stock 16:9 and isolated save files. Model observation is
offline computation and is intentionally available with physical FFB disabled.

To repeat a trial from this sealed case, use the frozen runner (not a silently
replaced current build), `tools/fzero_replay_adapter.py observe`, the existing
case path, toolkit path and original ROM, a new output filename, and optionally
`--steering-strength 0` or `80`. Omit `--strength` to preserve auxiliary intensity.
`create` pins the original inputs; `observe` verifies them, checks the entire
game replay before creating force observations, and refuses unsupported Deluxe
cases or an old runner which ignores separate steering.

Native lifecycle maintenance after this recording is a separate component
change; the frozen headless observations never load the native DLL. See
`reviews/2026-10-08-native-lifecycle-adoption.md` for that candidate and its
remaining runtime/physical checks.
