# Device-free FFB component comparison

October 8 adds an [independent steering candidate](2026-10-08-steering-strength.md).
The historical combined-strength numbers below stay valid for the legacy model.
An optional third trace argument varies steering alone, retaining the second
argument's damping/road level. Preserve both arguments with any comparison.

Build with the normal CMake/MSVC environment, then choose a new output path:

```powershell
cmake --build build-merge --target fzero_force_trace fzero_ffb_tests fzero_ffb_lifecycle_tests
ctest --test-dir build-merge -R '^fzero_ffb(_lifecycle)?$' --output-on-failure
./build-merge/fzero_force_trace.exe $newCsv 50
```

The trace compiles the real FzeroFfbCompute with `FZERO_FFB_MODEL_ONLY`, excluding
native DLL loading, discovery and device output. The ordinary model test now
uses that same compile definition; previously it included a Windows discovery/
missing-device smoke despite its name. The separate lifecycle test uses fake APIs.
Normal game builds do not define this flag and retain their existing behavior.

601 synthetic rows cover stopped, straight, left/right, rough surface, a single
collision event and stop. No ROM or game runs. The output file is exclusive and
cannot overwrite a previous result. This is a component-response fixture, not
a recorded stock or Deluxe playthrough.

At Strength50 the maximum spring coefficient is0.50, damper0.20, and road
amplitude0.20 normal /0.35 rough. At saved-default40 these become0.40,0.16,
0.16/0.28. The constant column is a **fallback** when spring is absent, not an
additional simultaneous force. The collision column is a boolean event edge,
not a magnitude. Keep all of these distinct in the shared
[FFB comparison](https://github.com/d-b-c-e/dbce-wheel-mod-toolkit/blob/master/docs/FFB-NORMALIZATION.md).
World units per frame are not a calibrated speed in m/s. A one-frame move of
more than 32 units (a save-state load or rewind) holds speed and skips that
frame's collision check, the same bound telemetry uses.

Condition coefficients cannot be equated to constant-force torque without wheel
displacement/velocity and device-response information. No strength/default or
installed settings were changed. A validated recorded reference, Deluxe identity
where applicable, and attended comparison remain before claiming STD-003.

## Strength 50 beside Art of Rally 50 (2026-10-22)

F-Zero values come from a fresh `fzero_force_trace` run at `50` (601 synthetic
rows; maxima are the real `FzeroFfbComputeSteering` output, divided by 10000).
Art values are from the toolkit
[`art-of-rally-50-v2.json`](https://github.com/d-b-c-e/dbce-wheel-mod-toolkit/blob/master/profiles/normalization/art-of-rally-50-v2.json)
(Art `8bc94b1`; steering gain 1, reference 19166.67 N, smoothing 0.2) and
[FFB-NORMALIZATION.md](https://github.com/d-b-c-e/dbce-wheel-mod-toolkit/blob/master/docs/FFB-NORMALIZATION.md).
Neither column is measured torque; the rows are different effect families.

| Component | F-Zero at 50 (trace maximum) | Art of rally at 50 | Comparable? |
|---|---|---|---|
| Steering / centring | Spring coefficient 0.50 (condition effect) | Signed constant-force request in [-1,1] from AxleForceCurve; no spring | No: coefficient vs force request |
| Steering fallback | Constant +/-0.275, only if spring is unavailable | (steering is the constant request itself) | Not additive with spring |
| Damper | Coefficient 0.20 | None | Art has no counterpart: unavailable, not zero |
| Road texture | Amplitude 0.20 normal, 0.35 rough | None in steering CSV; landing 0.05 sine, 25 Hz, 120 ms (on) | Different cue; not the same effect |
| Collision | Boolean edge, one event at 8.5 s; no magnitude | Crash push 0.50 constant 120 ms + rattle 0.25 sine, 25 Hz, 250 ms (crash cue off by default) | No: F-Zero has no amplitude to compare |

Limits. Spring and damper are DirectInput **condition** effects: the base
computes torque from physical wheel displacement and velocity times the
coefficient, so the delivered force is unknown until those are measured. Art's
steering and crash push are **constant-force** requests whose magnitude is the
request itself (times the base's own scaling). A condition coefficient cannot be
converted to a constant-force value without that data, and F-Zero speed is
world units per frame, not m/s. Do not sum spring, damper, road and collision
into one claimed torque, and do not multiply clipped outputs.

### Still missing for a physical match

1. Wheel displacement (steering angle vs centre) and velocity during an F-Zero
   drive, to turn spring/damper coefficients into torque. **Owner must supply:**
   a muted recording of the wheel's steering angle and velocity (with the base's
   active profile) over a matching F-Zero drive and Art drive.
2. Device response: measured torque (or an equivalent calibrated sensor
   output) for condition coefficients and for Art's constant requests on the
   same base and saved profile, including saved strength/torque percentages.
3. A recorded stock or Deluxe F-Zero reference drive (identity established), so
   corners, release and impacts are not synthetic.
4. An F-Zero collision severity input, if a collision magnitude is wanted.

The single input the owner must supply first is item 1, the wheel
displacement/velocity capture. No force default, saved setting or installed file
changed; `fzero_ffb` and `fzero_ffb_lifecycle` ctests pass.
