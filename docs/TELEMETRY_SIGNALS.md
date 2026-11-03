# F-Zero telemetry signal investigation

The desktop build currently sends a 324-byte Forza Horizon packet to the
configured UDP destination. It contains race state, approximate world-position
speed and distance, yaw, vehicle ordinal, energy as fuel, and digital driving
inputs. It does not claim suspension, tire, surface, collision, or actual
engine data. `currentEngineRpm` is a speed-derived dashboard approximation.
A one-frame move of more than 32 units (a save-state load or rewind; racing is
about 7) holds speed and distance instead of reading as thousands of km/h.

## Verified crash signal

The energy-loss detector used for FFB originally found four impact edges in
the September 28 recorded drive, at frames 3417, 5526, 6139, and 6169
(losses of 86, 40, 240, and 96 units). A closer replay trace found three
isolated 24-unit losses at 3068, 3502, and 10222 with no boost input; the old
32-unit threshold missed them. The threshold is now 16 units, with the same
eight-frame cooldown and boost-input exclusion. A full device-free replay
confirmed all seven pulses and no replay divergence. Repeated five-unit drains
remain below threshold. These three lighter events are plausible contacts, not
visually classified wall hits; physical impact direction and delivered wheel
torque are not encoded in the recording. The current FFB pulse uses a fixed
magnitude for all seven, so severity scaling is a separate experiment.

For richer telemetry, an explicit event record containing frame, energy loss,
and a source/confidence tag is the least ambiguous route. The standard Forza
packet has acceleration and surface-rumble fields but no explicit crash field;
mapping energy loss to acceleration without measured deceleration would invent
physics. A later SimHub-facing adapter could consume event records alongside
the existing Forza dashboard stream. If acceleration is used instead, derive
it from world-position velocity and verify its direction and timing first.

## Surface/slowdown patch is not identified yet

Opt-in headless `FZERO_SURFACE_TRACE=1` logs per-frame player world position,
`$00C7`, energy and input during a verified replay, then the raw bytes `d40`
(player slot of `$0D40`), `c0`/`c1`/`c2` (`$00C0`-`$00C2`), `f53` (`$0F53`)
and `e90` (the 15 lap-record bytes `$0E90`-`$0E9E`, as hex) for the lap
question below. It emits racing frames only; add `FZERO_SCENE_TRACE=1` for the
state bytes on every frame. In the September 28 case,
`$00C7` is zero for 9,810 of 9,847 race frames and `0x10` for frames 3154–3190.
Mean wrapped world movement was 6.59 units/frame in 3100–3153, 6.93 while
`$00C7=0x10`, and 7.33 in 3191–3240. Energy increased by eight units at the
end of that interval. This is **not evidence of a slowdown surface**. The
existing FFB model's `ram[0xc7] != 0` “rough” boost is therefore provisional;
do not expose it as a named ground-texture telemetry channel yet.

The next useful recording deliberately crosses a known slowdown patch, with
course and approximate time noted. Compare world-position speed, energy,
`$00C7`, nearby player-state bytes and the Mode 7 course tile under the craft
before, during and after contact. Only after repeated crossings agree should
we populate Forza's `surfaceRumble[4]`/`onRumble[4]` or a dedicated surface
event. The tile map may distinguish a patch even if `$00C7` does not.

The raw trace is diagnostic and private: it includes position and controller
input, not the ROM. Do not commit a recorded trace, ROM or save state.

## October 23 failed initialization cleanup candidate

Failed address resolution or socket creation now uses the existing shutdown
path to release the telemetry transport and its Winsock reference. Previously,
failed initialization could leave `s_winsock_started` set without a socket,
preventing the diagnostic no-delivery latch from being armed. Shutdown preserves
that process latch; a later Init cannot reopen delivery after it is armed.

