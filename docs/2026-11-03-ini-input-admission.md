# Settings-save input admission: binary, UTF-16 and size refusal

Kole #508529, 2026-11-03. Bounded game-side source/fixture slice on `eee32b8576791e5a88054caa0a3cab6193d91ae1`, after local integration of the qualified exclusive-temp slice. Worktree `E:/Source/_work/soba-fzero-ini-admission-20261103`, branch `soba/fzero-ini-admission-20261103`. CPU is separately requested; no tests have run for this candidate yet. No installed/owner files, UI, game, devices or public push.

## Defect and chosen increment

The native video/launcher readers consume byte-oriented INI. Previously `FzeroVideoSave` overwrote existing files without reading/admitting their representation, and `FzeroIniSaveKeys` accepted binary/NUL-bearing or UTF-16 data even though its parser did not implement those representations. The narrow writer already had a 64 KiB input limit, while the full writer had none.

A shared bounded binary read now precedes both writers. An absent file still means an empty source. Existing input is refused on read/close error, size above 64 KiB, embedded NUL, or either UTF-16 BOM (including a BOM-only file). Refusal happens before reserving a temporary file; both the source and any foreign temp remain untouched. The narrow writer reuses these admitted bytes for its patch; the full writer discards the read buffer after admission and retains its existing serialization behavior.

This is representation admission, **not** a new INI grammar or strict UTF-8 validator. Existing UTF-8 BOM-before-comment, per-line endings/EOF, scalar writes, legacy sectionless values, exact video key matching and distinct launcher first-match behavior are unchanged. Empty/missing files remain eligible for normal creation. Unsupported input returns the existing save-failure result and follows current caller reporting/rollback; no silent conversion or reset is attempted.

## Focused fixture, pending CPU GO

The existing video executable first writes four synthetic invalid inputs: an embedded NUL in otherwise INI-like bytes, UTF-16LE BOM only, UTF-16BE bytes, and 65,537 bytes. Each actual full and narrow save must return false, with an exact byte comparison after each refusal. The existing collision/readonly, video/F9 and panel/fullscreen cases remain the affected regression checks; the panel fixture already contains valid UTF-8 BOM-before-comment and mixed-ending preservation cases.

Planned qualification: original writer from `eee32b8` with new video fixture must fail the first full-save refusal assertion; changed video and existing panel fixtures must pass. Three sequential GCC compiles/three direct fixture roots, one worker, copied synthetic files only, with separate CPU GO and independent all-image scoped audit before exact release. No full game/package/CMake build or runtime action. Failed original output will be retained as a byte witness.

## Remaining STD-044 boundaries

Status stays partial. Source admission and replacement are not a destination compare-and-swap: a later concurrent edit is still a separate gap. Whole-file launcher/aspect saves still materialize known defaults and discard unknown valid text; explicit snapshot/dirty-key ownership requires a coherent caller transaction. Header suffixes/duplicates, long lines, malformed scalar grammar and general encoding policy remain unqualified; this slice does not claim all malformed input is refused. Valid arbitrary non-NUL byte text is not newly decoded or normalized by admission.

The exclusive temporary-file behavior and F6 scalar preservation remain as already qualified. Directory durability, metadata/ACL preservation, the two-file video/fullscreen partial outcome, rendered behavior, installation and owner/device acceptance remain separate. No Wheelkit dependency was found. Next increment is the explicit launcher snapshot/dirty-key plan with destination identity checks, sized independently rather than inferred from a bounded read.
