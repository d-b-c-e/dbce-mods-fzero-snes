# F6 settings persistence / STD-044 audit

Kole #508099, 2026-11-03. Source base `840d2c1cf56e5eb10ed191784755db04cf76ca63`. Qualified branch `soba/fzero-f6-persistence-20261103` was fast-forwarded to local main at `50126ee86815289199ce13d859fd319b947cfeac` under #508324, without repeating tests or pushing. Its finished worktree is retired after verified evidence relocation below. Local source/fixture slice only; no shared Wheelkit dependency. No installed files, launcher settings, ROM/save data, game, UI, input, audio or device work.

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

Focused regression coverage uses the real `FzeroPanelSave` path for mixed CRLF/LF, no final newline, indentation/tab spacing, semicolon/hash comments, an existing UTF-8 BOM before a comment, duplicate video keys with last-match semantics, and the fullscreen companion write/readback. It compares the entire resulting files and calls the native readers. The existing F9 video expectation now retains its original `TripleCompareView = ` spacing. CPU GO508203 qualified source `b547095225f956caae36d1fac58d8c8677b15792`: original writer failed the new exact-byte check, fixed panel and existing video fixtures passed. See the evidence record below.

## Remaining gaps — STD-044 stays partial

- `FzeroIniSaveKeys` still uses the fixed sibling `<path>.tmp`, not an exclusive unique temp; its `fopen` can overwrite a preexisting temp. Per-file replace/close checks exist, but collision/concurrent-writer admission and flush durability are not newly qualified. The full `FzeroVideoSave` path uses the same fixed-name pattern. This work touched only synthetic test files, not any owner temp or config.
- `FzeroVideoSave` emits the whole known video schema and discards unknown content. It remains reachable from the launcher provider's `commit` (`src/fzero_mods.c`), launcher exit and guarded startup/aspect paths in `src/sdl_main.c`. Whole-file saves can therefore supersede F6's preservation in a later, separate operation. Move those paths to explicit owned-key transactions as a coherent follow-up; do not claim the product writer fully adopted STD-044 from this narrow fix.
- The narrow writer is byte-oriented and has a 64 KiB input limit. A BOM immediately before the first section, UTF-16, malformed/oversized lines, embedded NUL, ambiguous header suffixes and inconsistent duplicate semantics between video's last-match and launcher's first-match readers still require explicit admission/refusal qualification. An existing BOM before a comment in the new fixture is only that exact case, not general encoding support.
- Inline comments separated from a valid scalar and all bytes outside that scalar are preserved. This does not broaden the video reader's accepted value grammar; adjacent comment markers without whitespace may remain invalid to that reader.
- Public `FzeroIniSaveKeys` input validation and malformed-file refusal are not exhaustively qualified. Fixed-width header/value limits and per-call error reporting remain as before.
- Fullscreen remains a separate write with a surfaced partial failure. Lowercase/duplicate launcher keys retain the existing first-reader/exact-writer caveat and existing negative fixture; changing it needs a consumer-aligned follow-up, not a silent case-policy change here.

Next bounded writer increment: an exclusive unique sibling temp and source-identity admission, followed by converting whole-file launcher/aspect saves to explicit ownership. Reuse existing tests and retain the distinct video/config readers. No new Wheelkit work is needed for this game-side repair.

## Qualification boundary

CPU GO **508203** (approval delivery #508206): one worker, three sequential GCC `-std=c11 -Wall -Wextra -Werror` compiles and three direct fixture executions, **11.8704 seconds**. The original source exported from `840d2c1` compiled with the new production-panel fixture and exited 1 at `tests/test_panel.c:62`, `file_is("test-panel-preservation.ini", cases[i].after)`. Its failed file is retained as `baseline-witness.ini`. Both fixed executables exited 0: “F-Zero F6 panel model passed” and “F-Zero video: viewport, anchors, clock, and config checks passed”. All compiler stderr logs were empty.

Raw evidence is retained at `E:/Source/game-modding/dbce-mods-fzero-snes/.local/retained-evidence/f6-persistence-soba-20261103/`: frozen `Run-Cpu.ps1` SHA-256 `632FA58B18073F301062391C2D8B464CF28255379B327E0B1FC2140EB949A028`, `plan.json` `55B3E601E3981010916D6B21DDD538D14BCEBEE78F423C56381A92941E1C779F`, original writer export, and `cpu-508203/` logs/binaries/result/audit/closure. All 22 files were moved from the finished worktree's `.local/f6-persistence-cpu/` and independently read back for SHA-256, length, last-write UTC ticks and attributes; `relocation.json` records the mapping and manifest. Original absolute paths inside frozen evidence are historical and were not rewritten. Runner result SHA-256 `07C495ECAFEFE4A6E75D4D620832037FF921E717967440A367FAE5E19501F1D3`.

Six direct roots had held handles and normal exits (including the expected-red exit 1), with observed descendants tracked by the existing Stage-OwnedProcess helper. Full descendant-job containment remains **unqualified**. Independent OS scoped audit confirmed all 19 observed original identities absent and zero scoped processes; audit SHA-256 `5AEEC545CEBA38BA37975BA956F29D7879D4814D64646DFB4D313F0EC5ABBD4C`. The exact named lease `soba-fzero-f6-cpu-508203`, token suffix `ecea0ec2ad0b4c96bea4b89b9dfe0f07`, was then released at `2026-11-04T01:16:44.2817827Z`; no custody remains. The entire lease interval was under the 180-second ceiling; exact timestamps remain in the raw audit/closure.

No game binary or package build, owner/installed settings write, UI/input/device or public push. Existing historical F6/F9 source or rig results were not rerun or promoted. Current rendered F6 interaction, restart/EOF/encoding behavior beyond fixtures, owner settings/metadata restoration and owner acceptance remain unqualified.
