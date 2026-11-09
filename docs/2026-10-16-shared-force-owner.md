# Shared input and existing FFB owner

Request199074 adds an explicit `[Input] SharedForceBridge=1` experimental route alongside `Backend=Shared` and `[ForceFeedback] Enabled=1`. Native remains the default. Ordinary Shared without this option retains its process no-force latch. Recording, scripted/replayed diagnostics and the existing captured no-force route stay separate; this option does not add a UI capability or claim physical qualification.

The game continues to use the existing force producer, native module, call gate and strength controls. The force math and default40 are unchanged. Before exclusive initialization, the optional snapshot exports and native600 version must be present and the schema1 steering instance must equal the selected force device. No direct-input fallback opens if this admission fails.

`fzero_shared_owner` uses the vendored canonical Controls binding parser, including product identity, for common and extras source inventory. It preflights eight-slot capacity and unique exact instance matches before opening a source slot, then checks the actual metadata identity, product, sparse objects/ranges, and generation-qualified interface path. The steering reader must report that it borrowed the force handle. Auxiliary slots use that same native module. Paths and metadata are copied under the force call gate before Core receives them.

Each simulation/overlay tick takes one snapshot per owned source slot and passes the copied batch to one Core bridge tick. The extras extension returns common and declared X/Y/L/R from that same tick. A physical read fault, stale sequence, changed generation or invalid timing clears the whole sample batch and permanently refuses further reads until explicit restart. A valid physical sample enables force only after the mapped input was consumed by a guest frame and Core reports Live, noninjected steering; overlay ticks stay silent; Core rearming/missing/stale input remains silent. The existing game mapper and Core normalization remain responsible for their original duties.

Core stops before read slots close, effects release, native cleanup and module unload. An unresolved Core stop marks the handle terminal, stops force and retains its source slots/module until process exit. The existing call gate drains in-flight force/source work and rejects late calls. No independent reader, force model, acquisition retry or callback escapes this owner.

## Qualification and remaining work

The separate native600 source candidate `59486032871b9a8fc349399ed96bc2cb5451bbf4` passed the six x64/x86 snapshot-fake/DLL-build/device-free lifecycle steps. It preserves the legacy41 exports/loader/version600 and adds three optional exports. Lola's ordinary-only bridge/extras source is `d92656657a4322a37db9cfb82073350ed8d01dcc`; this checkout uses its exact bridge header. Neither fact updates this game's accepted runtime pins.

The first game build passed; its52 affected CTests had10 failures. Three new fake force-owner cases correctly refused profiles missing the canonical required `prod`; seven existing diagnostics exposed an empty pre-open stop incorrectly spending the new terminal guard. Both issues are corrected in source. The pure source-owner test's original Release-disabled assertions are not credited; they are now always active. Metadata/path copying and native600 version admission were also tightened before the queued follow-up. A separate pending-sample flag keeps force stopped between the physical read and Core acceptance, including a concurrent force callback; silence/shutdown revoke both flags. Original `E:/fzero-research/owner-checks-nemo-20261016/run-01/result.json` remains FAILED. Exact safe cleanup/lease release was recorded before the next owner entered.

The corrected game-only continuation passed all four affected builds and52/52 CTests from source `a720d34f2944a9f41f9e7d2dd4ada8105c8108a9`, including the always-active Release source-owner assertions. The build emitted no warnings. `E:/fzero-research/owner-checks-nemo-20261016/run-02/result.json` records complete process-query/child closure and exact lease release at00:45:08.337Z before the direct Nade handoff. `shared-force-owner-qualification.json` binds its source tree, inputs and original logs. The runner used a fresh result folder and the previous cached ROM-free build, retaining the original failed results. The bridge DLL is now qualified and the separate optional dependency pair is staged and pinned as described below. A graphical executable has not yet been rebuilt with that pair. The prepared game19afaf25/UI6d9b715/helper32bfd0bd candidate, owner installation, source distribution and public releases are unchanged. No real game, DirectInput device or physical force ran during these fake checks.

All37 toolkit ledger IDs are still present. STD019 rendering is unchecked, STD021/031 physical normalization and STD037 schema2 remain pending. This source ownership work does not close those gates.


## Qualified optional dependency pair

Shared bridge product `d92656657a4322a37db9cfb82073350ed8d01dcc` has the retained x64 DLL `83dd742f015fd384dc2a9bee894ed8dccca0e204947e4fb158c3d276025888c2`,224 retained ABI checks and101 copied-input checks. Fixture-only correction `c1e2ddb502309c9a611d0e971355a9af1e1f6ad8` releases/arms its button before pressing it; the original held-button failure remains retained. The current generated receipt recordsc1e2ddb, while the x64 DLL was compiled atd926566. `vendor/dbce-shim/SHARED-FORCE-PIN.json` records that distinction, upstream receipt/log hashes, and canonical shared landing83b8ce92. Native600 remains the reviewed snapshot candidate5948603, x64 DLL `efa948e76b4060db3a5416bad7598d4c725e83fe009bd072049c5348e44adec8`.

Two unchanged DLLs and `PAIR.json` are staged at `E:/fzero-research/shared-force-dependencies-nemo-20261016/candidate`. This is a dependency candidate, not a rebuilt game/install. Its receipt SHA256 is `fa26f86ba563dff3353e480e47cb979fa11a554c1d95566c69dad3caafd75b51`. Actual PE inspection verifies x64 and44 native/31 shared exports; all seven game runtime source/header inputs still match the52-test compilera720d34. The old native DLL and baseline native/shim pin files are unchanged.

Recreate this dependency folder with the qualified input DLLs:

```powershell
pwsh -NoProfile -File tools/Stage-SharedForceDependencies.ps1 `
  -WheelFfbDll '<qualified native600 snapshot DLL>' `
  -ShimDll '<qualified83DD742F shared DLL>' `
  -OutputDirectory '<fresh absolute dependency directory>'
```

For a future private x64 graphical build, pass `-DFZERO_SHARED_FORCE_DEPENDENCIES=<absolute dependency directory>` in a fresh build directory, with `FZERO_SHIM_DLL` unset/empty. This route verifies both hashes before staging the pair. Changing either DLL or its pin forces reconfiguration/revalidation. Default builds still stage the original native DLL; the existing `FZERO_SHIM_DLL` route still accepts only its old qualified hash. No option writes player configuration: ordinary `[Input] Backend=Shared`, `SharedForceBridge=1`, optional `SharedExtras=1` and `[ForceFeedback] Enabled=1` remain explicit choices for later authorized runtime work.

Five metadata-only CMake admission checks passed: default off, exact pair, rejection of a tampered pair, conflicting old/new routes and a32-bit target. A language-NONE configuration also confirms the pin and both DLLs trigger revalidation after replacement. These checks started no compiler, DLL, game, provider or force output. `shared-force-dependencies-qualification.json` binds the exact staged bytes, receipts and check logs. The accepted ready candidate and owner installation are unchanged; a paired graphical rebuild and actual runtime/physical force acceptance remain unqualified. No passing52-case game suite or native binary was rebuilt for this dependency-only step.
