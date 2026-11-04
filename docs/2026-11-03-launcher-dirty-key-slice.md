# Next STD-044 slice: launcher video dirty-key ownership

Sizing/pick for kole #508735, 2026-11-03. Source base `52d675b1958fce6457b2930beeb700cb6851a702`, clean isolated worktree `E:/Source/_work/soba-fzero-launcher-dirty-keys-20261103`, branch `soba/fzero-launcher-dirty-keys-20261103`. **Picked, not implemented or tested yet.** The accepted admission/temp/F6 slices are on local main with verified archives and their finished worktrees retired. No Wheelkit dependency, CPU, runtime, installed/owner state or public push for this pick.

## Why this is a bounded source/fixture increment

Only four product call sites still use the full `FzeroVideoSave` serializer: provider `commit` in `src/fzero_mods.c`, and launcher exit, guarded stock-aspect initialization, and Ctrl-F6/F7 presentation shortcuts in `src/sdl_main.c`. The known video fields and scalar serializers live in `src/fzero_video.c` / `.h`. Existing `tests/test_video.c` and **real provider** `tests/test_mods.c` exercise this policy without a game or SDL window; CMake already has the latter's game-independent provider target.

One typed before/after delta helper plus these four callers is a coherent slice. A destination identity/locking transaction is separate: there is no new compare-and-swap guarantee in the dirty-key change.

## Proposed implementation boundary

1. Add an explicit before/after video save entry point (for example `FzeroVideoSaveChanges`) whose reviewed field-to-key inventory matches the current video schema. Compare fields by type, not whole-struct padding or disk defaults. Serialize only fields whose values changed. An empty delta returns success with no write or file creation, including absent configs.
2. Pass the complete owned-key delta through one existing admitted, exclusive-temp narrow transaction. Keep unknown keys, comments, missing optional/default keys, unrelated per-line endings/EOF and saved F9/eye-height values untouched when they were not edited. Do not call the full serializer as a fallback. Creation with a nonempty delta writes only its explicit keys; retain existing defaults through reader semantics rather than materializing them.
3. Capture the provider's opening video values when it is created; advance that baseline only after successful video commit. Preserve its wheel/profile/FFB save ordering and current failure reporting. A failed video save must not advance the baseline or start wheel/profile writes. A successful repeated provider commit with no new video edits must not rewrite video bytes.
4. Capture values immediately before each local startup/aspect or shortcut mutation, and before launcher editing for the exit path. Route all four product callers through the delta entry point. Provider commit followed by launcher exit may re-request the same edited keys; the final write policy should avoid rewriting already-equal owned values while retaining unowned source bytes. Do not broaden the full serializer's product reach or change hotkey, rendering, profile or output behavior.
5. Preserve each scalar's established representation/consumer meaning, including Auto FPS, HD scale validation/fallback, output mode and the F9 zero/none state. Resolve these explicitly against the actual reader and existing fixtures; do not infer equality solely from a missing on-disk key. Do not add an unrelated default migration or change owner choices.

## Focused qualification after separate CPU GO

Extend existing production video/provider fixtures: no edits preserve an absent/existing file; one explicit option changes only its owned scalar; unknown/comment/formatting/missing-default bytes remain exact; multiple edits coalesce; repeated commit is a no-op; failed save preserves file and provider baseline and suppresses later wheel/profile writes; successful retry uses the same requested delta. Cover Play/provider commit then launcher-exit semantics and the two local shortcut/startup policies through source-linked pure seams where needed. Retain and run the affected existing F6/temp/admission cases; do not repeat a full passing suite or build a game/ROM package merely to test the delta.

The red witness should be the current full-save path materializing unrelated fields or losing unknown text, not an invented mock failure. Pin the exact source/compiler/fixture/runner and use the established bounded CPU/lease/audit route only under a separate GO. `sdl_main.c` full-host compilation/rendering and owner acceptance remain distinct from pure fixture qualification.

## Remaining work

Dirty-key ownership limits the write set; it does not resolve another writer changing an edited key between capture/read/replace. Destination snapshot identity, stale-source refusal and the final replacement race need an explicitly scoped transaction design and platform qualification afterward. Broader INI grammar, metadata/ACL/durability and launcher `config.ini` wheel/FFB ownership remain separate. STD-044 stays partial; do not promote the grid or reuse this sizing note as implementation/runtime evidence.
