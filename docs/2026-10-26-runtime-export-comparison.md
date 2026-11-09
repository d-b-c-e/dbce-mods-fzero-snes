# Pinned app-local import/export comparison

DBC-328 follow-up to a9ace2c, 2026-10-26. Source-only byte inspection.
`windowsBaseline=null`; `runtimeAllowed=false`. No preparation integration.

All **108 app-local static import entries resolve** to concrete exports in the
three retained runtime candidates. Missing exports: **0**. All **491 external
Windows import entries remain unresolved evidence**, each recorded separately.
This is static symbol availability, not Windows qualification, dynamic closure,
execution permission or proof of loader behavior.

| Pinned DLL | Export slots | Named exports | Forwarders |
|---|---:|---:|---:|
| libgcc_s_seh-1.dll | 128 | 128 | 0 |
| libstdc++-6.dll | 7060 | 7060 | 0 |
| libwinpthread-1.dll | 156 | 156 | 0 |

The JSON report binds the import inventory, runtime manifest, comparator and
every consulted artifact by SHA256. The existing import reader rechecks all
four artifact identities and reconciles preparation/runtime pins and the older
DLL-level inventory before comparison. Retained symbol inventory must equal
fresh decoding exactly. No host DLL or current Windows installation is consulted.

Each app-local result records its importing artifact hash, original module and
symbol, status, and export chain. Each chain step records the selected module
hash, requested name/ordinal, actual export ordinal, RVA, aliases and forwarder.
Exported data in zero-filled mapped sections is valid; tables and strings must
be file-backed. A concrete export is not necessarily a function.

Names match exact case-sensitive spelling; module names normalize case. Hints
are retained as evidence and are not treated as ordinal requirements. Forwarders
resolve recursively by name or `#ordinal`, retaining all steps and refusing to
claim resolution for a cycle or a module outside the pinned dataset. The actual
three DLLs contain no forwarders; synthetic checks cover these resolver branches.

Statuses preserve the previous comparison contract:

- `resolved`: complete pinned chain ends at a mapped concrete export.
- `missing-export`: the exact pinned module has no matching named/ordinal export.
- `unresolved-evidence`: external Windows evidence is absent, a forwarder leaves
  the pinned dataset, or a forwarder cycle prevents a terminal resolution.
- `missing-module` requires a complete selected-image dataset proving absence.
  This bounded dataset supplies no such evidence, so the comparator never labels
  an external or absent candidate module missing merely because it lacks bytes.

## Reproduce

Use RUNTIME_BIN from the compiler path in the retained consumer CMakeCache:

```text
python tools/sdl-virtual-only/compare_exports.py --consumer .local/sdl-admission-consumer/fzero_virtual_consumer.exe --runtime-dir RUNTIME_BIN --output docs/2026-10-26-runtime-export-comparison.json
python tools/sdl-virtual-only/check_exports.py .local/sdl-admission-consumer/fzero_virtual_consumer.exe RUNTIME_BIN
```

Focused checks PASS: deterministic regeneration; all 7,344 export names,
ordinals and RVAs independently match the existing SHA-pinned objdump; one-byte
mutation of each of four disposable artifacts is refused; restored copies match;
scratch cleanup completes. Synthetic checks distinguish named/ordinal forwarders,
exact-case missing export, absent ordinal, external evidence and cycles. Initial
parser refusal of a legitimate zero-filled data export was corrected before the
passing run. `git diff --check` passes. No unchanged compile suite rerun.

Next useful feature outcome: consume an explicitly selected, provenance-bound
Windows x64 image/API-set/export dataset to resolve the remaining 491 entries
and forwarder chains. Required inputs remain intended edition, build, servicing
revision and authoritative image bytes/mappings. The comparison interface now
provides the exact symbol requirements; selecting or guessing a baseline is
outside this outcome. Minimum environment and use-time ownership remain separate.

Archie DBC-330, DBC-65 recovery and Sandy rig ownership remain preserved. No target
execution, preparation integration, runtime hook, live config/save/install/game/
input/desktop/public push or player meaning change. STD-001..040 represented;
STD-019 unchecked, STD-021/031/038 pending and partial adoption states unchanged.
No isolation hypothesis failed in this outcome.