The existing `fzero_telemetry` fixture gains failed-init, latch and no-reopen
checks after its normal loopback/idle-packet checks. Fresh MSVC x64 Release
compilation and the complete existing console fixture passed on exact source
`a52662ecc6d4e1d816b389c0162e9cecdc2531c1`, with no compiler warnings. The invalid
address is refused, the mute latches, and a later valid Init/Frame cannot deliver
another packet to the loopback receiver. No game, screen, device, owner install
or fresh product build has run for this candidate.
The package route and frozen stock artifact remain separate. STD-019 stays
unchecked and STD-021/031 physical normalization stays pending.

Evidence: ignored `build-telemetry-check/run-01/result.json`, SHA256
`A4476AC2D6267EC410AA5093829FA5ECF28CA928490712BD626E96DB6BC50D41`,
plus `init-compile` / `init-fixture` stdout/stderr logs. The same lease batch
independently compiled and ran the separate rank candidate `6b20d46`; these
results qualify the separate source heads. All 15 retained process identities were absent,
the complete census was safe, and exact lease `397837a4` released at
`2026-10-23T06:17:14.1093087Z`. Reha received the next-slot handoff immediately.

## October 23 rank source candidate; lap number still unknown

The packet candidate maps WRAM `$0DC8` directly to `racePosition` only during
the existing racing state and for ranks 1 through 20. Invalid values, menus and
snapshots ending before that byte retain zero. The prior minimum snapshot size
and shutdown idle packet remain valid. No input, force or transport path changes.

The retained stock generated function `$00:A8B1` reads `$0DC8`, divides it by ten
and publishes the rank digits in OBJ slots 48 through 51. `$00:E8E4` updates that
same value. The imported Deluxe delta has no edits over the complete rank HUD
routine `[A8B1,A97B)` or updater `[E8E4,E9B0)`. Its SHA256 is
`a9e31fd3a32103cc6591e70c31e4a182b631989b7ab31b21cb5aec6c5c1bc02b`.
This establishes the source mapping; it is not a new Deluxe runtime check.

Read-only inspection of 77 retained `triple-vehicles` / `triple-vehicles2`
capture pairs matched WRAM rank to published HUD digits in all 71 racing frames
(ranks 2 through 6). Six non-racing frames are excluded from that comparison.
The first check incorrectly required every capture to be racing and refused;
that failure is retained. Evidence and the corrected check are in ignored
`build-rank-source-check/retained-rank-proof.json`, `retained-rank.log`,
`initial-failure.txt` and `check-retained-rank.py`. No capture was regenerated.

`$0D40` is a completed-lap counter: initial car state writes `$FF`, the crossing
path `$00:E233` increments it, and `$00:99C0` finishes at five. The inspected
captures contain only `$FF` and zero, without later lap transitions. Deluxe
changes portions of the surrounding lap/finish paths, and this investigation
does not establish the receiver's current-lap numbering convention. Therefore
`lapNumber` remains zero; no live lap time is derived from this counter. Existing
`currentLap` / `currentRaceTime` timer behavior is unchanged. Closing this gate
needs attributable start/finish lap transitions in both modes and the packet
consumer's numbering contract, not a new guess from HUD text.

The existing telemetry fixture adds rank bounds, short-buffer, idle and unchanged
lap-byte checks. Fresh MSVC x64 Release compilation and the complete console
fixture passed on exact source `6b20d468d26422682a6482da2ba5d7733a1479f7`, with
no compiler warnings. These results qualify that separate source head, before
the combined private candidate described below.

Evidence: `build-telemetry-check/run-01/result.json` in the init-cleanup worktree,
SHA256 `A4476AC2D6267EC410AA5093829FA5ECF28CA928490712BD626E96DB6BC50D41`,
and `rank-compile` / `rank-fixture` stdout/stderr logs. Both independent telemetry
candidates passed under one short lease; all 15 retained process identities were
absent and the complete census was safe. Exact lease `397837a4` released at
`2026-10-23T06:17:14.1093087Z`, followed by the direct Reha handoff. No game,
screen, device, owner install or product rebuild ran. Lap numbering remains
unknown. STD-019 stays unchecked; STD-021/031 physical normalization stays pending.

