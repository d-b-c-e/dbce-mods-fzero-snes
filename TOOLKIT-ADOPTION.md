# Toolkit standards adoption

## October 23 scoped Native helper admission source

STD-033/035/037 source follow-up adds `nativeControls` to the Deluxe install
receipt: schema2, adapter `fzero-raw-wheel-2`, and the renamed launcher consumer.
It is separate from the retained top-level schema1/adapter1 and Shared controls
descriptor. The installer requires all three linked canonical axis-profile keys,
in addition to the clean source stamp and six action markers, before declaring
Native calibration. Receipt refresh/rollback preserve unknown metadata and
owner settings. No global `ini-controls-2`, Shared/Core upgrade, stock receipt,
product/package rebuild, owner install or physical qualification is implied.
Companion Wheelkit source on `lamo/fzero-native-schema2-helper-20261023` adds the
version-gated Native portable import/Preview/Apply path; focused F-Zero fixtures
227/227 and helper GUI compilation passed. The clean-source game producer
fixture passed117 inert checks under Windows PowerShell5.1; evidence is
`build-native-helper-check/native-pin-ps51.log`. Existing STD-019
unchecked and STD-021/031 physical pending states remain unchanged; all37 ledger
IDs remain represented.

## October 19 Native profile steering direction

STD-033 source follow-up adds the previously unsupported Native steering
inversion through the existing schema-1 planner, guarded config transaction and
raw SDL reader. The Native launcher checkbox and preview share the same
normalized signed-axis inversion; Shared keeps Core normalization and exposes
no Native wheel settings. `fzero_controls`225 checks and `fzero_mods` pass in
focused offline fixtures. See the October 19 source successor in
`docs/2026-10-10-profile-controls.md` for exact receipt and retained failures.
All37 ledger IDs remain represented. STD-019 stays unchecked;
STD-021/031 physical normalization remain pending; STD-037 source adoption below remains partial, and
STD-030 in-game F6 parity stays partial. This source change does not promote
installed, rendered, force, physical or distribution acceptance.

Which entries of the wheel toolkit's standards ledger
(`E:\Source\toolkits\dbce-wheel-mod-toolkit\STANDARDS.md`) this mod has brought in.
Update a row in the same commit that adopts it. Statuses: `adopted`, `partial`,
`pending`, `n/a` (say why), `unchecked` (nobody has looked yet).
Audited 2026-10-04 (read-only standards audit); STD-001 and STD-010 revised 2026-10-04 after the merge and install check.

| Standard | Title | Status | Notes |
|---|---|---|---|
| STD-001 | One mod per game | partial | wheel/FFB/telemetry/triple/recording merged with main on claude/fzero-main-merge (2026-10-04, merge of codex/triple-wheel-integration a1a1267; Release MSVC build with embedded BS Deluxe and 15/15 ctest pass); now merged to main b7eb125; previous staging branch is historical. codex/analog-wheel, codex/force-feedback, codex/telemetry hold superseded early versions (no unique work worth merging) |
| STD-002 | Recording and playback from launch | partial | October 13 private ordinary Shared Deluxe Practice route sealed 1,838 frames with explicit synthetic keyboard driving; a separate cold replay verified all 1,838 WRAM checkpoints and exited normally. The initial neutral-tail refusal and replay-only SHA-case admission refusal remain separate. Current Wheelkit has F-Zero discovery/shared UI; a local ROM argument fix passed 62 focused tests and a receipt-staged disposable-copy command check on the unchanged tape. Recording binds the selected/direct ROM; replay binds the recorded original. No Wheelkit UI launch or owner installation; Ctrl+F11 attended use and physical inputs/force remain unqualified. See docs/2026-10-12-recording-cli.md. |
| STD-003 | Normalized FFB strength | partial | Device-free production-model synthetic component trace; spring/damper/road remain distinct, default40 unchanged. Recorded reference and physical normalization pending. See docs/FFB-COMPARISON.md. |
| STD-004 | Consistent settings UX | partial | settings in the recomp launcher Mods page; F6/F8 taken by save slots/rewind (sdl_main.c:2020,2481-2503) |
| STD-005 | Camera numpad layout 8/2 9/3 4/6 7/1 +/- 5 | n/a | no 3D camera rig; Mode 7 fixed chase view |
| STD-006 | Camera step sizes are settings | n/a | no adjustable camera mount |
| STD-007 | Triple screens in one wide window | partial | side panels draw ground, sky and now live opponents (712a7cf, billboard at world anchor; seen headless frame 2070); manual enable, no Auto; side-by-side test install "F-Zero (SNES Recomp) triple-test"; seen at the rig under Surround 2026-10-05 (attract demo, 60 fps; side ground aliases at distance). October23 source corrects forced-blank side rows after sky/vehicle composition; synthetic renderer regression only, no new display or performance acceptance. |
| STD-008 | Display changes: game applies once | partial | SDL FULLSCREEN_DESKTOP, no mode changes (likely compliant); watchdog run pending |
| STD-009 | Dashboard telemetry matches the HUD | partial | speed from position deltas at a guessed 0.25 m/unit, not the HUD; RPM derived from speed (1200-12000, documented); gear fixed at 1 |
| STD-010 | Install the latest build for testing | adopted | `tools/Install-StreamDeck.ps1` backs up the replaced launcher/WheelFfb.dll, installs build-merge's exe as FZeroSNESRecomp-wheel-launcher.exe (the Stream Deck target) and writes install-receipt.json; `-Rollback` restores. Installed October 8 from 4ac6f6b (steering-native50ba), five protected files unchanged; 17 CTest and 31 adapter checks pass. Muted plain launcher rendered/exited 0, 36 protected files restored; settings navigation and force remain untested. |

