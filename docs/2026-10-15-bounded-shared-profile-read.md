# Bound the Shared profile stream read

## Baseline and bug

Source baseline is clean local `main`
`796cfeb137260b5c3a4747650e5ce86478186903`. Work is isolated on
`dine/fzero-bounded-profile-20261015`; the canonical checkout and its ignored
private assets/builds remain in place.

The existing `read_profile` constructed a string from `istreambuf_iterator`
through EOF, then checked the 1 MiB config limit. An oversized configuration
therefore caused an unbounded read/allocation before the intended refusal.
Both ordinary Shared and the diagnostic call this function before provider
open. This finding comes from source inspection; no oversized owner config
or runtime failure is asserted.

## Change

The same binary stream now reads at most **1 MiB plus one byte** into a fixed
size string. Read errors refuse. The extra byte detects an oversized file;
the accepted limit stays inclusive at 1 MiB. EOF at or below that limit is
accepted for the existing section scan, including files without a final
newline. No file-size precheck or truncated prefix is used for admission.

The existing bounded Controls/Extras section scans, BOM handling and exact
section bytes are retained. The pinned toolkit Core still owns binding
validation, device identity, calibration and one combined extras tick. This
change adds no binding parser, dependency, ABI, configuration key or receipt
format. Native/replay admission, explicit-ROM requirement, Shared force
refusal, guest input mapping and recording format retain their existing paths.

## Authored verification; not executed

Four cases extend the existing `fzero_shim_ordinary_tests` ABI-fake host:

| Case | Intended check |
| --- | --- |
| `configlimit` | A config exactly 1 MiB with no final newline still opens; the fake receives exact Controls bytes and the existing consumed-word tape check passes. |
| `extrasconfiglimit` | The same boundary preserves exact Controls and Extras bytes, declaration and single combined ticks. |
| `configoversize` | A config 1 MiB plus one byte refuses before provider open, preserves force-off ownership and leaves config bytes unchanged. |
| `extrasconfigoversize` | The same refusal applies with the declared extras opt-in and valid section prefix. |

No build or fixture command has run for this change. A named serialized slot
is required for the existing ROM-free Release configuration, its
`fzero_shim_ordinary_tests` target and these four CTest cases. Existing
`normal`, `extras` and `extrasbom` cases provide the small-file and BOM
regressions; the wider suite, graphical game and native DLL need no automatic
repeat for this stream-bound change.

This source work does not qualify a graphical consumer, physical input/force,
installation, public distribution or rendered triples. The private pin stays
`ea568c2` / `BF6455E6` (base ABI 2, extras ABI 1); launcher preview, force
ownership and all existing delivery gaps remain separately tracked.
