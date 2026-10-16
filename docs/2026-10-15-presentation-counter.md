# Measured presentation counter

## Source baseline

This isolated source candidate follows `149d914` (bounded Shared config read)
on clean recovered baseline `796cfeb137260b5c3a4747650e5ce86478186903`.
It addresses the missing measured frame-rate display tracked by STD-023/024.
The canonical game checkout, private assets, existing installed payloads and
saved owner settings are not development targets.

## Player behavior

The existing launcher's **Mods > Presentation FPS** feature gains **Show
measured frame rate**, default off. It saves `ShowFPS=0|1` in the existing
`fzero-video.ini`; an absent key retains the previous no-counter appearance.
The counter can stay enabled while the presentation-rate override is off.
Its boolean option does not alter simulation speed, frame scheduling, force,
input, camera geometry or recording data.

When enabled, the counter appears at the bottom of the center game view,
using the host's existing overlay renderer. It says **Collecting 10 seconds
of presentation timings** until a complete summary exists, then shows
average FPS, 1% low and worst-frame milliseconds. The label identifies
experimental separate windows or the experimental three-panel span when
actually selected by the presenter. It does not promote experimental triples.

The first complete summary is also written to stderr with
`[fzero-frame-rate]`. Subsequent summary logging is bounded to no more than
once every 30 seconds. This uses the game's existing log destination and
does not create another report/log directory.

## Toolkit reuse and host integration

`src/vendor/panel/dbce_frame_stats.hpp` is an unchanged copy of toolkit
commit `bb46a5e5b2073543dc01d623ddebc791ca485996`, raw SHA256
`30F7BE6756727A5242573BE0A8B914D139DBE2EE7AAB401D700716BFEB61D4A4`.
The MIT license is already retained at `lib/toolkit/LICENSE.txt`; provenance
is recorded beside the new header and a targeted attribute preserves its
raw bytes. No toolkit source was modified.

The small C ABI adapter passes measured monotonic intervals to the toolkit's
`FrameWindow` and uses its existing `FormatFrameLine`. The toolkit owns the
fixed buffer, summary calculation and text. The adapter invalidates both
partial and displayed summaries on explicit reset, invalid/backward/equal
timestamps or gaps exceeding the existing five-second toolkit cutoff.

The SDL host samples **one completed center-window presentation per call**
in either GL or SDL-renderer mode. Side-window swaps and simulation ticks
never contribute extra samples. Pausing/minimizing/loading uses the existing
clock-reset path; output dimensions/mode and target-rate changes also reset
the window. The counter uses the existing center-view rectangle. A separate
fixed-size SDL texture avoids repeatedly reallocating the existing session/
menu texture when both overlays are visible; teardown releases it normally.

These are CPU observations of presentation calls. They do not establish
physical scanout, dropped/composited frames or a performance improvement.
Existing richer opt-in timing diagnostics remain available separately.

## Authored checks and pending qualification

No compiler, fixture, game, display action or installation ran for this
source candidate. Source diff/whitespace inspection and raw vendor-byte
comparison are the only current checks.

- New ROM-free `fzero_frame_stats` fixture: exact ten-second cadence,
  average/low/worst metrics, reset and invalid clocks, no stale summary,
  display caption and bounded output.
- Existing `fzero_video` fixture: default off, ShowFPS round-trip, invalid
  values refused and absent legacy key retained as off.
- Existing `fzero_mods` provider fixture: boolean option presentation,
  strict values, feature routing and committed reload, independent of the
  presentation-rate toggle.
- Pending focused build also includes `149d914`'s four ordinary Shared
  boundary/oversize host cases and existing normal/extras/BOM regressions.

Use a named serialized slot for one focused ROM-free compile/fixture batch
and the needed graphical target/provider compilation. A later separately
granted muted graphical observation must confirm counter rendering in the
chosen GL/SDL path and center placement. Source is not rendered or physical
acceptance. No broad/native force suite repeat is proposed.

The shared launcher input-owner proposal belongs to Beni. Its SDL device
enumeration/bind-capture path, the explicit Shared launcher refusal and the
cross-toolkit force bridge remain outside this counter implementation.
