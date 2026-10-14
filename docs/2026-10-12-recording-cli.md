# Recording controls and shared-profile preview

Private-copy CLI checks passed October 12, 05:53–05:59 CT. Nothing was installed
in the owner's game or promoted as its default input backend.
The [earlier player Practice original](2026-10-12-shim-diagnostic.md) remains
immutable: all 2,351 final consumed input words matched its shared capture,
and the pinned headless runner matched all 2,351 WRAM checkpoints. That proof
uses the earlier original runtime. The new graphical CLI also replayed that
unchanged tape with all 2,351 WRAM checkpoints matching.

## Explicit launch options

From a candidate folder, with the same ROM, Deluxe/MSU mode and display settings
that will be used for replay:

```powershell
./FZeroSNESRecomp.exe --record E:/my-recordings/first-drive.fzpt ./fzero.sfc
./FZeroSNESRecomp.exe --replay E:/my-recordings/first-drive.fzpt ./fzero.sfc
./FZeroSNESRecomp.exe --shared-profile-preview E:/fz-owner-preview-01 ./fzero.sfc
```

`--record` uses the selected ordinary input backend and force settings. It records
from boot, including menus. It is **not a muted unattended test mode**. Choose a
fresh `.fzpt` path inside an existing directory; its `.fzpt.state` sibling must
also be absent. Neither file is overwritten. Keep the pair together.

The REC banner is at the top of the centre game view, on both SDL and OpenGL
presentation paths. **Ctrl+F11 stops an active recording** and shows Saved only
after the recorder closes successfully. Ordinary play can continue. Plain F11
and Shift+F11 retain their existing save-slot behavior. Loading a state, resetting
or opening the state/rewind overlay while recording invalidates the tape and
shows Error; this also makes the final process exit nonzero.

`--replay` consumes the recorded final SNES word and checks every post-frame WRAM
hash. Its REPLAY banner shows progress. It closes at the end and shows Replay
verified only after a complete close. Closing early or diverging is an error.
Recorded playback bypasses physical input, host gestures and overlay filters before
reading the tape, and explicitly skips self-test/state/viewport script callbacks.
It initializes neither the wheel-force device nor the telemetry transport and
does not apply the controls profile, create missing keybinds, migrate SRAM or
save video defaults. A CLI replay goes directly to its explicit ROM, so no
launcher settings page is opened or saved. ROM identity and the snapshot are
validated; the caller must retain the original presentation, Deluxe and MSU
configuration. For automated evidence, the existing pinned case adapter remains
the path that also verifies module/data/config identities.

All three options require an explicit ROM and reject `--launcher`, duplicates,
combinations, missing arguments and pre-existing recording/replay/injection
environment requests. Relative paths resolve against the caller's directory
before the host anchors its own configuration directory. Startup failures print
an error and return nonzero without falling back to ordinary play. Errors after
the game window exists show the Error banner. Normal launch without these flags
continues to use the existing backend and settings.

## Shared profile preview

`--shared-profile-preview` is a bounded, explicitly muted preview of physical
wheel input through the existing production shared ABI2 provider and F-Zero
mapper. It uses `[Controls]` in the candidate's `config.ini`, keeps keyboard SNES
input available, and has **no session/batch injection**. The fresh short directory
contains `original.fzpt`, its state, the frozen controls/config and the existing
shared capture with computed force/telemetry signals. It is not a new format.

The preview banner says MUTED PREVIEW. It blocks physical FFB and UDP/motion
delivery while retaining computed signals, uses private save storage and avoids
owner configuration writes. Ctrl+F11, Esc or normal window close stops the shared
provider and both recorders before showing Saved. It closes automatically after
230 seconds, before the diagnostic provider's 240-second hard limit. A provider
or recorder failure prevents successful sealing/exit. Startup may wait for the
configured physical devices; this mode does not fabricate missing input.