## October 23 lap fields: consumer contract pinned, lap transitions still unobserved

The installed SimHub Forza decoder fixes what the lap fields mean on the
receiving side. `GSIReader.dll` (SHA256
`39046E0CFAE85D837A2307507EC3BDB8C62563EAA5EFC1AEAB8A6CE821D3AB79`), class
`GSIReader.ForzaMotorsport7.ForzaManager<T>`, read with ilspycmd:

- `CompletedLaps` is the dash `lapNumber`, so the field counts completed laps.
- `CurrentLapTime` is the dash `currentLap`; `LastLapTime` and `BestLapTime` are
  `lastLap` and `bestLap`. `currentRaceTime` is not read at all.
- `Position` is `racePosition`; gear byte 0 shows R, 11 shows N, others as is.

So the packet's `currentLap`, which carries the `$00C0`-`$00C2` timer, is what
SimHub shows as the current lap time. If that timer is the race total (its
per-lap behaviour is not established here), the value is right only on the
first lap. `lapNumber` stays 0, which SimHub shows as no completed laps.

No retained evidence contains a lap transition. The 77 WRAM snapshots end
12.37 s into the first lap (18.55 s was the clock read as binary; it is BCD,
see below) with the player's `$0D40` at `$FF` before the start
and 0 after it. Playthrough cases store input and WRAM hashes, not WRAM bytes.
The October 8 session evidence holds force traces only, and the September 28
full-race case was not found (see PLAYTHROUGH_RECORDING.md). The `$FF` to 0
step at the start is the only observed transition. Increments at the line,
finish at five, restart reset and Deluxe behaviour remain static claims.

Closing this needs a full race, in stock and Deluxe, through at least two
crossings and the finish, with `$0D40` (player index 0) and `$00C0`-`$00C2`
logged per frame. The surface trace now carries both. The input is one
complete-race playthrough case per mode: an attended `FZERO_RECORD_PLAYTHROUGH`
drive (see PLAYTHROUGH_RECORDING.md) from the start line through the finish,
with no save-state, rewind or reset (those invalidate the case). Keep the
`.fzpt`/`.fzpt.state` pair, the cartridge mode, MSU and center viewport. The
existing cases (2,351 frames Deluxe Practice, 3,600 frames stock synthetic) are
too short for a finish; whether the stock one reaches a crossing is untested,
and replaying it with this trace would show that. Replay it with
`FZERO_REPLAY_PLAYTHROUGH`, `FZERO_SURFACE_TRACE=1` and `FZERO_SCENE_TRACE=1`
on the headless runner; every frame's WRAM hash must match. Keep stderr with
the case hashes. Only then map `lapNumber` from `$0D40` (0 to 5, `$FF`
as 0), `currentLap` as the timer since the last increment, `lastLap` and
`bestLap` from completed laps, and leave `currentRaceTime` on the timer. Until
then the packet is unchanged.

## October 23 lap fields from the game's own lap records (source candidate)

A static trace of both ROMs (stock `fzero.sfc`, BS Deluxe v1.1 `552159A1…`)
covers every instruction that touches `$0D40`, `$0F53`, the lap records
`$0E90`-`$0E9E` or the race clock `$00C0`-`$00C2`. The ROM-derived listings
stay untracked.

- **The race clock is BCD.** `$00:8ECE` ticks it in decimal mode: `$C2`
  hundredths, `$C1` seconds to `$59`, `$C0` minutes, frozen at 9'59"99. The
  HUD (`$00:A880`) draws their nibbles. Only race start and this tick write
  it, so it is the race total. The recompiled tick emulates decimal ADC. In
  all 71 racing frames of the 77 retained snapshots the bytes are BCD, and the
  HUD digits are the RAM time minus one tick (0.01 s). The packet read the
  bytes as binary, which is wrong above 9 s (10.12 s read as 16.18 s).
- **Race start** sets the player's `$0D40` and `$0F80` to `$FF`, every lap
  record to `$FF`, `$0F53` to 0 and the clock to 0.
