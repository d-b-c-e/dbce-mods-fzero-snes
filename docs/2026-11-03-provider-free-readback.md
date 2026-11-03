# Provider-free F-Zero readback

Assignment 501336; operator napo. Isolated branch
`napo/fzero-readback-20261103`, based on local main `2eeda80`.
Read AGENTS, the hub F-Zero profile, reconciliation `c2b63db` and publication
receipt `78ac0a1`. The profile is an October 4 snapshot; reconciliation establishes
source/ROM-free fixtures only. Public origin remains untouched by this task.

## What each route can establish

| Row | Provider-free assertion and limit |
| --- | --- |
| Recording/playback | R0 headless validates every sealed input/WRAM checkpoint. R1 graphical playback validates those checkpoints, exact session ID/PID/birth/ROM, advancing frames, terminal `verified` and normal exit. Synthetic recorded case, not physical acquisition. |
| Rendered game | R1 final game BMP is guest-renderer output, not scanout. R2 two exact PID/birth/SDL HWND WGC images correlated with advancing status can demonstrate fresh visible game content; inspect pixels after exit. |
| Controls | ROM-free controls/companion fixtures prove parser/policy behavior. Replay bypasses controls and cannot prove current-main Native2/cardinal bindings, Apply consumption or current devices. A fresh diagnostic with consumed words and movement is needed. |
| Telemetry | R0 fixtures and replay-derived models are available. Graphical replay mutes UDP and skips telemetry delivery. Live UDP, lap/rank/time publication and SimHub remain unverified. A separately fenced gameplay/loopback route is needed; SimHub stays untouched. |
| F6/F7/settings | Pure serialization can prove owned-key behavior. Playback disables interactive F6; this run cannot pass F6, real-key F7, persistence or Wheelkit Apply. A fresh current graphical candidate and ordinary diagnostic path are prerequisites. |
| Triples | Captures document the selected candidate only. This single-window stock run cannot qualify side vehicles/effects, seams, curved projection, racing-row placement or full STD-047 adoption. |
| Owner acceptance | Physical wheel binding, torque/FFB normalization, comfort/seat view, owner choices and attended visual acceptance remain attended gates. |

## Exact candidate and run

Reuse the receipted stock package at
`E:/Source/temp/fzero/candidate-2b088a0/extract`; compiler source
`9dc40e99f5d3b5bc6d17f44c5ee2fbfd0dba2914`, game SHA256
`062FE593621891BE1687E8B8E9619DA5FBCB5589A3FDEB3344C7C6FC6919B9E4`.
It predates reconciliation and is never represented as its binary. The immutable
private case at `%LOCALAPPDATA%/Dbce/StagePlayback/SessionEvidence/fzero-stock-synthetic-20261008-0346`
has 3,600 frames, stock ROM SHA256
`BF16C3C867C58E2AB061C70DE9295B6930D63F29F81CC986F5ECAE03E0AD18D2`.
No ROM, save state, private trace, binary or helper is tracked or published.

`tools/rig/Invoke-FzeroReadback.ps1` uses production `--replay` and session status.
Prepare checks the full package manifest, copies into an isolated app, pins the
runner/capture closure and case, and sets scratch-only windowed 16:9, FFB off and
telemetry off. Rehearse runs the actual headless replay in a suspended-start job;
no window, hook, input or device. Live requires that rehearsal, one GO, raw idle
>=180 s and the named `napo-fzero` lease, then records foreground/display/input.

Live opens one window, uses the GPU for up to 80 s and emits no OS input.
Playback latches force/network output off and excludes controller acquisition.
The prelaunch foreground may remain unchanged; one natural transition to the
exact new game PID/birth is allowed, then fixed. Any physical/injected LL event
or foreign foreground aborts terminally. The LL message pump has a monitored
heartbeat and stop/join evidence. At most three raw idle resets with zero LL
events are tolerated within 90 s of owned birth, only on that game or unchanged
prelaunch foreground. No activation/focus fallback or retry in the GO.

Capture the exact SDL HWND at advancing frames >=600 and >=1800. Capture children
enter the same job before execution, each bounded to 8 s. Verify distinct PNGs,
dimensions, final game BMP and matching graphical/headless final WRAM. End at
replay completion. Close only owned windows/job members, verify empty job, stop
hooks and verify display identity. Owner config, selection/receipt and local
F-Zero state are snapshotted/restored using type-aware UTC conversion; a separate
numeric-tick verifier checks SHA/ticks/attributes. Archive changed/new files
before restoration. Cleanup uncertainty retains custody. No Steam action is used.

## Offline evidence and next action

First rehearsal `.local/readback-01/rehearse/result.json`: all 3,600 WRAM
checkpoints, exit 0, empty job. No window/device. Focused fixtures: 14 policy,
22 snapshot/restore (independent child included), and 51 existing companion checks
pass. The requested single GO was sent and approved as 501445.

## GO501445 result and corrected ingress

`FAIL_HARNESS`, no rendered/gameplay assertion. The initial admission refused
the historical owner path because `E:/Source/toolkits/launchbox` is a junction.
Zero processes or owner writes; that refusal and release remain at
`.local/readback-03/admission-refused-junction`. The canonical owner directory is
`E:/Source/tools/launchbox/Launchbox-Racing/Games/Windows/F-Zero (SNES Recomp)`.
Its relevant bytes matched through both paths before continuing admission.

