# Pinned consumer import symbols and missing-export comparison

DBC-328 follow-up to bc3ce5f. Read-only artifact inspection, 2026-10-26.
`windowsBaseline=null`; `runtimeAllowed=false`. No preparation integration.

[Symbol inventory](2026-10-26-consumer-import-symbols.json) records every normal
import descriptor and original thunk in all four retained AMD64 PE32+ artifacts.
The reader checks SHA256 against both the existing DLL-level inventory and the
current preparation/runtime candidate pins before decoding. It rejects nonzero
delay directories, missing original thunk tables and bound imports rather than
silently treating them as ordinary imports. No module is loaded.

| Artifact | DLL descriptors | Named imports | Ordinal imports |
|---|---:|---:|---:|
| fzero_virtual_consumer.exe | 20 | 308 | 0 |
| libgcc_s_seh-1.dll | 7 | 42 | 0 |
| libstdc++-6.dll | 15 | 168 | 0 |
| libwinpthread-1.dll | 7 | 81 | 0 |

Each symbol retains `name`, `ordinal` and `hint`. The hint is an export-name-table
lookup accelerator, **not an ordinal requirement**. Named exports must match the
exact case-sensitive ASCII spelling, including C++ decoration. The module name
is retained verbatim; Windows module matching can normalize case. An actual
ordinal import would have a numeric ordinal and null name/hint. There are none
in these bytes. All four delay-import directories are empty.

## Reproduce without executing a target

From the retained integration worktree, use the compiler's bin directory from
`.local/sdl-admission-consumer/CMakeCache.txt` as RUNTIME_BIN:

```text
python tools/sdl-virtual-only/import_inventory.py --consumer .local/sdl-admission-consumer/fzero_virtual_consumer.exe --runtime-dir RUNTIME_BIN --output docs/2026-10-26-consumer-import-symbols.json
```

The inventory binds its generator and previous inventory by SHA256. Local paths
are inputs, not resolution claims. The reader shares the existing digest helper;
it does not invoke preparation, a compiler, a loader or a target entry point.

## Missing-export comparison contract

A future source-only comparator consumes this inventory plus a separately
selected system-image export dataset. Its report must bind the inventory digest,
edition, architecture, Windows build and servicing revision, provenance of the
system image, API-set schema bytes/hash, and each consulted module's bytes/hash.
No current host inspection or baseline selection is supplied by this inventory.

Emit one result per `(artifact SHA256, imported module, name or ordinal)`:

- `resolved`: record API-set mapping, selected host DLL, matching export and the
  entire forwarder chain to a concrete terminal export. For non-system imports,
  require the already pinned runtime candidates; do not substitute a local DLL.
- `missing-module`: the complete selected-image dataset establishes absence of
  the required module or API-set mapping.
- `missing-export`: the selected module's complete export table lacks the exact
  name or required ordinal. Report the artifact and spelling that require it.
- `unresolved`: evidence is absent/incomplete, schema selection depends on an
  unrecorded importer context, a forwarder target is missing, or the chain cycles.
  This is not evidence that the baseline is incompatible.

Forwarders can name another API set, module, symbol or ordinal; resolve them
recursively with importer context and cycle detection. Checking filenames or
finding a name in any arbitrary DLL is insufficient. Never match by hint or
strip C++ decoration. Distinguish required static imports from optional dynamic
lookups and their reviewed fallbacks. An all-resolved result proves only static
export resolution for those exact image bytes, not full dynamic closure,
initialization safety, minimum environment, loader-search policy or runtime
admission. Preserve duplicate requirements across importing artifacts in reports.

No comparison is claimed here: the selected authoritative system-image dataset
remains missing, as recorded in the [baseline contract](2026-10-26-windows-ucrt-baseline-contract.md).
Next independent useful milestone: compare the non-system import requirements
against exports of the three already pinned runtime candidates, including their
forwarders, without selecting Windows or integrating preparation.

## Focused validation

All 599 named entries, hints and 49 descriptors matched a fresh read-only `-p`
inspection using the existing SHA-pinned objdump tool. All four hashes and DLL
lists reconcile with the previous inventory. Deterministic regeneration passed;
a one-byte consumer mutation was refused before decoding. Scratch mutation was
removed. No compile suite rerun or target execution occurred.

Archie DBC-330, DBC-65 recovery and Sandy rig ownership remain preserved. No live
config/save/install/game/input/desktop/public push or player meaning change.
STD-001..040 are represented; STD-019 remains unchecked, STD-021/031/038 pending,
and partial adoption states unchanged. No isolation hypothesis failed.