| STD-011 | Work lands on main | adopted | Packaging research starts from b7eb125 and returns to main; private ROMs, generated content and recordings remain outside release archives. |
| STD-012 | Reproduce the route and preserve original signals | partial | October 12 private Deluxe player Practice original has 2,351 exact cold WRAM checkpoints; final consumed words equal the shared capture and computed force/telemetry remain available under muted delivery. Original immutable. New CLI graphically replays the same 2,351 frames and a fresh 900-frame preview exactly, with rendered status; owner normalization remains open. |
| STD-013 | The installed build launches plainly | partial | The Stream Deck key launches the integration build with no extra arguments (grid PLN ðŸ§ª); triple screens start only when the Triple Screen mod is on in the launcher |
| STD-014 | Request reciprocal review when progress stalls | adopted | Process standard; reviews go through the portfolio inbox |
| STD-015 | Triples on Surround and on separate monitors | partial | Display layout Surround (one spanning fullscreen display) or Separate monitors (one borderless SDL window per display); Surround seen at the rig 2026-10-05, separate displays not yet at the rig |
| STD-016 | Telemetry: Forza Horizon layout, on by default | partial | Forza Horizon 324-byte UDP built in, but off by default (`[Telemetry] Enabled=1` in config.ini) |
| STD-017 | Hide settings pages that have nothing to offer | n/a | Settings live in the recomp launcher's Mods page; there is no family panel |
| STD-018 | Handling changes never reach online scores | n/a | No online play or leaderboards |
| STD-019 | Menus on the centre screen; side screens only in gameplay | unchecked | The centre window keeps the stock game, vehicle and HUD; whether the side windows stay black in menus is not yet checked |
| STD-020 | Standard feature checklist per game | adopted | Row in the portfolio grid (dbce-project-mgmt PROJECTS.md) |
| STD-021 | art of rally is the FFB reference | pending | Own condition-effect model (spring, damper, road), default 40; Strength 50 trace exported (docs/FFB-COMPARISON.md); physical comparison with art at 50 pending |
| STD-022 | One triple-screen selector | adopted | Launcher: the Triple Screen mod on/off is Off, then Display layout "Surround" / "Separate monitors" (labels aligned 2026-10-07; saved values stay Span/Separate) |
| STD-023 | Frame-rate readout in the settings panel and the log | partial | October 15 source candidate reuses the unchanged toolkit FrameWindow/FormatFrameLine for ten-second measured center-window presentation summaries, with average/1% low/worst frame and bounded log cadence. Existing launcher settings describe the measurement; live in-game settings panel remains absent. Authored fixtures and graphical build/render checks are not yet run. See docs/2026-10-15-presentation-counter.md. |
| STD-024 | Optional on-screen frame-rate counter | partial | October 15 source adds ShowFPS (default off) in the existing launcher Presentation FPS options and centers the summary at the bottom of the center game view for GL/SDL. Source only; rendered/physical acceptance remains open. |
| STD-025 | One force model for every tyre game | n/a | Hover machines with no tyre model; the condition-effect model stays game-specific |
| STD-026 | Deliberate player tuning | partial | Launcher exposes steering/crash; saved legacy auxiliary level is retained. Reviewed candidate installed October 8; launcher rendering and owner feel pending. |
| STD-027 | Independent steering and effect strengths | partial | October 8 candidate splits spring/fallback from damping/road; preserves legacy model 2 and adds model 3/headless header. Build, model/lifecycle/provider and adapter tests pass; rendering and feel pending. See docs/2026-10-08-steering-strength.md. |
| STD-028 | Complete owner registry snapshots | n/a | Native app uses INI/save files; current test and install paths do not export/delete Unity registry keys. |
| STD-029 | New functionality installed and on | partial | Existing controls runtime installed; next capability receipt candidate is source only until its clean build and closed-game installation. |
| STD-030 | F6 panel first | partial | F6 opens an in-game settings panel (src/fzero_panel.c model, overlay in sdl_main.c; [KeyMap] ModMenu): triple screens, output, curve presets, widescreen/aspect, presentation FPS, FPS readout, crash-flash reduction, saved through the launcher's fzero-video.ini writer. Plain F6 no longer loads quick slot 6 (Shift+F6 saves; the browser loads). Wheel/FFB/controls remain launcher-only; not yet seen at the rig. |
| STD-031 | New Art reference level | pending | Physical normalization remains pending; this controls work does not change forces. |
| STD-032 | Wheelkit reads portfolio records | partial | Installer declares hash-bound fzero-raw-wheel-1 controls in install-receipt.json after checking configured and linked source stamps. Restores/withdraws capability with the runtime. Wheelkit's Native writer exists. October 13 source-only private stager adds a separate Shared controls descriptor inside its existing build receipt; installed receipts and catalog remain unchanged. |
| STD-033 | Rig-profile controls contract | partial | `[Controls]` in config.ini -> `[Controller.<SDL GUID>]` at start (src/fzero_controls.cpp, the toolkit's parser vendored from e4502a6); `fzero_controls` ctest: 102 checks, translation, startup apply and the game's reader on a virtual wheel. Installed 2026-10-10 (`docs/2026-10-10-profile-controls.md`): the owner's profile applied by the game itself to the real R12 at start. Wheelkit Native writer exists; owner acceptance and Shared production Apply qualification remain separate. DirectInput injection does not reach the Native SDL path. 2026-10-22 binding coverage (source read only): the Native profile translates steer, throttle/brake (axes), confirm->A, back->B, start, select and navUp/Down/Left/Right; Native source now translates `nativeX/nativeY/nativeL/nativeR/saveStateMenu/rewind` into the existing `[Controller.<guid>]` keys; Shared binds the same common actions through Core. In-game and physical delivery of these bindings is unverified. See docs/2026-10-22-game-action-bindings.md. |
| STD-034 | Apply qualified by a closed loop | partial | Existing raw-wheel evidence remains separate. October 12 shared-shim production Core diagnostic reached player Practice, matched 2,351 consumed words and cold WRAM checks; this injected drive is not Wheelkit-to-shim or physical owner-control qualification. See docs/2026-10-12-shim-diagnostic.md. |
| STD-035 | Identity, provenance and failure rules | partial | Exact profile bytes through pinned ABI2; muted latched private Deluxe original and native terminal shutdown qualified October 12. Five private CLI runs exit0 with all77 owner pins exact; nonlatched preview seals and matches final consumed words. Ordinary Shared9790788 failed in legacy SDL acquisition (retained dump). Corrected7187c57 disables SDL for Shared; a private900-frame at-rest original and fresh graphical replay pass exactly, normal exits and owner77 pins unchanged. Legacy wheel extras and launcher remain explicitly unavailable pending owned-reader integration. Shared+FFB refusal remains pending the ownership bridge. No installed capability/default changed. See docs/2026-10-12-ordinary-shared-input.md. |
| STD-036 | Bindings from each device | partial | Shared Core supports one owned epoch per bound instance in the diagnostic; installed raw-wheel writer still has its R12/single-device scope. October 14 source adapter adds opt-in declared fzero-snes.x/y/l/r on the same owned extras tick, with 36/36 device-free fixtures and independent source review. Clean ea568c2 x64/MT BF6455E6 native candidate is archived with passing native fixtures and pure consumer ABI/export checks; new private source pin/staging owns both headers, while old installed four-file receipts retain extras-off compatibility. Adoption fd52deb passed 33 package tests, 34 synthetic installer checks, 51 companion checks, 36 consumer cases and actual five-file dependency-only staging. Runtime, physical and multi-device acceptance remain open. No unrelated generic action aliases. See docs/2026-10-14-shared-extras.md. 2026-10-22 binding coverage (source read only): Shared binds steer, throttle/brake, confirm/back/start/select and navigation by default, fzero-snes.x/y/l/r with `SharedExtras=1`, and saveMenu/rewind with `SharedHostControls=1` as well; gears, clutch, handbrake, camera, lookBack, reset, horn, modMenu and muteFfb are declared unsupported. Native source binds common actions plus `nativeX/nativeY/nativeL/nativeR/saveStateMenu/rewind` from the profile, all on one device. In-game and physical delivery is unverified. See docs/2026-10-22-game-action-bindings.md. |
| STD-037 | Measured endpoints through schema 2 | partial | October22 retained Native source adopts the exact toolkit ee8014d controls header (6002B403) plus unchanged canonical vectors (30EEEDA8) and opts Native Apply/action denormalization into Schema2. Native SDL reads canonical persisted bindings before existing deadzone/curve/threshold, with inversion once. Qualified raw range0..65535 only; invalid/unrepresentable Schema2 refuses the transaction. Schema1 and Shared/Core remain unchanged; no global ini-controls-2 capability, writer, product/package/install or physical acceptance is claimed. October23 independent focused MSVC compilation (including the launcher object) and four fixtures passed; contract599 checks/207 vectors, controls308 checks. Merged installer passed92 synthetic checks under PowerShell5.1. Product/package, rendered and physical qualification remain open. See docs/2026-10-10-profile-controls.md. |
| STD-038 | Game window lands on racing row | pending | Current shared ledger adds this October24 rule. Retained native SDL window placement has not been reconciled against the row policy; no runtime evidence added by the controls refusal fix. |
| STD-039 | Direct in-process wheel input | partial | Retained Native SDL controls consume raw wheel bindings in-process. This source-only refusal correction does not qualify physical response or alternate Shared paths. |
| STD-040 | One simple package and standalone binding parity | partial | Unified package direction exists; standalone launcher controls and Wheelkit selected-profile action parity, replacement install and public packaging remain unqualified. No package promotion in this change. |


October 12 delivery follow-up for STD-002/012/035: `tools/Recording.ps1` and
`docs/RECORDING-QUICKSTART.md` expose the existing record/replay CLI in the player
package. Optional private Shared staging pins the dynamic DLL/header/provenance;
the public packager explicitly refuses it until the distribution gap is resolved.
Native defaults, installed owner settings and the existing force model are
unchanged. Remaining launcher, wheel-extra, force-ownership and Wheelkit gaps are
listed in `docs/2026-10-12-delivery-readiness.md`.

October 13 source follow-up for STD-032/033/035/036: explicit private stock
staging now declares `fzero-shim-ini-controls-1` for the actual ordinary consumer,
with its 11 delivered and 20 unsupported actions, exact existing ABI-2 pin,
direct-ROM/no-FFB limits and linked-source-stamp check. The same build receipt
is copied beside the private payload without a self-hash entry. Wheelkit's
isolated writer candidate routes only an already selected Shared backend through
this contract and the existing shared-profile serializer, preserving Native
controller keys and the force setting. Native/missing Backend keeps the Native
writer; unknown values refuse. October 13 offline validation passed all 48 focused
Wheelkit controls tests and all 32 unified-package tests. No new staged binary,
production Apply, runtime, multi-device proof, installation or public capability
follows from these source and fixture results.

Ordinary Shared admission now logs Core's existing bounded `profile_info`
response once at startup, including the accepted revision and exact profile-byte
SHA-256. This adds the missing runtime comparison point for a production Apply;
it changes neither the ABI nor the profile/recording format. All 12 focused
ordinary-host cases pass, and the private production Apply caller builds without
warnings. October 13 `E:/fz-probe-stock-apply-02` completed production Controls
Apply, ordinary profile revision/hash admission, a 900-frame boot/idle recording
and fresh replay with all 77 owner pins unchanged. This is not ordinary driving.

October 13 follow-up for STD-002/033/034/035: the same exact Applied Controls
bytes (`A983D5A7...9467`, revision `B081640C...A1E0F`) were reused by stock source
`51999e3` in a latched diagnostic Practice drive. Attempt04 has 2,213 exact
consumed words, real player countdown, 182 injected throttle frames and 61
steering frames/30 right PDM pulses; a fresh graphical snapshot replay verifies
all 2,213 WRAM checkpoints. Both normal exits and all 77 owner pins pass. Retained
REC/replay/verified images are available; replay capture helper exit4 is a
partial failed burst, not capture-performance proof. Native9B8A036F remains
unchanged. This advances diagnostic/input recording evidence only: ordinary
Wheelkit Play With Recording, physical bindings, wheel extras, Shared force
ownership, multi-device runtime and installed delivery stay partial. See
`docs/2026-10-12-shim-diagnostic.md`, October 13 stock result.

October 13 ordinary recording source follow-up for STD-002/012/032: independent
request-bound CLI status/Stop now reuses the actual recorder, preserves Ctrl+F11
and Saved/Verified terminal outcomes, and survives transient Windows status locks.
The existing installer/Recording.ps1 gain an ordinary receipt-owned companion;
private diagnostic build receipts cannot enable it. Interim installed recording
admission is Native only, with Shared ownership and automated-original isolation
still open. Offline native, companion and Wheelkit fixtures/build pass; no new
game build, launch, installation or public publication follows. See
`docs/2026-10-12-recording-cli.md`, October 13 ordinary source checkpoint.

October 13 installed Shared source follow-up: optional `-PrivateSharedInput` on
the existing Deluxe installer owns the same four private files and source PIN as
the stock stager. Its ordinary session receipt admits Shared only with this exact
inventory and `ForceFeedback/Enabled=0`; no configuration is rewritten. Public
packaging remains unchanged. Source/fixture work is not an installation or new
runtime qualification.

October 13 follow-up for STD-002/012/035: ordinary private Shared
automation adds explicit owner-state isolation separately from physical/network
output mute. The existing companion/installer bind it to actual Shared ownership
and a linked native isolation marker, with fresh private saves/keybind defaults,
fenced owner writes and request-bound Stop that leaves the game open. Native
automation remains refused. Candidate `026ec2a` passed focused native/companion/
installer/Wheelkit checks, a fresh Deluxe build and disposable install/rollback.
The actual installed companion recorded 699 title/menu frames, saved after Stop
while the game remained open, then verified all 699 WRAM checkpoints in a fresh
cold replay. Both games exited normally, all 77 owner pins matched and the lease
was released. The retained schema-1 Controls hash is `CA540938...387A9`, revision
`owner-provisioned-2026-10-09`; it is not the earlier Applied Controls case.
Thirty-two Left pulses prevent a strictly neutral claim. There is no ordinary
driving, physical-axis, force-signal or FFB qualification, owner install, schema-2
promotion or public push. The previous bf4c63a candidate has no automated
capability. Exact source/build/tape/screenshot evidence is recorded in
`docs/2026-10-12-recording-cli.md`.

The later clean local `9604802` Shared-only gameplay gate preserved the profile
and native ABI. A single muted ordinary Deluxe route with explicitly synthetic
Down/X/Z/Right keys reached player Practice driving at frame 1,813 (30.65 m/s,
383.53 m) and sealed 1,838 frames. The original runner refused its artificial
all-zero/180-frame tail before replay; the port marker is bit 30 and post-route
Right PDM remains legitimate input. A replay-only wrapper then verified all
1,838 WRAM checkpoints from that unchanged pair with normal exit and output
muted. Its first invocation's uppercase descriptor SHA was refused before game
launch; the corrected lowercase argument passed. The owned-window WGC capture
shows a Practice replay frame, but is not owner visual acceptance. Owner 77/77
and disposable protected 65/65 files matched after rollback; no owner install,
physical wheel/force, profile retune or public push occurred. Exact evidence is
in the portfolio hub's `artifacts/fzero-ordinary-driving-20261013` and
`E:/fz-ordinary-practice-runtime07` / `E:/fz-ordinary-practice-replay08`.

October 14 STD-032/033/035/036 source/offline closure: private descriptor
`369c926` publishes the exact four-slot C4 declaration and linked-consumer guard.
Game fixtures pass 34 package tests and 42 synthetic installer checks; actual
BF6455/five-file dependency staging passes. Wheelkit `2a063cc` is reviewed and
integrated: 260 focused tests plus one historical skip, clean App build and
actual-VM command gating/bind/save/reopen/first-clear/history/drift smoke pass.
This does not qualify a graphical consumer or runtime/physical inputs. The
follow-up HUD distinguishes admitted extras capability from extras-off warning;
clean private graphical build/stage is pending. Game source remains local-only,
with public publication deferred. Native force ownership/snapshot adoption is
separately deferred; schema-2 and other pending/unchecked rows remain open.

October 15 source follow-up for STD-035: Shared profile admission now bounds
the binary config stream to 1 MiB plus one byte before scanning the unchanged
Controls/Extras bytes. Four boundary/oversize cases extend the existing
ABI-fake host; they are authored but unexecuted pending a named slot. The
toolkit remains the binding parser and input owner. See
`docs/2026-10-15-bounded-shared-profile-read.md`.

October 15 source preparation for STD-013/030/035/036: the game selects a
keyboard/mouse-only Shared launcher provider and the paired UI suppression
flag, without invoking native wheel/force callbacks or saving legacy controls.
Callback-spy cases are authored; the explicit-ROM admission restriction stays
until the actual standalone F-Zero/C4 bundle and launcher action are connected.
Paired UI source rebuild is required. See
`docs/2026-10-15-shared-launcher-preparation.md`.

October 16 connected source for STD-013/030/035/036: paired launcher217ce445
and standalone helper source b8396ecc connect the existing external C4 editor
through a qualified optional controls-setup bundle. Runtime checks only the
bounded helper EXE/manifest identities. Configure result4 starts the exact
helper with PID/creationFILETIME, then exits before Core; the helper retains
its real process guard. Only a matching real bundle admits keyboard/mouse
Shared launch; absent bundle keeps the old refusal. Game-window Start rechecks
the current profile/backend/extras/force state before one existing owner.
All new fixtures/build/package/runtime checks are unrun; owner installation
and inline in-game editor parity stay open. See
`docs/2026-10-15-shared-launcher-connected.md`.

## Optional controls setup install ownership - source only (October15)

STD-032: existing installer gains explicit PrivateControlsSetup with PrivateSharedInput,
reuses setup_bundle.py admission and linked identities, records complete helper paths,
and restores/removes those exact files through its existing backup transaction.
Implicit withdrawal/unowned overwrite refuse. No owner settings or Deluxe gate change.
Authored inert ownership/admission checks remain unrun; no install/rollback claim.
STD-019 unchecked; STD-021/031 physical normalization and STD-037 schema2 remain pending.
See docs/2026-10-15-controls-setup-install-ownership.md.

STD-032 source review172399 successor: internal rollback uses its admitted
transaction inventory if final receipt output is truncated; manual rollback
keeps the existing ownership guard. Complete helper install receipts alone use
the reused JSON validator with an explicit 2 MiB cap; status/session reads keep
64 KiB and duplicate-field rejection. Focused inert-file cases are authored and
unrun. No owner install/rollback or live-candidate qualification is claimed;
the existing unchecked/pending standards above remain unchanged.

STD-032 private integration follow-up (October 16): source reviews172675/172942
found no blocker in a799891. One Test-ControlsSetupRecovery execution passed
14 focused inert-file checks; separate closure verified three original births
Absent, scope zero, exact pins and free lease. Evidence is hub dfef2463,
actual D39B25DB / independent E19F24DB. The integration retains exact tested
installer, Recording, definitions helper and fixture bytes. This qualifies the
focused recovery/receipt cases only; Test-InstallControlsSetup remains unrun,
and real installer/rollback, owner installation and physical gates remain open.
No standard is newly declared fully adopted. See
docs/2026-10-16-private-installer-integration.md.

STD-032 interpreter-resolution follow-up (October 16, request173822): the
helper admission route selects the first Application returned by Get-Command,
preserving normal PATH precedence and explicit SetupPython paths. Three
read-only checks of the production assignment passed (default plus explicit
Python3.12/3.14); no interpreter, installer or build ran for this check. The
retained three-application installer failure remains separate. Admission and
receipt behavior are unchanged; existing pending/unchecked standards stay open.

## Help repository links in existing Support - source only (October16)

STD-004/032, owner179543: the isolated source successor adds small text links
to the mod and Wheelkit repositories under Help in the existing
Support/Diagnostics feature. Diagnostics behavior, settings and force defaults
are unchanged. This uses the actual recomp-ui Mods renderer's existing
TextLinkOpenURL widget/opener, following the reviewed e8fca59 native Help
presentation contract without importing dbce_panel or a second UI framework.
Paired UI53e9350b81aca02fc9b4c2fbb3d795ea621bea6f adds optional bounded Help
metadata to the source feature descriptor; host/UI must be rebuilt together.
No build, fixture, link click, rendered observation, game or owner install ran.
The existing Wheelkit App ReportProblem flow is not directly reachable from
this launcher or bundled setup entry point; that route remains a separate gap.
No issue/report framework or browser draft substitute was added. Existing
unchecked/pending adoption states and frozen5f/D37/UI217/helperb887 stay intact.

October16 request181987: source-only Help report audit found the shared report
window/builder source-linked into helperb887, but no report-only entry or native
launcher callback. STD-004/030 remains partial; frozen483/UI53 advertises no
report action. See docs/2026-10-16-help-report-route-design.md for the smallest
reusable command/capability/host handoff and actual version/log/config gaps.
No new report framework, uploader, helper bundle, tests or runtime result.

October16 request182337 isolated source follow-up for STD-004/030: the existing
Support feature gains a capability-gated report host callback to the existing
shared helper report window/VM/builder. Explicit review/browser-draft-only;
small repository links retained. Old helper/Linux unavailable. Focused fixtures
authored, build/tests/publish/render unrun; adoption remains partial. See
docs/2026-10-16-help-report-route.md. Frozen483/UI53/808ae1f5 untouched.
## Opt-in launcher mouse diagnostic - source only (October16)

STD-004/030/035, request185827: existing SDL-to-ImGui Configure path gains an
opt-in LNG_MOUSE_TRACE=1 stderr observation (edges/scales/focus, logical bounds,
ImGui pressed/action and mapped host result). It changes no input or default
behavior and creates no persisted setting/log framework. Source only; matching
host/UI build and runtime remain unrun. Failed05 actual/closure are preserved;
coordinate/event-delivery cause remains unestablished. STD-019 unchecked and
STD-021/031/037 pending remain unchanged; all37 ledger rows are present.
See docs/2026-10-16-launcher-mouse-trace.md.

## Combined Help report and mouse-trace source (October 16, request190751)

The isolated successor composes the existing report route with the unchanged
opt-in mouse trace. Its UI retains the report callback and Configure trace;
the host retains both exact report dispatch and action-result observation.
No settings, input/force semantics or native pins change. Shared helper source
4eac3037 remains unchanged and requires a later matching report-capable bundle.
This combination is source only: build, focused executable tests, publication
and rendered acceptance remain unrun. Qualified A140BCF6 candidate and failed
runtime05-07 evidence remain unchanged. STD-019 unchecked and STD-021/031/037
pending stay open; all 37 ledger entries are present.

October16 request190853: composed report+trace source19afaf25/UI6d9b715 with
helper32bfd0bd now passes matching private build, report smoke, seven bundle
and five native/UI/no-bundle checks; 16 existing Core passes reused. This
advances offline STD-004/030 integration only; rendered/owner/physical and
STD-019/021/031/037 gates stay unchanged. See the existing Help report note.

October16 request193338 reconciles the delivery handoff against those original
results and the existing installer source. STD-032's optional Shared/helper
ownership and rollback are implemented; the old statement that only launcher
and WheelFfb are owned is historical. Previous c814 disposable install02 passed,
but exact report-helper/resolver candidate assembly and install/rollback remain
unqualified. STD-004/030's conditional Shared launcher and report action are
source-integrated and offline-built, not rendered acceptance. Shared physical
FFB, schema-2 and dedicated host save/rewind remain deferred implementation;
no ABI/pin/default changes or new tests accompany this documentation correction.
All 37 ledger IDs remain present; existing partial/pending states are retained.
See docs/2026-10-12-delivery-readiness.md for the exact remaining checks.

October16 request193417 closes that exact-candidate local assembly gate using
unchanged game19afaf25/UI6d9b715/helper32bfd0bd. Existing real installer with
default Python verified537 owned files, rollback restored the disposable
bootstrap baseline and final installation leaves a578-file private Deluxe
candidate. Three missing app-local release CRT files were added as separately
identified candidate assets; recursive native imports and unchanged owned bytes
then passed. Original failed import check remains retained. STD-032 advances
local packaging evidence only; no owner install, runtime/physical acceptance,
ABI/pin change or public redistribution approval. Existing partial/pending
states remain unchanged.


October16 request199074 adds the explicit default-off SharedForceBridge source
route using the existing force producer/call gate and optional owned snapshots.
Common+extras share one copied sample batch/Core tick; exact instance/product,
borrowed wheel metadata, source failure and stop-before-source-close guards are
implemented. Native600 six-step fake/device-free qualification passed. The first
game52-test run failed10; source fixes are prepared and a narrow continuation is
queued. The Release-disabled pure-owner assertions from that run are not credited.
Accepted runtime pins and private ready candidate remain unchanged pending the
paired shared DLL qualification; physical FFB is still unqualified. See
`docs/2026-10-16-shared-force-owner.md`. Existing pending/unchecked rows remain.


Corrected source `a720d34` now passes all52 affected fake-only CTests and the
four target builds, including always-active Release owner assertions, physical
sample-to-Core/guest-consumed force admission, common+extras one-tick ownership,
permanent source fault stop, normal/unresolved shutdown and legacy diagnostics.
Original run01 remains FAILED10/52; run02 closes children/query/exactlease at
00:45:08.337Z. This qualifies source ownership behavior only. Paired actual DLL
pins/private candidate adoption and physical normalization remain separate;
accepted native/shim pins and the ready Deluxe candidate remain unchanged.


The optional paired dependency pin is now qualified separately:
shared83DD742F (productd926566, fixturec1e2ddb, canonical83b8ce92) plus
native600EFA948E7 (source5948603). Explicit CMake admission/staging verifies
both hashes and x64, refuses tampering/conflicting routes, and revalidates on
DLL/pin replacement. Two unchanged DLLs are staged as a dependency-only
candidate with an exact receipt. The seven game runtime sources remaina720d34
and their52 passing cases stand. Default native/shim pins, ready Deluxe
candidate and owner install are unchanged. A graphical host rebuilt with this
pair and actual runtime/physical force are still unqualified; pending standards
remain pending. See docs/shared-force-dependencies-qualification.json.

## October 16 optional Shared host controls

Dedicated save-state browser/rewind bindings now use the same extras tick and
existing host panel handlers through an explicit six-slot declaration. Native and
Shared share the edge/strip helper; host bits never enter the guest or recordings.
Private staging/installer receipt publication requires an explicit host-controls
variant and linked consumer markers. Wheelkit game-specific admission and Apply
preserve flags, unknown settings and default C4 behavior. Focused qualification
passes: three fake target builds, 9 CTests, 3 package +1 host contract tests,
47 synthetic installer checks, and 61 Wheelkit cases including common host-only
dispatch. Original import/fixture failures and cleanup corrections remain recorded;
the exact final identity audit closes all children and the lease. See
docs/2026-10-16-shared-host-controls.md. All 37 ledger IDs are present.
STD-019 remains unchecked; STD-021/031 physical normalization and STD-037 schema 2
remain pending. No installed capability, ready candidate or force tuning changed.

## October 17 matching stock private host-controls package

The source-qualified host controls now have a matching stock graphical/headless
build and exact standalone helper. New common helper b695fe39 restores F-Zero
controls-only scope/import and the existing exact-process-exit/60000 ms contract;
the old f115 helper did not expose F-Zero and was not substituted. Current scoped
object manifests are admitted explicitly without removing wait/hash checks.
Three new manifest cases, incremental host builds, private staging, all577 archive
member hashes/linkage and packaged f-zero PlanOnly pass. Initial missing headless
C++17 compile failure is retained; prior passes were not repeated. No game, owner
install, native dependency rebuild, force tune or public distribution occurred.
See docs/2026-10-17-host-controls-private-package.md. The older Deluxe candidate is
unchanged; Deluxe portable admission and existing pending/unchecked standards
remain separate.

October 22 ledger comparison: all STD-001 through STD-037 are represented.
STD-019 remains unchecked and STD-021/031 remain pending physical normalization;
none is exercised by this source-only Native calibration task. STD-033/035/036
retain their partial status and per-device refusal. STD-037 gains canonical
Schema-2 Native Apply, consumption, preview and vector-fixture authoring;
Shared/Core, global capability, installed settings and physical acceptance
are not promoted. No new family-wide standard is introduced.

## October 22 installer suites under Windows PowerShell 5.1

All three authored suites ran under Windows PowerShell 5.1 against synthetic
temporary targets only (no owner install path, game or device touched):
Test-InstallControlsSetup 18 checks pass; Test-InstallNativePin 37 pass;
test_private_shared_install 50 pass. Initial failures, all fixture/installer
5.1 incompatibilities, were fixed: the native-pin fixture lacked the
`fzero.session-status@1` marker the installer requires; the shared test used
`ConvertFrom-Json -AsHashtable` and let 5.1 turn expected native stderr into
errors; Install-StreamDeck.ps1 used .NET Core `SHA256.HashData`/`ToHexString`.
The controls suite prints an expected Python "identity differs" traceback from
its refusal case. Not changed: tools/Recording.ps1 and Stage-SharedForceDependencies.ps1
still need PowerShell 7 (`-AsHashtable`, System.Text.Json); the Shared transition
case spawns pwsh 7 for the real installer.

## October 22 STD-032 Wheelkit-free path documentation

[docs/SETUP.md](docs/SETUP.md#wheelkit-free-private-install-and-bind-path) now gives
one ordered path (Install-StreamDeck.ps1 with explicit PrivateSharedInput and
PrivateControlsSetup, then binding on the launcher Controls page; Wheelkit
optional). Documentation only; no acceptance is claimed. Still unqualified:
closed-game helper binding (controls-setup `Start-ModSetup.ps1 -Game f-zero`
against a real install), real initial setup on a clean target, and the owner
install. STD-032 remains partial.

October22 Help report route offline recheck on main cfd4e26: fzero_mods, fzero_setup, fzero_setup_no_bundle and the ten setup_bundle report-marker tests pass. Report action stays unavailable for old controls-only helpers (marker0), isolated sessions and Linux (source-confirmed). No new framework, uploader or HTTP submission; STD-004/030 states unchanged. See docs/2026-10-16-help-report-route.md.

## October 23 private Deluxe package route source

The existing unified stage/package scripts now offer an explicit local private
Deluxe route with exact Native2 installer/helper admission and fresh-scratch
rollback provenance. Default stock/public fences remain. Forty inert stage/package
tests pass; no fresh helper/product build, package, owner installation or device
acceptance is claimed. See [the private route](docs/UNIFIED-PRODUCT.md#october-23-private-deluxe-native2-packaging-source).
All 37 ledger IDs remain represented. STD-019 stays unchecked, STD-021/031 physical
normalization stays pending, and STD-010/033/035/037 acceptance states are not
promoted by source packaging checks. The frozen stock package is unchanged.
## October 26 selected-device consumer contract

STD-033/035/037: production Native planner now checks both instance and product
identity for non-steering bindings. Device-free contract passes 913 checks / 207
vectors after 16 conflicting-product failures demonstrated the defect. No writer,
vendored header, package, runtime or physical qualification changed. STD-019 stays
unchecked; STD-021/031 pending; STD-037 partial. See
[consumer evidence](docs/2026-10-26-cardinal-consumer-contract.md).


October26 refusal-review follow-up: current ledger contains STD-001..040; missing
STD-038..040 are now recorded above. STD-019 remains unchecked, STD-021/031
pending, STD-037 partial. Accurate product-conflict rebind feedback supports
STD-004's settings UX; deployment and physical qualification stay unchanged.

October26 DBC-328: raw_snapshot hat loop now explicitly bounds indexing by its
four-entry array capacity in both SDL branches. Existing 0..4 count clamp and
cardinal/neutral behavior are unchanged. SDL3 Release consumer compile/link
passes without the reproduced GCC bounds warning; fixture not executed.
No acceptance status promoted. See docs/2026-10-26-raw-snapshot-hat-bounds.md.

## October 29 clean-install plan ledger refresh

The current ledger contains STD-038 through STD-040, which were absent from the older 37-entry snapshot above.

| Standard | Title | Status | Notes |
|---|---|---|---|
| STD-038 | Game window lands on the racing row | unchecked | Fresh-package qualification must observe actual window placement; no runtime evidence added by the plan review. |
| STD-039 | In-process input instead of a virtual controller | unchecked | Existing Native/Shared paths need a current consumer audit; no new compliance claim from packaging preparation. |
| STD-040 | Standalone configuration and one complete package | partial | Stock-only Setup.cmd/launcher package path exists; docs/CLEAN-INSTALL-20261029.md covers the intended ordinary flow. Real package dependency closure, ROM pick, configuration, restart and F6 parity remain unqualified. |

This refresh records gaps only. No build, public push, install or hardware action; existing pending physical/FFB and F6 parity gates remain.

## November 2 local branch reconciliation

The retained egon/lamo source deltas are bounded-ported locally; dine is artifact-only and remains unmerged. ROM-free native 77/77, controls 207 vectors/1,534 checks and inert Python packaging 51/51 pass. Owner-approved public source push after explicit fork visibility check; no player package, install, runtime/device or owner acceptance. See [reconciliation evidence](docs/LOCAL-RECONCILIATION-20261102.md).
## November 3 provider-free readback standards refresh

Compared the current shared ledger with this file before preparing the isolated
runner. STD-019 remains unchecked; STD-021/031 physical normalization remain
pending; STD-037 remains partial. Existing partial/pending rows stay open.

| Standard | State | Scope in this change |
| --- | --- | --- |
| STD-041 | unchecked | Current corrected triple camera behavior needs fresh graphical qualification. Replay runner changes no product camera. |
| STD-042 | unchecked | Per-camera saved framing/naming is outside this replay check. |
| STD-043 | unchecked | Held-key/hotkey behavior needs ordinary input; replay bypasses it. |
| STD-044 | partial | Runner snapshots preserve exact bytes, attributes and UTC ticks; product settings-writer adoption remains unqualified. |
| STD-045 | pending | Saved owner force choice is preserved. New-install On migration is not implemented here; unattended replay latches output off. |
| STD-046 | unchecked | Camera starting choices and offsets need separate implementation/qualification. |
| STD-047 | partial | This scratch replay uses one window, without qualifying production triple-span policy or geometry. No separate-window test or owner layout change. |
| STD-048 | unchecked | Ordinary numpad direction/custom-binding migration is outside replay. |
| STD-049 | n/a | SNES 2D vehicle representation has no bonnet camera or cab-over mesh placement route. |

See docs/2026-11-03-provider-free-readback.md for candidate identity and evidence
boundaries. Public source/binary distribution and owner acceptance are unchanged.
