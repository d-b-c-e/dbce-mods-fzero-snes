# Launcher and narrow settings saves: exclusive temporary files

Kole #508324, 2026-11-03. Source/fixture slice based on clean local main `c79e0b5780337dd9462b370fec746cf41d428bc7`, after integrating the qualified F6 scalar preservation fix. Worktree `E:/Source/_work/soba-fzero-launcher-persistence-20261103`, branch `soba/fzero-launcher-persistence-20261103`. No installed files, owner state, game, UI, devices or public push. CPU execution requires a separate GO.

## Concrete defect and change

Both the launcher's whole-file `FzeroVideoSave` and the panel/F9 narrow `FzeroIniSaveKeys` previously opened `<destination>.tmp` with ordinary write mode. If a stale, rollback or concurrent writer's file already occupied that path, the save truncated it; replacement then consumed that path. Failure cleanup could remove it as well.

Both writers now reserve their own sibling `<destination>.tmp.<pid>.<sequence>` with C11 exclusive `x` creation before writing. The sequence increment is atomic (Windows Interlocked / POSIX C11 atomics); an occupied candidate is skipped without modifying it, with at most 32 attempts per call. Any other open error or overlong name refuses the save. Only a successfully created candidate is eligible for the caller's existing failure cleanup. The full writer retains its text mode and the narrow writer its binary mode; final replace and existing error reporting remain unchanged.

Microsoft documents `x` as failure when the file exists, equivalent to `_O_EXCL`, in its [CRT fopen reference](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen?view=msvc-170). Qualification must exercise that behavior on the actual local toolchain; the API reference is not a passing test.

## Focused fixtures, pending CPU GO

The existing video executable calls a new temporary-file regression before its other save cases. It creates both a legacy `.tmp` marker and the first candidate name, then invokes the actual full writer. Both markers must remain byte-exact, the saved config must read back, and the successful candidate must be consumed by replacement. The same test invokes the actual narrow writer and checks both markers and readback again. On Windows, readonly destination failures through both writers must preserve the destination and both foreign markers, and a scoped filename enumeration must show no leftover candidate owned by the test saves. Fixture cleanup removes only the synthetic files it created.

Planned CPU: original writer from `c79e0b5` with the new video regression must fail the legacy-marker preservation check; changed video and existing panel executables must pass. Three sequential compiler invocations and three direct fixture roots, one worker, synthetic files only, with exact candidate/source pins and independent scoped process audit before release. No full game/package/CMake build or replay. Existing F6 scalar evidence is retained and will not be rerun beyond the affected panel fixture.

## STD-044 remains partial

This fixes ownership of the temporary file, not concurrent edits to the destination. There is no new source-identity compare-and-swap, directory durability guarantee, source-file locking, metadata/ACL preservation claim or cross-file transaction. PID/sequence names are not secrets; exclusivity, bounded retry and refusal prevent overwriting a foreign candidate. Arbitrary long paths may now refuse when the longer suffix cannot fit the existing 1024-byte temporary buffer.

The launcher's whole-file serializer still materializes the known schema and loses unknown keys/comments. Explicit dirty-key ownership across launcher commit/exit and guarded startup/aspect remains a separate increment. The parser still needs encoding/BOM/NUL/long-line/header admission and consumer-aligned duplicate rules. The F6 video-then-fullscreen partial outcome and all defaults, mode/framing values, force tuning and reader semantics remain unchanged.

Next after this bounded slice: define a launcher snapshot/edited-key transaction so unchanged/missing/default fields are not materialized, preserve unknown source bytes, and refuse ambiguous/unsupported input before writing. Reuse the game’s actual distinct video/launcher readers. No Wheelkit work is needed. Fixture passes, local integration, install, rendered launcher/F6 behavior, device behavior and owner acceptance remain separate evidence levels.
