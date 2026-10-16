# Toolkit frame statistics

`dbce_frame_stats.hpp` is copied without changes from
`E:/Source/toolkits/dbce-wheel-mod-toolkit/native/panel/dbce_frame_stats.hpp`.

- Last source change: `bb46a5e5b2073543dc01d623ddebc791ca485996`.
- Raw SHA256: `30F7BE6756727A5242573BE0A8B914D139DBE2EE7AAB401D700716BFEB61D4A4`.
- License: MIT, copyright 2026 d-b-c-e, retained in
  `lib/toolkit/LICENSE.txt`.

The toolkit owns the ten-second window, average, slowest-one-percent mean,
worst-frame calculation and summary wording. `src/fzero_frame_stats.cpp`
adapts monotonic center-window completions to that class and invalidates the
displayed summary when the host resets or an invalid/gapped clock is observed.
It adds no SDL, renderer, device, force or network dependency to the toolkit.
