# Independent steering strength candidate

The launcher now calls the wheel centering control **Steering strength (%)**.
It changes the spring coefficient and the constant-force fallback only. Damping,
road texture and the independent crash intensity remain at their recorded levels.
This is a hover-machine condition model, not a tyre force or fixed-torque scale.

`[ForceFeedback] SteeringStrength` is new. Without it, steering inherits the
existing `Strength` (default 40), so every force term remains unchanged. The
launcher preserves that legacy value for damping/road and saves steering
separately. The fresh launcher default now agrees with the runtime's existing
40; it previously displayed 35 when the key was absent. Saved values are retained.
Crash settings, native DLL and installed owner files are unchanged by this source
candidate. The legacy auxiliary level remains an INI setting, not a new slider.

The old `FzeroFfbCompute` entry retains the original behavior. The separate
`FzeroFfbComputeSteering` entry updates the same state once and separates only
the two steering outputs. Zero steering still permits damping, road and crashes;
FFB Off remains the all-effects stop.

## Recording and comparison

The headless observer emits legacy `FZFFB1` / model 2 for legacy captures and
`FZFFB2` / model 3 for a separate steering setting or trial. The new header pins
both the auxiliary and steering strengths. The adapter refuses an old runner
which ignores the new setting and emits the old header. Trial/config/profile
hashes distinguish the chosen model; original tapes are never rewritten.

`--steering-strength 50` performs an independent model-3 trial. `--strength`
retains its old meaning for legacy cases; with independent steering selected it
changes damping/road only. Omit it to retain the recorded auxiliary level.
`--impact-strength` stays independent. The adapter still refuses Deluxe cases
without a separately pinned Deluxe data identity.

The ROM-free component trace also accepts an optional independent steering value:

```powershell
./build-merge/fzero_force_trace.exe <new.csv> 40 0
./build-merge/fzero_force_trace.exe <different-new.csv> 40 80
```

Retain the command beside the CSV: its columns alone do not pin the supplied
strengths or build. It is synthetic component evidence, not a captured playthrough.

## Qualification

The Release launcher, headless player and affected fixtures build. There is one
pre-existing HIBYTE macro-redefinition warning in sdl_main.c. All 17 CTest suites
(including model, fake-native lifecycle and launcher provider) and seven
replay-adapter Python tests pass. New cases cover old-config fallback, independent save, invalid range,
600 trajectory frames with zero steering and unchanged auxiliary/collision
output, and new-header refusal by the old contract.

An archived pre-change model-trace executable and the new legacy path produce
byte-identical 601-row output at 40: SHA-256
`22252853BC927FDA24D7D3A6E9BE21A378F9D20073B2689C96A0AFE8984E1DF8`.
Independent steering 0/80 preserves every damping, road and collision column;
zero steering produces zero spring/fallback values throughout. Evidence is
private under `SessionEvidence/fzero-independent-steering-20261008`.

The historical September 28 case path in PLAYTHROUGH_RECORDING.md is no longer
present at its documented or archived location. A fresh synthetic stock case
now passes three complete 3,600-frame headless replays with exact auxiliary
invariance across separate steering trials; see `2026-10-08-stock-replay-proof.md`.
Claude's source cross-review passed for the split and accepted-zero startup.
Launcher rendering, Deluxe capture and attended feel remain pending. This
checkpoint is not installed and no physical force was sent.
