# Windows/x64 UCRT baseline contract: missing selection evidence

DBC-328 follow-up to 1e6749e, 2026-10-26. Source-only outcome.
`runtimeAllowed=false`; `windowsBaseline=null`; no preparation integration.

The retained evidence cannot select a named supported minimum Windows build.
It establishes an Intel/AMD x64 UCRT toolchain prerequisite, not an exact
consumer support floor or qualification of this machine. Windows 10 is a named
OS family with an inbox UCRT; it is not evidence that build 10240, every later
build, or every edition resolves this pinned consumer's complete imports.
No baseline is promoted in runtime-candidates.json.

## Evidence and requirements

The retained [toolchain metadata and hash](2026-10-26-runtime-dynamic-load-evidence.json)
identify gcc-16.2.0-mingw-w64ucrt-14.0.0-r1, Intel/AMD 64-bit, POSIX threads,
UCRT. The vendor permits Windows 10 or higher, or older supported systems with
the UCRT update, excluding pre-Windows 7 SP1 / Server 2008 R2 SP1. This broad
prerequisite does not select a product minimum or guarantee all linked APIs.
The x64 contract concerns these retained artifacts; ARM64 emulation and x86
are not qualified by that metadata.

[Microsoft UCRT deployment](https://learn.microsoft.com/en-us/cpp/windows/universal-crt-deployment?view=msvc-170)
states that Windows 10+ includes UCRT, and Windows 10/11 uses the system copy
even when a newer app-local UCRT is provided. Therefore copying UCRT beside the
consumer cannot establish the required system version. Older Windows needs a
separate deployment decision; the vendor's update option alone does not qualify it.

[Microsoft API sets](https://learn.microsoft.com/en-us/windows/win32/apiindex/windows-apisets)
describes API sets as contracts resolved by the device's schema. A matching DLL
filename on disk is not the resolution criterion. The retained
[import inventory](2026-10-26-consumer-loader-imports.json) spans the consumer
and three SHA-pinned runtime candidates and requires these 12 contracts:

- api-ms-win-crt-convert-l1-1-0
- api-ms-win-crt-environment-l1-1-0
- api-ms-win-crt-filesystem-l1-1-0
- api-ms-win-crt-heap-l1-1-0
- api-ms-win-crt-locale-l1-1-0
- api-ms-win-crt-math-l1-1-0
- api-ms-win-crt-private-l1-1-0
- api-ms-win-crt-runtime-l1-1-0
- api-ms-win-crt-stdio-l1-1-0
- api-ms-win-crt-string-l1-1-0
- api-ms-win-crt-time-l1-1-0
- api-ms-win-crt-utility-l1-1-0

The generic platform pages do not supply build-specific mappings and exported
symbols for this inventory, including crt-private. No special compatibility
claim is inferred from that contract's name. All ordinary Windows DLL imports,
forwarders and required symbols must also resolve. The reviewed optional
SetThreadDescription and precise-time lookups have fallback paths; their API
introduction dates alone do not impose a minimum build. See the retained
[dynamic-load review](2026-10-26-runtime-dynamic-load-review.md) for its limits.
Microsoft sources consulted 2026-10-26.

## Exact missing input and next independent action

A build-specific support decision needs an intended Windows client edition,
release/build and servicing revision (or an explicit supported range), x64,
and an authoritative image/source for its system module and API-set evidence.
For that chosen baseline, review schema-to-host mappings and required exports /
forwarders against the consumer and all three pinned DLLs. Record the identity
of the image and relevant system files. Neither a current host OS string nor
successful compile/link supplies that evidence. Full dynamic closure remains a
separate unfinished review; even a static resolution pass would not admit runtime.

Independent useful next source action: extend the existing read-only objdump
inventory to retain imported symbol names/ordinals per module for all four
artifacts. Reuse the existing SHA pins and tool identity. That provides the
concrete export-resolution checklist for whichever baseline is later selected,
without needing to load the consumer, query devices, or choose an arbitrary OS.
Do not add a launcher or repeat the passed compile suites to obtain it.

A later baseline record must distinguish declared requirements, source-reviewed
resolution, and actual host qualification. Fixed environment, DLL search and
foreign-module assumptions (including conditional DW2 loading), and use-time
ownership remain separate. No guessed PATH, SystemRoot or WINDIR is added.

## Validation and preserved state

Cross-checked the 12-contract union against the retained four-artifact import
inventory and the three-file candidate manifest; reviewed retained toolchain
metadata. Documentation/link and git diff checks only; no unchanged suite rerun,
target execution, runtime hook, or preparation change. No failed isolation
hypothesis; no new Archie escalation threshold reached.

Archie DBC-330 verdict, DBC-65 recovery, and Sandy rig ownership remain intact.
No live config/save/install/game/input/desktop/public push or player meaning
change. Ledger comparison: STD-001..040 represented, none missing; STD-019
unchecked, STD-021/031/038 pending, existing partial states unchanged. This
baseline evidence changes no adoption status.