The current shared mapper can represent steering, throttle, brake, Confirm,
Back, Start, Select and menu navigation. ABI2 has no independent SNES X/Y/L/R or
host save/rewind actions, so those wheel actions are explicitly unsupported
here; they are not aliased onto unrelated generic actions. Existing keyboard
SNES bindings remain usable in this preview. This action-vocabulary limitation
also applies to the current ordinary Shared candidate. Multi-device
normalization is provided by Core, but a real owner wheel+button-device drive
has not yet qualified this consumer.

These diagnostic restrictions are **not permanent product requirements**.
The [ordinary Shared candidate](2026-10-12-ordinary-shared-input.md) separates
input ownership from preview persistence/deadline policy. Its first SDL
supplement experiment hung during device acquisition. Corrected `7187c57`
removes SDL wheel reading entirely from Shared; a private direct-ROM 900-frame
at-rest original and fresh graphical replay passed at 07:03–07:05 CT. Legacy
wheel extras remain explicitly unavailable, and Shared+FFB remains refused
pending device-ownership integration. No installed backend or tune changed.
A real unlatched owner gameplay recording and cold replay are still needed.
Launcher/Wheelkit record/replay buttons are also unimplemented; the CLI gives
them a direct entry point.

## Source validation

`fzero_session_ui` exercises the production parser and real `.fzpt` recorder:
108 checks cover CLI ambiguity, unchanged F11, clean nonempty sealing, verified
replay, early replay close, empty recordings, shared-provider failure, sticky
invalidation, a real read-only stdio close failure, and production tape dispatch
that never invokes the live input/host-action callback (including at EOF). The targeted 16-test
recorder/shared-provider suite and the graphical build pass. The only compiler
warning is the existing Windows `HIBYTE` macro redefinition.

## Private CLI and rendered-status qualification

All five runs used frozen clean source `26a3f0e239b03ac970654ec99c1f9f57bee0900f`
(tree `5ef43de2f4752b02bc84f2e0dfc2687513c65bb9`). Runtime SHA256:

```text
FZeroSNESRecomp.exe 5973ac8e49263915c39840081501e212fda6c112842ebd6e950b4b184eb88735
dbce_shim.dll      9b8a036f664cf8f45cccf82b98e5e4376f93215b60eac50b6db965bf498015d8
WheelFfb.dll       3f6cc51444879d01feb900098b2de94242594d093f523e3c34bd511d69c0d946
```

The wrapper `E:/fz-cli-research/Invoke-CliCopy.ps1` uses the actual CLI, the shared
lease and idle check, an exact owned child, copied game/assets/config and read-only
owner pins. Every game exited normally with code 0, its active recorders resolved,
77 owner file/absence pins unchanged and the lease released. Preview/replay use
the pre-initialization no-force/no-transport latches; ordinary recording used
only a **private copy** with ForceFeedback/Enabled and Telemetry/Enabled set to 0.
No session/batch injection or OS input was used in these five runs.

| Private evidence folder | Actual result |
|---|---|
| `E:/fz-probe-cli-01` | Capture-only shared-profile preview sealed 900 frames; P21 14,679 checks passed. |
| `E:/fz-probe-cli-old` | Existing Practice tape replayed graphically; all 2,351 WRAM checkpoints match. Original hashes/config preserved. |
| `E:/fz-probe-cli-record` | Ordinary backend recorded 900 frames; rendered REC and Saved captured. |
| `E:/fz-probe-cli-fresh` | Preview-01 replayed all 900 WRAM checkpoints; rendered REPLAY and Replay verified captured. |
| `E:/fz-probe-cli-02` | Fresh capture-only preview sealed 900 frames; P21 passed; rendered MUTED PREVIEW–REC and Saved captured. |

The two fresh previews each contain exactly 900 shared `game.valid=1` rows, with
simulation frames 0–899. Every final consumed `game.inputWord` equals its `.fzpt`
word, and no action is marked injected. Capture-only mode has no extra terminal
game row; its successful shutdown is recorded in `host-shutdown.json`. The earlier
latched Practice original retains its extra non-game terminal row. Both fresh
preview tapes have identical bytes, as expected for this at-rest/menu workload.