The owned game PID41304, birth UTC ticks639293015959616592, started
2026-11-03T11:19:55.9616592Z, then exited2 during option parsing:
`Explicit session conflicts with existing recording/replay/test environment`.
The runner had incorrectly combined explicit `--replay` with the headless-only
`FZERO_REPLAY_PLAYTHROUGH` environment. No SDL window, session frame, capture,
controller or output assertion followed. This is a runner failure, not product
failure or successful replay. No live retry followed this result.

The named lease30e0479283ce497999c9c3f14124d1e6 was released. Job empty;
LL physical0/injected0/startup-resets0, hook heartbeat1 and monitor heartbeat4,
hooks released/thread joined. Foreground remained VS Code PID26220, birth
639292453274157775, HWND460684, with four-display topology unchanged.
Type-aware restore/presence enumeration passed; independent numeric verifier
passed all5 original owner files (SHA, exact UTC ticks, attributes). Local F-Zero
state directory presence was unchanged. Evidence: `.local/readback-03/live`.
FINAL-release was sent to remi, soba and kole; no custody remains.

`Replay-Invocation.ps1` now gives graphical playback only explicit CLI arguments,
and headless only the legacy replay environment. A small console-only probe calls
the real production `FzeroSessionParse` with the actual invocation returned to
the runner: corrected live accepted (mode2/env0), headless accepted (mode0/env1),
and the original mixed request reproduced/refused (exit2). All3 checks pass at
`.local/ingress-01/result.json`, without SDL, game launch, hooks or devices.
Corrected preparation/rehearsal is `.local/readback-04`. The fresh disposition
and bounded runtime result follow; the original failure is retained separately.

## GO501486: stock replay and unfocused capture qualified

`PASS` for this exact stock recorded case / candidate, not current-main controls,
clean UI layout, physical input or owner acceptance. Corrected source runner
`f744b0c`, SHA256 `315E1AEC74A2F6054AA555FA6BC134CB51405CD27A4526A9076B4A44E72D4F18`;
prepared manifest `.local/readback-04/prepared.json` SHA256
`CD883FC9062DA299D4BB53B2431A38C0DBEFDAB6D8323F97E35DA5E241610EE8`.
Candidate game/source identity above is unchanged.

The exact owned game PID26048, birth ticks639293018011846585
(2026-11-03T11:23:21.1846585Z), request
`a74d53fc034a47938e21320f776e1162`, SDL HWND38669466, verified3,600/3,600
input/WRAM checkpoints and exited0 normally. First observed status0 at
11:23:21.2917532Z; terminal `verified`3,600 at11:24:21.6047783Z.
Final status says `outputMuted=true`; the reviewed replay consumer excludes
controller acquisition, force and UDP delivery. No input keys/posts or activation
request were sent. This is synthetic tape-driven response, not a fresh wheel run.

R2 captured that same HWND/PID/birth twice while VS Code PID26220/HWND460684
remained foreground throughout. Both images are770x608 and have different hashes:

| Image | Status at request / visible replay overlay | SHA256 |
| --- | --- | --- |
| `live/capture-0.png` | 600 / 626, Blue Falcon selection/menu content | `9462A6FF911B0B20FA2F172E2EA95203651C026D054C046CB737C18FAE41BA2F` |
| `live/capture-1.png` | 1800 / 1822, craft/opponents on the track | `CAC23A6F827691442B52379B029A3011747E52434481FC05DD6AD77004990634` |

Both show REPLAY and FORCES OFF. These are fresh changing game-content captures,
not an exact scanout timestamp, demonstrated occlusion recovery or clean-layout
acceptance. Menu/HUD samples contain overlapping glyphs; that visual observation
remains unresolved rather than being classified as correct rendering. No display
or framing change was attempted to investigate it in this interval.

The game wrote `live/final.bmp` and131,072-byte `live/final.wram`. Graphical and
headless final WRAM SHA256 both
`5D41D90DB02E54D1FD1E222C8228C18232F57D5FD5A7F0CA38011377FE7DC7E8`.
Final game BMP SHA256
`5981061532FB3A215272D30B3BB296A35AE6C2BEAE82DE0AC8BEFDA4C7449ED7`.
The BMP is guest-renderer output; WGC proves window content separately. The
presentation log (180Hz target,10,403 presentations) is not a pacing benchmark.

Cleanup: LL physical0/injected0/startup-resets0; no abort,61 message-pump and1,980
monitor heartbeats, hooks released/thread joined, original foreground/raw tick
and all four display identities unchanged. Owned game/capture job empty.
Snapshot/presence verifier passed, and the separate numeric-UTC child passed all5
original files' SHA/ticks/attributes with zero errors. Lease
`62dc950172364b538f63a8a3a1598b06` was acquired around11:23:21.015Z and release
receipt written11:24:23.211Z (about62.2s total; about1.6s from final assertion to
release, within the approved80s+15s bounds). FINAL-release sent to remi, soba,
kole. No custody remains; no Steam actions or public push.

Raw evidence is `.local/readback-04/live/`: `result.json`, `status.json`,
`observations.json`, `process.json`, both capture identity receipts/PNGs,
`images.json`, `guard.json`, `owner-snapshot/manifest.json`, `owner-restore.json`,
`owner-independent.json`, `stderr.txt`, `lease.txt`, `release.txt`.
Next actionable work: a current-main private graphical candidate is needed for
cardinal controls/Apply/F6 qualification. Review the sampled menu/HUD overlap
against that candidate before claiming clean layout; physical and owner gates
stay separate. No further runtime run is queued by this document.
