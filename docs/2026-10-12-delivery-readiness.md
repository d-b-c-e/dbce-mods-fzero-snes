# Shared input and recording delivery checkpoint

Source is on `codex/shared-shim-diagnostic`, based directly on main
`e77d0f99ea4d69d7dabecca73fc5246d1010db80`. Canonical merge target:
`E:/Source/games/dbce-mods-fzero-snes`, branch `main`. At this checkpoint it can
fast-forward. Recheck the target before merging; use the DBCE origin, not the
upstream mstan remote. No installed copy, default backend or force tune changed.

## Delivery implemented

- Existing direct-ROM `--record` and `--replay`, rendered REC/Saved/Error and
  REPLAY/Verified, using `.fzpt` plus its initial-state sibling. The unchanged
  2,351-frame private Practice original and fresh 900-frame ordinary Shared
  recording have exact cold game-memory proof. See the [ordinary evidence](2026-10-12-ordinary-shared-input.md)
  and [CLI evidence](2026-10-12-recording-cli.md).
- `tools/Recording.ps1` supplies a readable package entry point without config
  writes, filename overwrites or a new container. The public package includes
  it and [the quickstart](RECORDING-QUICKSTART.md). Record uses normal force and
  persistence settings; replay keeps its existing no-output/no-write behavior.
- Native remains the default. `[Input] Backend=Shared` is explicit; bound
  generic actions come only from the Core-owned reader. Missing/faulted Core
  actions do not fall back to legacy SDL.
- The stock stager's `--private-shared-input` option retains exact DLL/header
  pins and provenance. The public ZIP refuses this private receipt and DLL.
  It still refuses embedded Deluxe builds. Existing qualified private copies
  are not repackaged or relabelled as stock/public evidence.

## Narrow remaining integration work

| Area | Current boundary | Next implementation/check |
|---|---|---|
| Wheelkit | No recording discovery or Play Recording entry for this consumer | Call the existing CLI, retain tape/state and the pinned game/config case; expose capability limits, no new format. |
| Launcher | Ordinary Shared currently requires an explicit ROM and refuses `--launcher` | Replace the launcher's SDL steering/device preview with the same owned snapshot path before admitting it. Native launcher remains unchanged. |
| Wheel-only SNES extras | X/Y/L/R and host save/rewind are unavailable under Shared; keyboard remains | Optional raw digital snapshot from the existing Core-owned epoch plus exact legacy-device identity; no second SDL open, no unrelated action aliases. |
| Shared physical FFB | Shared+enabled FFB is explicitly refused | Reuse the single-owner toolkit/provider bridge being developed for OutRun, then source and muted qualification before attended force checks. No new competing force lease. |
| Actual owner controls | At-rest ordinary capture is not a moving owner drive | Fresh attended Shared drive once complete wheel-button/force delivery is ready; keep the injected Practice case as diagnostic evidence. |
| Ctrl+F11 | Production key routing and stop behavior tested offline; graphical runs used frame bounds/normal close | One attended stop-key check, no OS-key injection in the unattended wrapper. |
| Installed delivery | Stream Deck still launches the existing Native launcher; installer owns only launcher and WheelFfb | Add an explicit optional Shared owned-file receipt and rollback when the complete launcher/force path is ready; never silently flip the owner's backend. |
| Public package | No new binary release; current DLL is private-only and Deluxe contains embedded cartridge data | Resolve shim source/notices and Deluxe distribution separately. For stock, fresh clean build/receipt and first-launch preservation checks remain required. |

The current source/package work does not depend on rewriting accepted FFB or
triple-screen settings. These are capability gaps, not new user approval steps.

## Offline delivery validation

October 12 after the coordinated DRIVE capacity run exited:

```powershell
py -3 tests/test_unified_package.py
py -3 tests/test_recording_helper.py
```

28 package checks pass, including deterministic ZIP/verify, pinned private
DLL/header staging, missing/mutated bytes, public DLL/receipt refusal, and the
private option still refusing embedded Deluxe before creating output. Four
Windows PowerShell 5.1 helper fixtures pass against a harmless argv executable:
external caller directory, special-character paths, package child working
directory, native exit 17 preserved, fresh-path/pair refusals, and unchanged
settings. No game, device or installed file was used by these checks. No actual
stock binary ZIP was produced: the fresh stock build/runtime check remains open.
