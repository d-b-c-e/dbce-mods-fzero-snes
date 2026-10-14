# Shared input and recording delivery checkpoint

Current local `main` is `9604802057691dc9ea78cc8ac78b33d1b8c1ba2d`.
The earlier `codex/shared-shim-diagnostic` branch and stock `51999e3`
checkpoint below remain historical evidence. No owner-installed copy, default
backend or force tune changed; this local F-Zero commit is not publicly pushed.

## Delivery implemented

October 13 evidence update: clean source `51999e3` privately staged stock with the
unchanged ABI2 native pin. Production Controls Apply has ordinary boot/idle
admission and 900-frame replay proof. Reusing its exact profile, stock diagnostic
Practice attempt04 consumed and freshly replayed 2,213 frames exactly; retained
images show REC, active replay and REPLAY VERIFIED. The replay capture burst was
partial/exit4, separate from successful game verification. See the
[stock evidence and hashes](2026-10-12-shim-diagnostic.md#october-13-stock-applied-controls-diagnostic-practice-and-fresh-replay).
No owner installation, public package, ordinary moving-input or Wheelkit Play
With Recording qualification follows from this diagnostic.

Later ordinary Deluxe evidence supersedes the moving-input gap for its narrow
scope. Clean local `9604802` gated Shared analog steering outside gameplay.
The installed private companion on a disposable copy recorded explicitly
synthetic keyboard navigation and a moving Practice drive, sealing 1,838 frames.
The original runner failed its all-zero/180-frame tail guard before replay;
bit 30 is the input-port marker and continued Right PDM is valid recorded input.
A separate replay-only run of the unchanged tape passed all 1,838 WRAM
checkpoints with output muted and normal exit. The first replay-only admission
failed before game launch on uppercase SHA text and is retained separately.
An owned WGC frame shows active Practice replay, with status at 1,200 frames
around capture and on-screen overlay at 1,242/1,838; it is not an exact scanout
timestamp or owner visual acceptance. See the hub's
`artifacts/fzero-ordinary-driving-20261013/replay08-outcome.json`. This did not
exercise physical wheel axes, force, an owner installation, or Wheelkit's UI.

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
| Wheelkit | Current Wheelkit has F-Zero capability/discovery and shared Play with recording/Play recording UI; its command omitted `-Rom` for a direct `fzero.sfc` copy with no `rom.cfg` | Local source now passes the current `rom.cfg` ROM for recording, direct `fzero.sfc` only when no selection exists, and the recorded descriptor ROM for replay. Missing/invalid selection refuses before launch. Focused tests passed 62/62; a receipt-staged disposable-copy command check resolved both actions against the unchanged 1,838-frame pair and rolled back. No game/UI launch or owner install. |
| Controls Apply | Hash-bound Shared writer `09c9223` exists; the ordinary Deluxe Practice capture reused the retained schema-1 profile without retune | Preserve native routing/FFB refusal; physical binding/calibration and owner-driven behavior remain separate checks. |
| Launcher | Ordinary Shared currently requires an explicit ROM and refuses `--launcher` | Replace the launcher's SDL steering/device preview with the same owned snapshot path before admitting it. Native launcher remains unchanged. |
| Wheel-only SNES extras | October 14 source adds opt-in declared X/Y/L/R through the existing Core extras API and one combined tick; archived BF6455 native candidate passes native/ABI checks, and private source pin/staging adopts both headers. Existing installed 9B8A remains extras-off. Legacy wheel save/rewind remain unavailable; keyboard remains | Complete focused private staging fixtures, then stage explicit [Extras] bindings and qualify muted ordinary input plus fresh cold replay before attended controls. No second SDL open, legacy-index migration or unrelated action aliases. See [source handoff](2026-10-14-shared-extras.md). |
| Shared physical FFB | Shared+enabled FFB is explicitly refused | Reuse the single-owner toolkit/provider bridge being developed for OutRun, then source and muted qualification before attended force checks. No new competing force lease. |
| Actual owner controls | The moving ordinary Practice capture used explicitly synthetic keyboard input, not the owner's physical wheel | Fresh attended Shared drive once complete wheel-button/force delivery is ready; keep the private Practice tape as synthetic-input evidence. |
| Ctrl+F11 | Production key routing and stop behavior tested offline; graphical runs used frame bounds/normal close | One attended stop-key check, no OS-key injection in the unattended wrapper. |
| Installed delivery | Stream Deck still launches the existing Native launcher; installer owns only launcher and WheelFfb | Add an explicit optional Shared owned-file receipt and rollback when the complete launcher/force path is ready; never silently flip the owner's backend. |
| Public package | No new binary release; current DLL is private-only and Deluxe contains embedded cartridge data | Resolve shim source/notices and Deluxe distribution separately. Review the clean stock receipt and public first-launch preservation before promotion. |

The current source/package work does not depend on rewriting accepted FFB or
triple-screen settings. These are capability gaps, not new user approval steps.

Promotion still requires reviewing/merging the local consumer source into the
current main, retaining a clean source-stamped payload and exact dependency
receipt, and an explicit owned-file install/rollback contract. The private stock
stage now closes the clean build/runtime prerequisite for its tested scope;
public redistribution and owner-launcher integration remain unresolved. Do not
silently enable Shared, relabel a Deluxe executable, or map boost/lean to gears
or camera. The local Wheelkit ROM command fix still needs deployment/
owner use; launcher preview, extra controls and force
ownership each need their own real consumer path before advertising parity.

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
stock binary ZIP was produced at that checkpoint. The October 13 private stage
and runtime evidence above supersede the stock build/runtime gap for that scope.