- **The line.** `$00:9968` runs for each car (X = 0 is the player). A
  forward crossing increments `$0D40,X`; a backward one decrements it (the
  player's only while `$0D40` equals `$0F53`).
- **A timed lap.** `$00:99C0` runs for the player only, while `$C3` is 0.
  When `$0D40` = L ≥ 1 and record L is still `$FF`, it stores the clock at
  `$0E90 + 3(L - 1)` and sets `$0F53` = L. L = 5 is the finish (`$C3` = `$20`
  stops the clock). `$00:9A99`, run for each timed lap before the finish,
  branches on `$0D40` = 4 (the start of the last lap).
- **Not the player.** `$00:DED0`'s `$0D40,X` changes (`$E206`, `$E233`,
  `$E24C`, `$E265`) are dummy cars placed relative to another slot, not the
  player's crossing.
- **Deluxe.** All of these instructions are byte-identical in Deluxe. Its
  hooks nearby write none of these bytes:
  - `$00:DED2` → `$1E:B2D6` skips the dummy-car placer while `$7F:4CA0` bit 7
    is set (its slot-2 car mode);
  - `$00:915D` / `$00:919E` → `$1E:B41E` / `$1E:B46A` add the slot-2 car and
    Deluxe's own `$7F:4CA0` flags around the stock per-car calls;
  - `$00:9A18` → `$1E:BA42` runs after the finish.

  A byte-pattern search of the changed bank 00-03 bytes and the Deluxe code
  banks `$1E`-`$1F` finds no direct or long store to these bytes. Indirect
  stores are not covered.

So `$0F53` is the authoritative per-frame completed-lap count: it counts the
laps the game has timed, and it never falls within a race. `$0D40` can sit one
lower after the player reverses over the line. The packet now maps:

- `lapNumber` = `$0F53` (0-5);
- `lastLap` and `bestLap` from the differences of the recorded clock times;
- `currentLap` = the clock minus the last record (lap 1: the clock);
- `currentRaceTime` = the clock.

Everything is derived from WRAM each frame, so a save-state load or rewind
carries over. A count above 5, a record that is not a BCD time, is unrecorded,
runs backwards or is later than the clock, a non-BCD clock, a short snapshot or
a menu frame leaves `lapNumber`, `lastLap` and `bestLap` at zero, with
`currentLap` on the clock. The telemetry fixture covers each of these, the
finish, and a reversal. It has not been compiled or run yet.

What stays unobserved: no retained capture completes a lap. A full-race replay
in each mode, as described above, would show `$0F53` rise at each crossing, the
recorded times and the finish at 5. The surface trace line now ends with `f53`
and the `e90` record bytes, so the replay shows them directly: at each step of
`f53` to L, the record bytes for lap L should equal that frame's `c0`-`c2`.

## October 23 combined private Deluxe candidate

The isolated `lamo/fzero-deluxe-telemetry-integration-20261023` lane combines
qualified init-cleanup head `771b608` (runtime `a52662e`, including private
package route `6cfaa61`) with rank head `5d33cce` (runtime `6b20d46`). Both
production changes and all assertions from both telemetry fixtures are retained;
only this handoff needed a merge resolution. Lap number remains zero. No
controls, force, helper capability, runtime pin or default setting changes.

The separate passing checks above remain valid evidence for their exact heads.
The combined telemetry fixture is queued with one fresh private Deluxe build.
Use the existing helper publication, CMake targets, scratch installer and explicit
private stage/package/verify route in [UNIFIED-PRODUCT.md](UNIFIED-PRODUCT.md#october-23-private-deluxe-native2-packaging-source).
Keep the Native2 helper at `bdb6e09`, both private generated/data inputs and clean
source/dependency pins. Use a new empty scratch target with explicit `-Target`;
no owner install or game launch is part of this batch. Exact predecessor EXIT
and atomic lease admission are required after the current short slots and Nepo
FH1 build. The frozen stock package and public origin remain untouched.
