# Compile-only SDL consumer admission

DBC-328 follow-up to `36aa178`: the consumer previously compared archive/config
bytes with mutable generated provenance only. It did not recheck source bytes,
the driver allowlists, or later incremental builds.

`tools/sdl-virtual-only/Admit.cmake` now runs before `enable_language` and as an
always-run dependency of the consumer target. `approved-admission.json` pins the
already recorded archive/config hashes, original full-source manifest hash, and
patched-source hashes from `.local/sdl-reviewed`. Every source file is checked
against that manifest with the approved patch overrides; added files also refuse.
These pins identify the retained cache artifact, not independent upstream trust.

The gate checks `runtimeAllowed=false`, exact DUMMY+VIRTUAL joystick macros,
the six approved compiled joystick sources, and the imported Release archive's
resolved path against the local archive and recorded provenance. A single-config
Release build is required. New artifact hashes require explicit source review
and a pin update; regenerating mutable provenance cannot silently admit them.

Validation: clean configure admitted before GNU compiler detection; production
consumer compile/link passed (nine build steps). Incremental builds refused
source, config, archive bytes, archive provenance, source manifest, driver source,
link path and runtime flag drift before consumer compilation. A fresh configure
with source drift refused before compiler detection. All modified copied
dependency files were restored; restored build passed. Logs are retained under
`.local/sdl-admission-evidence`. No duplicate planner tests were added.

The existing GCC warning in `raw_snapshot` remains: its hat loop uses the device
count to index a four-entry array. The next unfinished same-game source outcome
is to reconcile the established count bounds and fix a demonstrated overflow in
that reader under a separate source assignment. This change does not modify it.

No fixture execution or runtime hook. DBC-330 source recheck remains separate;
DBC-65 recovery and Sandy's rig ownership remain intact. No live configuration,
save, installation, game, input, desktop or public push; no player meaning change.
Ledger STD-001..040 are represented. STD-019 remains unchecked; STD-021/031/038
pending; STD-037 partial. No adoption or acceptance is promoted.
