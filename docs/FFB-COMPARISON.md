# Device-free FFB component comparison

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
World units per frame are not a calibrated speed in m/s.

Condition coefficients cannot be equated to constant-force torque without wheel
displacement/velocity and device-response information. No strength/default or
installed settings were changed. A validated recorded reference, Deluxe identity
where applicable, and attended comparison remain before claiming STD-003.