```text
old Practice .fzpt 4955c7ef6a6887c86d8de060bb99f64ebcfa8c3bc4e3e4da09ab2ea3712870d3
fresh preview      0142307c378f6a9c09a193c99d33e0876d465937e1ac066d79a14287fc568880
ordinary record    1b8e68cdbff92f4572e15e49907f9e423052750d862799b31b88d10d551510d8
all initial states 18bda07046cd36c3dceca4ca26d38fa0d7317ed325ff29390af34c54dc8042db
preview-02 source  92d0d23e1347b3d3f46e16f0eddbbb82150aeff20ff451cba9b3c68a4e4db706
```

The independent retained `E:/fz-cli-research/verify-cli-evidence.py` verifies the
old 2,351 and both fresh 900 consumed-word sequences plus all five results.
Its report is `cli-evidence.json`, SHA256
`62ba33e306b8c9a96b92c829498b9b1dfc50a76b6de0788d3c744060de05535e`.
The old original, its sealed capture and its prior headless comparison were not
rewritten. The current graphical replay provides an additional regression check.

Inspected game-window frames (private, retained at these paths):

- Ordinary REC: `E:/fz-probe-cli-record/frames/burst/frame-0001.png`.
- Ordinary Saved: `E:/fz-probe-cli-record/frames/burst/frame-0158.png`.
- Preview REC and Saved: `E:/fz-probe-cli-02/frames/burst/frame-0001.png`
  and `frame-0160.png`.
- Fresh REPLAY and Replay verified: `E:/fz-probe-cli-fresh/frames/burst/frame-0001.png`
  and `frame-0144.png`.
- Old Practice tape at frame 1,441/2,351: `E:/fz-probe-cli-old/frames/105453928.png`.

The first preview and old-replay burst helper rejected `--duration`: the toolkit's
existing built executable was older than its source. Those failed helper receipts
remain. The old replay's one-shot frame succeeded. Later copies use the current
read-only source built privately at `E:/fz-cli-research/wgc`; their PNGs are valid,
but WGC exit 4 means the target closed before the requested burst ended. That is
partial image evidence, not a successful full-duration capture. Final wrapper SHA:
`6738bf0fc5916b4d64bede0f0920fb37c0dac035d3167730b83b2521ebc620ec`.

**Scope:** the new recordings are title/menu, physical-at-rest checks. The old
Practice original supplies player-drive replay coverage. Normal closure used a
900-frame bound, so **actual Ctrl+F11 event routing remains untested**. The images
prove the current SDL presentation; they do not separately qualify OpenGL or
triple views. No new owner wheel drive, physical FFB acceptance or normalization
claim follows from these checks.

## Next owner-facing integration

1. **Wheelkit discovery and commands:** declare the CLI capability only for a
   receipt-pinned build that contains it. Play with Recording needs a fresh `.fzpt`
   destination plus explicit installed ROM; Play Recording keeps the `.state`
   sibling and matches original Deluxe/MSU/config identities. Discover complete
   tapes separately from incomplete/error recordings; retain originals and show
   the runtime's success/failure. The existing pinned replay-case adapter already
   describes these extra identities; do not make a second recording container.
2. **Ordinary shared input:** the installed launch still uses its existing raw
   backend. The new source-only opt-in backend retains the owner's SNES extras
   and separates diagnostic fences. Qualify actual unlatched wheel/menu gameplay,
   resolve force/read ownership, and retain Native as the default until then.
3. **Release and owner checks:** retain the reviewed source/ABI pins, build the
   proper installer and distribution variant, then verify installed CLI discovery,
   an attended Ctrl+F11 stop and a real owner recording/replay. Deluxe embeds
   cartridge data: the existing public-packaging boundary still applies.

## October 13 ordinary companion source checkpoint

The existing `--record`/`--replay` pair now accepts `--session-id` (32 lowercase
hex characters) and `--session-status` (fresh file) on Windows. This is independent
of diagnostic injection. `fzero.session-status@1` binds action, state, actual PID
and creation FILETIME, frame counts, effective ROM hash and output-mute state.
An exact request-bound `status.json.stop` closes the existing recorder and leaves
the game running, as Ctrl+F11 already does. Plain F11 is unchanged.

