# DBCE F-Zero SNES Unified Preview

F-Zero SNES and optional BS Deluxe use one native host, existing launcher,
settings set and version. Wheel, FFB, telemetry, recording/replay and experimental
triples are internal features. F-Zero X is a different product. The source
folders in E:/Source are attached development worktrees, not separate products
to merge. The in-place rename to `dbce-mods-fzero-snes` is complete, retaining the fork relationship,
upstream attribution/history and atomic feature commits.

Canonical metadata is [game-product.json](../game-product.json). The
[identity policy](PRODUCT-IDENTITY.md) defines the numeric upstream base,
source-qualified preview label, reviewed repository-name migration and remaining
consolidation actions. As verified on 2026-10-02, default `main` and the retained source lane
[`codex/unified-product-20261001`](https://github.com/d-b-c-e/dbce-mods-fzero-snes/tree/codex/unified-product-20261001)
both contain reviewed source `14ea520bf97700f6bd5bb832b1daf1fdb6483f50`.
Historical release ZIPs are not unified preview binaries; no DBCE unified
binary release or installation promotion has occurred.

## Install and setup

Extract the complete candidate ZIP to a new folder. Run `Setup.cmd` to open the
existing launcher, choose your own USA ROM, and configure controls there.
`Setup.cmd` only opens that launcher; it does not rewrite settings or profiles.
See [SETUP.md](SETUP.md) for the feature settings and recovery steps.

To update a working install, first close it normally and back up its complete
folder. Verify the candidate in a separate folder before parent-coordinated
deployment. Retain config.ini, fzero-video.ini, keybinds.ini, rom.cfg, saves,
music and user shader folders. Do not extract over a running game. The candidate
does not include an installer that modifies an existing installation.

## Reproducible candidate packaging

The default route below remains stock-only. The explicit local private Deluxe
route is documented at the end of this page; it does not authorize distribution.

The historical `make_release.py` release route is retained. It expects embedded
BS Deluxe data and is not this candidate's distribution route. The unified
candidate excludes the private generated BS module, its embedded payload and
all user data. A stock-only source build must omit FZERO_DELUXE_GEN_DIR and
FZERO_DELUXE_DATA_FILE. No binary from the current BS-equipped installed build
is repackaged here. Public packaging of that module requires a separate review
of distribution rights and build inputs.

`package_unified.py` accepts a prebuilt payload folder and an explicit build
receipt (`dbce.fzero-build`, version 1). The receipt pins source commit/tree,
product version, stock-only mode, dependency identities, PE import closure and
every approved payload file hash. It is build-time consistency evidence, not a
cryptographic attestation of the compiler. Never manufacture a receipt for an
old binary. The packager validates the receipt, x64 PE imports, the pinned
dynamically loaded WheelFfb.dll and notices, then adds this product's docs,
Setup.cmd and machine-readable manifest. ZIP ordering and timestamps are fixed.
It refuses path escapes, symlinks, user settings, private data, shader packs,
missing runtime dependencies, stale hashes and existing outputs. It runs no
payload executable and never scans/copies a whole build folder.

```powershell
cmake -S . -B build-stock -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -DFZERO_DELUXE_GEN_DIR= -DFZERO_DELUXE_DATA_FILE=
cmake --build build-stock
python tools/stage_unified.py --build <fresh-stock-only-build> --output <new-stage-directory>
python tools/package_unified.py --payload <stock-only-stage> --receipt <build-receipt.json> --output <new-output-directory>
python tools/package_unified.py --verify <candidate.zip>
python -m unittest discover -s tests -p test_unified_package.py
```

Initialize the exact submodule pins and generate your own stock module first,
as described in README.md. The MSVC recipe uses its static runtime; a dynamic
runtime build must stage every imported non-system DLL with reviewed notices.
The tooling refuses to assume that a runtime installed on the build machine
will also exist on another user's computer.

Required payload: FZeroSNESRecomp.exe, its FZeroSNESRecompHeadless.exe replay
companion, WheelFfb.dll, approved build runtime DLLs,
checked-in assets/shaders, the pinned launcher fonts/images, licenses and
dependency/font notices. The staging tool reads CMake's exact dependency roots,
verifies clean gitlinks and the build's source stamp, and pins those assets.
The included replay adapter uses externally supplied pinned toolkit tools only
for optional normalized force observations; game recording and headless replay
do not require private toolkit access. No toolkit implementation is copied.
Receipt example and fields are documented in tools/package_unified.py.
The numeric upstream base comes from VERSION. The preview name/artifact stem
and canonical links come from game-product.json and are checked by the
packager. The full label is `DBCE F-Zero SNES Unified Preview
1.8.3+g<sourceRevision12>`; filenames use the DBCE preview namespace.
The package channel is candidate, with no new
published release implied. Submodule pins and toolkit file hashes remain
independent of the game version. Experimental actuator DLL overrides must be
reviewed and repinned explicitly; they are not silently treated as v0.13.0.

## Validation boundaries

Physical FFB is opt-in and requires attended acceptance. Replay compares game
simulation and normalized software force requests without wheel actuation.
The current BS replay adapter refuses Deluxe cases until their extra data is
pinned. Three-window diagnostic replay inside Surround does not validate
three physical monitors, scanout, focus or seams. Side world sprites/effects
remain incomplete; keep experimental availability in the product manifest.
No current installed payload, monitor profile or saved tune is changed by this
candidate work.

The approved interactive stock replay at `21f2605` completed 11,364 frames
and rendered the original CRT preset on three separate monitors, with FFB
disabled and the staged wheel DLL excluded. Placement, desktop capture and
sampled CPU/GL backbuffer evidence were retained. Center keyboard/foreground
focus failed acceptance; no definite application defect was established.
An attended focus check remains required. This does not establish optical
scanout/seams, comfort, complete side content or full rig acceptance.

Published source `14ea520` adds an optional softer CRT mask while preserving
the original preset and default. Its before/after comparison is simulated
CPU shader math, not a new GPU render. GPU compilation and visual acceptance
remain pending. Each separate view uses its own input texture size; the
reproduced 512-output-pixel band is evidence of a mask-alias contribution,
not proof of one mask stretched over the full desktop. No game, device,
display, install or release action is part of this documentation update.

## October 23 private Deluxe Native2 packaging source

The existing stager and packager now have an explicit private Deluxe route.
This is source qualification only: no fresh Deluxe product or Native2 helper
publication has been made for this route. The frozen stock private ZIP is
unchanged. Public/default packaging continues to reject private receipts and data.

Required inputs are a fresh clean build of this source candidate, exact clean
framework/UI gitlinks, the pinned WheelFfb and private Shared runtime, and a newly
published helper from Wheelkit commit
`d0f333c60892dc44beb36e277b19bd6a650507b7`. Older helper publications
cannot be substituted. Publish with Wheelkit's existing
`tools/Publish-StandaloneSetup.ps1 -Artifacts <new-private-output>` from that
exact clean checkout, holding the shared lease throughout publish/build/cleanup.
Configure the fresh game build with `FZERO_SETUP_BUNDLE` pointing to that
published bundle and both `FZERO_DELUXE_GEN_DIR` and `FZERO_DELUXE_DATA_FILE`
pointing to the retained private generated sources and imported data. Preserve
the existing stock build's compiler, dependency, runtime and notice inputs.
Build both graphical and headless targets; both must embed identical Deluxe data.

Retained local input locations are
`E:/Source/games/dbce-mods-fzero-snes/captures/bs-deluxe/gen` and
`E:/Source/games/dbce-mods-fzero-snes/captures/bs-deluxe/mods/bs-deluxe.dat`.
Their presence is not a new build qualification. The stage receipt records the
actual generated C/header hashes and embedded data hash/size/offset. This binds
inputs and byte consistency; it does not independently attest compiler provenance
or generated-code semantics.

First use the existing installer against an explicitly named **new empty scratch
target**. Always supply `-Target`; the installer's default is the owner install.
Create that empty directory explicitly and supply `-PrivatePackageScratch`.
This mode refuses missing, linked or occupied targets and cannot be used with
rollback. Normal installation still requires its existing launcher. A package
target never needs a dummy launcher or copied ROM to bypass that guard.
Retain its complete backup folder separately. Then stage and package:

```powershell
./tools/Install-StreamDeck.ps1 -BuildDir <fresh-deluxe-build> -Target <new-empty-scratch-target> -Label native2-package -PrivateSharedInput -PrivateControlsSetup -PrivateHostControls -PrivatePackageScratch
python tools/stage_unified.py --build <fresh-deluxe-build> --output <new-stage> --private-shared-input --private-controls-setup --private-host-controls --private-deluxe-install <new-empty-scratch-target>
python tools/package_unified.py --payload <new-stage>/payload --receipt <new-stage>/build-receipt.json --output <new-output> --private-deluxe
python tools/package_unified.py --verify <private-deluxe-native2.zip> --private-deluxe
```

These are future delivery steps, not commands run during source qualification.
The scratch receipt must name this exact source commit, the renamed wheel
launcher, the scoped Native2 contract and all six actions. Launcher/WheelFfb,
helper manifest and apphost, complete helper inventory, Shared pins and every
installer-owned file must match the actual build. The scratch backup-state must
prove every owned path was absent; previous owner files are refused. The archive
retains that state under `private-inputs/backup-state.json` as provenance, not as
an installation rollback command. Use the retained scratch backup with the
existing installer for scratch rollback; no owner rollback is performed here.

The payload has only `install-receipt.json`; `build-receipt.json` stays beside it
in the stage and is embedded in the archive manifest. A second root receipt,
stock executable, unreceipted payload file, old helper or Native1 receipt refuses.
Settings, saves and ROMs are not copied. `Setup.cmd` opens the renamed launcher.
ZIP bytes are deterministic for identical admitted inputs, including identical
receipt bytes; fresh builds or backup labels need not produce identical ZIPs.

Qualification: 40 inert Python package/stage tests pass, including byte-identical
repeat archives, default public refusal, stale source/helper/data, stock headless,
owner rollback, ambiguous receipt and real staging-entry refusal cases. The
fixture uses synthetic PE files and temporary scratch state, with no game, helper
execution, provider, screen, device, owner install or public upload. Evidence:
`build-private-route-check/package-tests-headless.log`; earlier failures remain
in that ignored folder. Fresh helper publication, Deluxe build/package, packaged
PlanOnly and actual rendered/device acceptance remain open.

## October 23 combined private product attempt and retained gates

Combined source `1f88c2fb0fc34cc35c7ea405d620eb409877ad80` includes telemetry
merge `0ddec5c`, the existing private route and Reha's qualified forced-blank
renderer fix. Exact helper `bdb6e09` published, CMake configured, graphical and
headless hosts linked, and the combined telemetry fixture passed. The two
existing `HIBYTE` C4005 warnings remain. No game or GUI was launched.

The original scratch installer refused its empty target because the normal
route requires an already installed launcher. No staging, ZIP or PlanOnly phase
ran. Source correction `c5b196f` adds the explicit empty-target opt-in above;
the existing NativePin suite passed **129 inert checks under PowerShell 5.1**,
including default refusal, implicit/missing/occupied target and rollback refusal,
all-new Native2 installation and absent-state rollback. It opened no device.

Evidence is in this worktree's ignored
`build-private-deluxe-integration-20261023`: `scratch-ps51.log` and
`run-01/result.json` (SHA256
`DC833659D93E91754AD58B24A1F090F2534833A64A650AE2E9F6D7EC16F38C48`).
The failed result remains unchanged; `run-01/prior-build-1f88c2fb0fc3` preserves
the passing binaries, headers, cache and hash inventory before any cache reuse.
All scoped children exited and the exact lease released at
`2026-10-23T06:31:38.5527197Z`, before the next owner entered; no overlap occurred.

Reha subsequently found that the bdb helper refuses an absent `config.ini`, so a
fresh package cannot configure Native controls before first launch. Final helper
publication, staging/package/verify and PlanOnly are held for his qualified
missing-file-baseline successor and its explicit game helper-pin adoption.
Resume with the existing object cache, fresh source/helper stamps and a new
scratch/output folder; do not republish the old helper or repeat accepted force,
renderer, driving or device checks. No owner install, ROM/config copy, game,
public upload or acceptance is claimed. Lap number remains zero/unqualified.

### Qualified missing-config helper successor

Reha's clean `d0f333c60892dc44beb36e277b19bd6a650507b7` successor passed all
232 F-Zero Core cases and the helper GUI Release build (zero warnings/errors).
It includes the Native2 route, verified missing-config baseline and Play fixes.
The game now hard-pins this source for the private route; prior bdb manifest and
apphost identities cannot substitute for a fresh matching publication.

Evidence: Wheelkit worktree
`E:/Source/toolkits/.worktrees/reha-fzero-deluxe-routes-20261023`,
`.local/fzero-route-audit/run-02/result.json`, SHA256
`AA306187956561378F0781B3C4276D5507D1E9BC84B2749A432F35D9A4524F3E`.
Earlier expected baseline failures and the corrected fixture expectation remain
separate. Exact lease released at `2026-10-23T06:50:35.0397930Z` with a safe
complete census and all 21 retained process identities absent. This qualifies
source behavior; actual helper publication, linked product, scratch/package
verification and rendered/device acceptance remain their separate gates.
