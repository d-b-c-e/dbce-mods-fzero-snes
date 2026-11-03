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
pass. Final preparation/rehearsal: `.local/readback-03`. Send one GO request with
exact prepared-manifest/runner hashes. Live rendering/capture remains unverified
until execution; no lease is currently held.