Status publication is atomic and retries transient sharing locks using only its
own temporary file. Status transport failure and foreign/malformed Stop files
do not invalidate a recording. Once the recorder closes successfully, Saved or
Verified remains its terminal outcome; later game/supervisor errors stay separate.
`--mute-outputs` only mutes delivery and does not change save/config/input behavior.

`tools/Recording.ps1` adds an installed companion route over the same `.fzpt` and
`.fzpt.state`, with a small hash descriptor for library discovery and ROM/config/MSU
identity. It also pins the exact optional `mods/bs-deluxe.dat` override, including
its absence for an embedded-only Deluxe run. Adding, changing or removing that
override refuses playback before launch. Native game replay still validates the
snapshot and every WRAM checkpoint.
The ordinary installer must own the launcher, WheelFfb runtime and exact companion,
and stamp `fzero.session-capability@1` after checking the linked source marker.
Existing owner receipts and private stock build receipts do not qualify. The
interim installed recording admission is Native only; installed Shared shim
ownership is the next bounded addition. Replay remains muted and uses the existing
private save-root option. Automated originals are refused until owner-state
isolation is reviewed; human recording retains its existing output/progress policy.

Offline checks: 130 native CLI/UI checks, 282 control checks (including a real
Windows deny-delete status reader, continued recording/Stop/recovered publication,
and 61-frame tape replay), 16 PowerShell helper checks, 191 Wheelkit session tests,
and a Release Wheelkit app build with zero warnings/errors. The initial Wheelkit
test run retained one fixture-only mixed-separator assertion failure; the corrected
absolute-path assertion passes. Evidence is in the private hub's
`artifacts/fzero-ordinary-session-20261013`. This checkpoint has no new game binary,
game launch, installation, physical input/force proof or public push. The previous
stock diagnostic Practice evidence remains unchanged.

### October 13 optional installed Shared ownership source follow-up

The existing Deluxe installer now accepts explicit `-PrivateSharedInput`. It owns
exactly the same four private files as the stock stager (`dbce_shim.dll`, header,
PIN and provenance), derives the ABI/revision/runtime hash from the source PIN,
and checks actual DLL/header bytes and the linked Shared adapter marker before
any write. It preserves the ordinary Deluxe build identity; the earlier stock
binary is not relabeled as an installed launcher. Public packaging is unchanged.

`privateSharedInput` in the existing install receipt names that inventory, and
`session.recordBackends` admits Shared only with it. Both companion and Wheelkit
check ownership, source/runtime agreement and actual force-disabled semantics.
No backend/configuration/force setting is rewritten. A Native-only update over
previously owned Shared files refuses before mutation; retain the explicit switch
or roll back the Shared installation first. Backup/rollback owns all four paths.

Focused offline checks for that ownership follow-up passed: 33 companion
assertions, 24 installer inventory/rollback assertions (including two real
entry-point Shared-to-Native refusals against disposable fixtures), 112 Wheelkit
session tests and the Release app build with zero warnings/errors. Nine initial
legacy Woden trajectory-fixture failures were preserved and corrected by
selecting their historical protocol explicitly; the new ordinary Woden default
has separate lifecycle fixtures. Evidence is in
`artifacts/fzero-shared-woden-sessions-20261013` in the private hub. No native/game
rebuild, owner install, game launch or public push occurred for that follow-up.

### Ordinary automated-original isolation and title/menu replay qualification

Private Shared candidate `026ec2a` adds explicit `--isolate-owner-state <root>`
alongside `--mute-outputs`, with the existing request-bound ordinary Record
arguments. Isolation requires a fresh short local root and the ordinary Shared
backend. Native automation and alternate network/mod callback builds refuse.
The host creates that root, copies the selected keyboard bindings there, routes
save directories, performance diagnostics and crash reports there, and fences
owner configuration writes, legacy SRAM migration, save-state/rewind gestures
and the debug server for the whole process, including after Stop. It starts
from fresh guest state; the existing snapshot captures that actual initial state.
Keyboard SNES input and Ctrl+F11 remain available. Human recording and output
mute alone retain their prior semantics.

