# F6 settings persistence / STD-044 audit

Kole #508099, 2026-11-03. Source base `840d2c1cf56e5eb10ed191784755db04cf76ca63`. Isolated worktree `E:/Source/_work/soba-fzero-f6-persistence-20261103`, branch `soba/fzero-f6-persistence-20261103`. Local source/fixture slice only; no shared Wheelkit dependency. No installed files, launcher settings, ROM/save data, game, UI, input, audio or device work.

## Actual save path and retained behavior

`src/sdl_main.c` closes the F6 overlay with an edited-row bitmask → `FzeroPanelSave` in `src/fzero_panel.c` → `FzeroPanelKeys` → `FzeroIniSaveKeys` / `ini_put` in `src/fzero_video.c`. The overlay uses this narrow writer, rather than the launcher's whole-file `FzeroVideoSave` path.

| Edited row | Owned video keys |
|---|---|
| Triple screens | `TripleScreen`, and `TripleOutputMode` when On; Off retains the saved output |
| Curve | `TripleCurveRadiusMm` |
| Widescreen | `EnhancedRenderer`, `Aspect` |
| Aspect | `Aspect` |
| Presentation FPS | `PresentationFPS`, `PresentationEnabled` |
| FPS counter | `ShowFPS` |
| Crash flash | `ReduceCrashFlash` |

All edited video keys share one replacement. Turning triples On then checks `[Graphics] Fullscreen` through `FzeroIniReadInt`; if off, it writes `Fullscreen=1` in `config.ini` and verifies through the same reader. This is **not** a two-file atomic transaction. Video failure restores the overlay's in-memory settings; fullscreen failure preserves the already-saved video choice and reports the partial outcome. The existing tests cover these distinctions and they remain unchanged.

## Bounded source fix

Before this slice, replacing an existing key copied bytes outside that key's line, but reconstructed the entire edited line as `Key=value`. An F6 save therefore deleted that line's indentation, spaces and trailing comment. The fix replaces only the scalar value token after `=`, keeping the prefix, trailing whitespace/comment, original line terminator and EOF bytes. Known callers write scalar tokens. Existing missing-key insertion, exact-case section/key matching and last matching video-key selection remain unchanged; this slice introduces no new parser or duplicate-key policy.

Focused regression coverage uses the real `FzeroPanelSave` path for mixed CRLF/LF, no final newline, indentation/tab spacing, semicolon/hash comments, an existing UTF-8 BOM before a comment, duplicate video keys with last-match semantics, and the fullscreen companion write/readback. It compares the entire resulting files and calls the native readers. The existing F9 video expectation now retains its original `TripleCompareView = ` spacing. Test execution remains pending separate CPU GO at this commit; source review is not a passing fixture result.

## Remaining gaps — STD-044 stays partial

- `FzeroIniSaveKeys` still uses the fixed sibling `<path>.tmp`, not an exclusive unique temp. Per-file replace/close checks exist, but collision/concurrent-writer admission and flush durability are not newly qualified. The full `FzeroVideoSave` path uses the same fixed-name pattern. This slice does not remove, overwrite or clean an existing owner temp file.
- `FzeroVideoSave` emits the whole known video schema and discards unknown content. It remains reachable from the launcher provider's `commit` (`src/fzero_mods.c`), launcher exit and guarded startup/aspect paths in `src/sdl_main.c`. Whole-file saves can therefore supersede F6's preservation in a later, separate operation. Move those paths to explicit owned-key transactions as a coherent follow-up; do not claim the product writer fully adopted STD-044 from this narrow fix.
- The narrow writer is byte-oriented and has a 64 KiB input limit. A BOM immediately before the first section, UTF-16, malformed/oversized lines, embedded NUL, ambiguous header suffixes and inconsistent duplicate semantics between video's last-match and launcher's first-match readers still require explicit admission/refusal qualification. An existing BOM before a comment in the new fixture is only that exact case, not general encoding support.
- Inline comments separated from a valid scalar and all bytes outside that scalar are preserved. This does not broaden the video reader's accepted value grammar; adjacent comment markers without whitespace may remain invalid to that reader.
- Public `FzeroIniSaveKeys` input validation and malformed-file refusal are not exhaustively qualified. Fixed-width header/value limits and per-call error reporting remain as before.
- Fullscreen remains a separate write with a surfaced partial failure. Lowercase/duplicate launcher keys retain the existing first-reader/exact-writer caveat and existing negative fixture; changing it needs a consumer-aligned follow-up, not a silent case-policy change here.

Next bounded writer increment: an exclusive unique sibling temp and source-identity admission, followed by converting whole-file launcher/aspect saves to explicit ownership. Reuse existing tests and retain the distinct video/config readers. No new Wheelkit work is needed for this game-side repair.

## Qualification boundary

Planned CPU check: compile the unchanged original writer with the new panel regression to establish a red witness; compile the changed writer and run the existing panel/video fixture executables, one worker, separate synthetic files only. No game binary or package build. Record the exact GO, candidate, logs and process cleanup before any release claim. Existing historical F6/F9 source or rig results are not rerun or promoted by this audit. Current rendered F6 interaction, restart/EOF/encoding behavior beyond fixtures, owner settings/metadata restoration and owner acceptance remain unqualified.