The existing companion binds automated identity before startup, verifies the
native muted status and unchanged ROM/configuration/Deluxe/MSU inventory, and
seals the existing tape/state pair when Saved while the game remains open.
Its existing deadline requests Stop, allows a five-second continuation, then
requests normal close; exact-process recovery retains unresolved evidence.
The private save root is retained and named in the existing companion metadata.
The installer declares automated recording only for its owned Shared inventory
and a freshly linked executable containing the isolation seam. Prior receipts
cannot acquire that capability from a nearby helper or DLL.

This uses the existing no-force and no-network latches before initialization;
Shared also avoids legacy SDL controller/force acquisition. Software force and
telemetry calculations are unchanged. Ordinary `.fzpt` captures consumed input,
not a force-signal stream. No diagnostic input channel is enabled by isolation.

October 13's leased batch passed two native test targets, 44 companion checks,
29 installer checks, 119 focused/shared Wheelkit tests and an App build with zero
warnings/errors. The fresh embedded-Deluxe build passed its linked source stamp,
runtime closure, seven-file Shared installation and disposable rollback. Graphical
SHA-256 is `A6E2AFF80D4841CE8020D8FF197930290F3C23DFB6101277D83DFD6612E2C6AA`;
headless is `BD4B87F65DB98A20B5E1886ED9C6F678CB1A077F983864473F709793688FE190`.
Native ABI-2 `9B8A036F...015D8` and WheelFfb `3F6CC514...D946` are unchanged.
The game build retains two existing `HIBYTE` macro redefinition warnings.

`E:/fz-ordinary-isolated-runtime02` then ran the actual installed ordinary
companion with `-Record -InputOrigin automated -Seconds 30`, without diagnostic
flags or OS input injection. The request-bound companion Stop sealed **699
frames** while the exact game process was still open. Native status acknowledged
output mute, the live module inventory contained no WheelFfb DLL, and ordinary
Shared shutdown resolved with input released and zero held inputs. The game
closed normally. A fresh companion replay verified **all 699 WRAM checkpoints**
from that exact input/state pair, with a different request/process and normal
exit 0. It opened no live Shared input backend.

The accepted profile was **Moza KS Pro Triple**, revision
`owner-provisioned-2026-10-09`, exact Controls SHA-256
`CA540938048AAE42C8185BFC6236ADB4CE11946C16F435ED63F5E0F4348387A9`.
This is the retained schema-1 profile, not the earlier `A983D5A7...` production
Apply case. Config/keyboard/video, ROM, embedded Deluxe override absence and
selected MSU identity remained unchanged. The effective ROM was
`419EED641CFA20013D1E9B8270062D90155051A441441A2118BF23846AC8D747`.
All 77 owner file/absence pins matched, the disposable install rolled back, and
the shared lease was released at 13:33:40.508 CDT. Owner activity was explicitly
allowed by the existing policy; no idle observation was fabricated.

Inspected center-window images show only the title Grand Prix/Practice menu:
REC at 10.8 seconds, SAVED 699 frames, and REPLAY 155/699 with Forces Off. The
tape contains 667 words `0x40000000` and 32 words `0x40000040`. Bit 30 marks the
active input port; bit 6 adds SNES Left. Left occurs every 21–22 frames, consistent
with a small steady PDM steering offset, but the ordinary tape cannot attribute
it to a particular input source. This was **not strictly neutral**, player
driving, physical-axis qualification, force-signal capture or FFB acceptance.

| Retained artifact under runtime02 | SHA-256 |
|---|---|
| `recording/recording.fzpt` | `580512598D86EAA568ED8BAE8529051CF1F1D01F6FE87317A4C2F56352B8D6CC` |
| `recording/recording.fzpt.state` | `18BDA07046CD36C3DCECA4CA26D38FA0D7317ED325FF29390AF34C54DC8042DB` |
| `recording/recording.json` | `14DD8DCD7D8884EAF16D3AF887AAFE5C73440A0C0B3AF5007BB4E785C5366EAD` |
| `frames/recording.png` | `DBFD6888AF6288C87CBF9F92E357292627F0C864B74156EE138C8A95F8FC3342` |
| `frames/saved.png` | `EF0015003F9729D3ECECBBD777553D6ED0744D248B42C3FDECE1B8D68207ECAF` |
| `frames/replay.png` | `F5C65E265DCB28028A326383953EFDC31516615E5A810686B63CBE30A0455DC0` |

The preserved runtime01 failure occurred before any game launch: the private
supervisor called the companion's self-hash helper outside its script context.
The corrected private recipe leaves real companion validation intact. No consumer
change or repeated game attempt was needed. Owner state and rollback passed for
both copies. The older stock/Deluxe diagnostic tapes remain immutable.

The next meaningful ordinary driving qualification can use the existing keyboard
passthrough with explicitly synthetic input and the exact Deluxe Practice route;
it would exercise ordinary consumed keyboard input, not Shared axis normalization.
The existing Core injection transport remains diagnostic-only and is refused by
ordinary sessions. A live route/source guard must be reviewed before any driving
run; neither an idle repeat nor a diagnostic tape substitutes for that evidence.

The source-only route-witness candidate adds an optional `-RouteWitness` switch
to the installed companion, forwarded as `--route-witness` only for automated,
muted, isolated ordinary recording. It extends the existing atomic
`fzero.session-status@1` JSON with a `routeWitness` object, not the `.fzpt`
format or an input command. After each successfully recorded guest frame, it
reports the final consumed input word, zero-based simulation frame, raw
`$54/$55/$56` state, read-only Practice/Deluxe menu selectors, and independently
computed software speed/distance. The enclosing status already carries exact
request ID, PID, process creation FILETIME, ROM SHA and tape frame count. A
driver must require all of them, `state=recording`, `valid=true`,
`frames=simulationFrame+1`, and a strictly advancing fresh frame before each
key action. The driver must causally match its own navigation/confirm consumed
words (and Start if used) to the selected Practice/menu transitions in the
same request, then establish
the real player countdown `(2,2,2)` before live `(2,3,0)` with movement.
`$58` training selection is a stronger Practice witness; `$5B=0` is only
supporting data until attract behavior is captured. A live-looking attract
state alone is insufficient. The snapshot stops after 6,000 frames and marks
itself invalid, while ordinary session status continues. If status publication
is temporarily blocked, the recorder continues and the driver must refuse a
stale row. The disposable run must also retain frame-time diagnostics because
per-frame atomic status updates can affect the timing of short keyboard taps.
Mufa passed the witness as a passive source provider; the route driver and
disposable runtime remain unqualified.

The separate `tools/fzero_ordinary_practice_route.py` candidate reads the
fresh companion/status pair for one request and verifies PID, creation
FILETIME, executable path, and the unique visible SDL window before posting
keyboard edges to that exact HWND. It never changes foreground focus.
It requires an advancing post-frame witness and observed release before each
Down/X menu pulse. Every pulse must appear in the consumed word and lead to
the exact next Practice selector. It then requires the player countdown before
Z throttle and Z+Right steering, and prints bounded step and observed-status
timing summaries to stdout. It sends no Stop, changes no configuration and
never drives replay; the existing companion owns Stop/seal/close and replay.
These status-poll intervals are not a substitute for game frame-time
diagnostics. Mufa closed the driver's ROM, status-read, startup and key-hold
review findings. The posted-key transport and private runner still need
disposable runtime qualification.
The driver's `--release-only` mode sends idempotent key-ups for exactly
Down/X/Z/Right to the same request's verified live game HWND after the route
process exits; it cannot press a key. If that exact process exited or its PID
was reused, release is a reported no-op because its SDL key state is gone.
The private runner must keep its lease and copy intact if the driver remains
live or cleanup fails against a still-live process.
