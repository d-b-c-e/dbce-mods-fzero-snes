# 2026-10-10: rig-profile controls installed and applied on the rig (STD-033)

## October 19 source successor: Native steering inversion

Private branch `samo/fzero-native-steering-invert-20261019`, based on canonical
`0758a9b`, closes the previously documented Native inverted-steering refusal.
The schema-1 planner now writes both `SteeringAxis` and `SteeringInvert = 0 | 1`
under the selected `[Controller.<SDL GUID>]`. Applying a later noninverted
profile explicitly clears the flag. The existing atomic apply/backup, qualified
device numbering, Revision gate and unrelated-key preservation remain in use.
An already-applied unchanged Revision is not automatically reapplied on upgrade;
the launcher control can explicitly change its direction without reapplying a
profile or overwriting subsequent local edits.

The existing Native raw-wheel reader inverts once before digital or proportional
SNES steering. A shared game-owned helper mirrors the normalized signed axis,
preserving exact zero and both full-lock endpoints despite SDL's asymmetric
`-32768..32767` range. It does not change the response curve, dead zone, pedals,
standard gamepad mapping, Shared Core normalization, force output or input ABI.
Supplemental-only reads still exclude steering; the existing overlay axis path
uses the same direction as ordinary input. Loading another wheel profile resets
the flag to off when it is absent.

The existing launcher wheel page adds **Invert steering**, using its current
Boolean/save route. Its live preview uses the same helper. Native wheel option
indices remain stable; Shared still suppresses all Native wheel options. There
is no second reader, physical-device acquisition or new force setting.

Focused offline checks passed in 8.39 seconds after Wheelkit deployment released
its hold: `fzero_controls` **225 checks**, including actual planner -> disposable
config -> production reader on an SDL virtual wheel, and `fzero_mods` PASS for
preview, strict Boolean edits, saved true/false and per-device reload. MSVC x64
Release compiled all affected fixture source objects plus their small video
dependencies; only the unchanged cached SDL3 static library was reused. No
compiler warnings were reported. No full graphical/headless product was built.

Receipt: portfolio hub `.local/fzero-steering-samo-20261019/check-04/result.json`,
SHA256 `61C2C08EC2F072902ADB38FEBCB04E44FF72E158147945A2371ABEF8F0700231`.
It pins source/header inputs, cached SDL3 library, compiler response files and
logs. Initial failures remain in check-01 through check-03: a stale fixture
write-count expectation; a test incorrectly treating the overlay as
supplemental-only; then the real new-code endpoint defect from naive negation.
The final normalized helper resolves that endpoint failure. These are fresh
source fixtures, not a repeat of accepted gameplay or physical checks.

No owner config/install/ROM/save, public branch or existing candidate changed.
Product packaging, rendered checkbox behavior and physical wheel direction
remain separate future qualification. This feature changes input direction
only; it does not qualify force polarity or feel. Historical evidence below is
preserved with its original scope.

### Product qualification and local integration, October 19

Kobe grant284740 built the exact `d0319b59a11a62d5a20bf1435916dd851b7071b0`
source with MSVC x64 Release/Ninja, one compiler job, `BUILD_TESTING=OFF` and the
existing private Deluxe payload, UI `6d9b715`, framework `59d2966` and helper
`89878cd`. Both graphical and headless targets linked. The two existing C4005
`HIBYTE` macro redefinition warnings remain; no compiler errors. The original
225-check controls and launcher fixture results were retained without reruns.

| Artifact | SHA256 |
|---|---|
| Graphical build / privately staged launcher (7,158,784 bytes) | `AFD7B72F19CED0BA13100F476AC0FAB9B2A4F42EF3F944B72CBADFD2B3256278` |
| Headless build (3,014,656 bytes) | `26DCA1DD53FADB3E2585BFB6FC25BE05E35B3725896F8DC7392A9BED640C5690` |
| Existing WheelFfb | `3F6CC51444879D01FEB900098B2DE94242594D093F523E3C34BD511D69C0D946` |
| Existing private shim | `BF6455E63696A93B37C19DABF09B1E0606F26B0534BCE828417B1DF602DD8A7A` |

The real `Install-StreamDeck.ps1` privately staged
`E:/fzero-research/native-steering-samo-20261019/candidate-01` with a new
**538-file** owned receipt, SHA256
`484DBB5B631C49B86B0E14081F50FAD12E1826DE0B39731234B3CF13193E22E2`.
It binds this source, renamed Deluxe consumer and existing helper. All receipt
rows match; linked source/feature/Deluxe markers were checked, unowned bootstrap
bytes were preserved, and the packaged helper's controls-only PlanOnly passed
without changing candidate membership or bytes. No game image was executed.
The baseline authentic receipt, existing candidates, ROM/save/tape assets,
owner installations/settings and both runtime pins remain unchanged. This is
a private directory, not a public archive or adoption of the experimental
Shared force dependency pair.

The original runner exited **1** despite all payload phases passing: its exact
owned MSVC PDB server remained alive, so it correctly retained the lease.
Original `product-01/result.json` SHA256
`D6E4D9FE23C8AADBB588C098914D39961673257130B024213DECB9550E6770B5`
remains unchanged. Recovery waited for that process's natural exit, with no
termination, rebuild or test rerun. `cleanup-supplement.json` SHA256
`73FA9A7A46D50E743C75CEC14976E2690912D2F798222952A4AE892929BE61A9`
then proved a safe complete census, all98 recorded identities quiescent and
exact lease release at `2026-10-19T17:28:56.2793842Z` (total610 seconds).
All1,151 pinned inputs were unchanged. Both receipts and original logs live in
portfolio hub `.local/fzero-product-samo-20261019/product-01`; read them together.

After release, grant284947 integrated `d0319b5` by fast-forward from clean local
main `0758a9b`. The exact nine-path change yields tree
`1ab02dd6f3e0969ebcd73fe94fab9748964c4cf8`. No public push or owner deployment.
This documentation successor does not relabel the binary's source stamp.
Next acceptance is the rendered Native checkbox/preview/save and wheel direction
on the qualified candidate under a fresh runtime scope. Physical force and
Shared binding/setup acceptance remain separate; accepted driving is not repeated.

Source `af2ffe6` (`src/fzero_controls.cpp`, `src/fzero_controls_sdl.c`, README "Rig-profile controls"). Built in a
separate tree (`build-claude-merge`, BS Deluxe embedded, SDL3 sources copied from `build-merge`). All 18 ctest suites
pass there; the new `fzero_controls` has 102 checks. In the first full run `fzero_gamepad` hung once (stopped after 10
minutes). It passed three reruns, and 18/18 passed on the next full run. That test leaves SDL's hardware joystick
drivers on, so the rig's real devices are enumerated during it. `fzero_controls` turns them off.

## Install

`tools/Install-StreamDeck.ps1 -BuildDir build-claude-merge -Label profile-controls`: launcher `79667FC2...` from
`af2ffe6`, `WheelFfb.dll` unchanged (`3F6CC514...`). Backup `deployment-backups\2026-10-10-profile-controls`. Only
documentation changed between the previous install (`4ac6f6b`) and `af2ffe6`'s parent.

## The owner's profile

The owner's active Wheelkit profile ("Moza KS Pro Triple", the same section V-Rally 4 received on 2026-10-09) was added
to `config.ini` as `[Controls]` by a test helper. That is helper provisioning, not Wheelkit's production Apply. Backup:
`E:\Source\_archive\2026-10-10\fzero-owner-controls-provisioning-005018` (`config.ini.before`, `.provisioned`, the
section).

## Launch check, 00:50-00:51 CT

Direct start with the ROM (no launcher), no input, force feedback off for the run. The game's log:

```
[fzero-controls] profile 'Moza KS Pro Triple' revision owner-provisioned-2026-10-09 applied to 0300102d6e3400000600000000000000 (MOZA R12 Base): 13 keys
[fzero-input] ignoring raw Gudsen MOZA Multi-function Stalk guid=030063096e3400002400000000000000 (wanted 0300102d6e3400000600000000000000)
[fzero-input] raw Gudsen MOZA R12 Base guid=0300102d6e3400000600000000000000 steering-axis=0 gas-axis=2 brake-axis=5
```

- It reached the attract demo ("PUSH START") and closed normally.
- `config.ini` changed only where intended: in `[Controller.0300102d...]`, `ButtonStart` 36 -> 35 and `ButtonSelect`
  23 -> 22; a new `[ControlsApplied]` (Revision, Device); `config.ini.before-profile-controls` created.
- Every other key already matched the profile (steering 0, accelerator 2, brake 5, A 31, B 18, hat 128-131), and the
  owner's own X/Y/L/R, save-state, rewind, dead zone and pedal threshold stay.
- The `[ForceFeedback] Enabled` line went back to the owner's `1`. The other 97 owner files were byte-exact; one new
  diagnostics file was archived and removed.
- Not applied (logged): clutch, handbrake, shifts, reverse, gears, camera, look back, reset, horn. F-Zero has no such
  controls.
- Evidence: `E:\Source\_archive\2026-10-10\fzero-controls-apply-005025` (stderr, frames t10-t40, `config.ini.after`).

## Still open

- **Start and Select now follow the profile** (35 and 22) instead of the owner's earlier launcher capture (36 and 23).
  If either does nothing at the wheel, check those buttons in the Wheelkit profile. Every game follows the profile.
  Undo: copy `config.ini.before-profile-controls` back, or rebind on the launcher's wheel page (it stands until the
  profile's revision changes).
- Observed input on the real wheel: F-Zero reads the wheel through SDL inside the process, where the toolkit's
  DirectInput injection does not reach. So "observed" rests on the virtual-wheel ctest, and owner acceptance is the
  next step.

## 00:54: reinstall with Astra's review fixes (95d34e4)

Two attached devices with the profile's vendor/product are refused, and only qualified devices (so far the R12)
carry DirectInput indexes over to SDL (`fzero_controls`: 108 checks; 18/18 suites). Installed with label
`profile-controls-refusals` (backup `deployment-backups\2026-10-10-profile-controls-refusals`). Launch check 00:54:
the revision was already recorded, so nothing was rewritten (`config.ini` byte-identical across the run). The game
opened the R12 with steering 0, accelerator 2, brake 5, and the other 98 owner files were byte-exact. Evidence:
`E:\Source\_archive\2026-10-10\fzero-controls-refusals-005400`.

## 01:07: the reviewed transaction build (06ef443)

Astra reviewed 3cfaa00 (3487): both blockers from their 3459 are closed. Button pedals are refused, and the edit is
one read-back document moved over `config.ini` in one step, with fault-injection tests. Remaining limits as stated
there: R12 correlation only, the owner's threshold and rest behaviour kept, and a revision gate rather than
continuous readback. 06ef443 adds the toolkit 65c686d header (one-line bindings). 18/18 suites. Installed as
`controls-transaction` (backup `deployment-backups\2026-10-10-controls-transaction`). Launch check 01:07: nothing
pending, `config.ini` unchanged, the R12 opened with steering 0 / accelerator 2 / brake 5, the other 98 owner files
byte-exact. Evidence: `E:\Source\_archive\2026-10-10\fzero-controls-transaction-010755`.

## 02:01: Astra's capability receipt installed (e82a669)

Clean reconfigure and Deluxe rebuild in `build-claude-merge` (source stamp `v1.8.3-178-ge82a6692f127` in the header and
the image), 18/18 suites. `Install-StreamDeck.ps1 -Label controls-capability`. The receipt now declares
`controlsProfileSchema 1`, `controlsProfileAdapter fzero-raw-wheel-1`, the launcher (`4DA28F9D...`) and native hashes,
and source `e82a669`. Launch check 02:01: nothing pending, the R12 opened with steering 0 / accelerator 2 / brake 5,
and the other 98 owner files were byte-exact. Evidence: `E:\Source\_archive\2026-10-10\fzero-controls-capability-020158`.

## Test injection and the closed loop (2026-10-10, 8930d45)

F-Zero reads the wheel through SDL, so the DirectInput proxies' fence cannot inject into it. `src/fzero_inject.cpp`
substitutes samples in our own reader (`gamepad_read`), before the applied `[Controller.<guid>]` keys turn them into
SNES bits. It uses the toolkit's grammar and injection table. It arms only when all three hold, and is a no-op
otherwise:

- `%LOCALAPPDATA%\dbce\fzero\inject.on` exists;
- `config.ini [ForceFeedback] Enabled = 0`, so no force runs in the process;
- the `[Controls]` profile is applied to the opened wheel.

Commands come from `inject.txt`, raw or action (resolved through the applied `[Controls]`). `dev=` must be the profile's
wheel. `fzero_controls` ctest checks it through the real reader on an SDL virtual wheel (166 checks).

First run, 04:03-04:06, on the owner's applied profile with FFB off:
- the wheel's start, hat and confirm drove the menus to a Practice race;
- throttle, steer ± and brake were observed in the game's input word, with speed 72 -> 378 km/h and 231 -> 10 km/h,
  in F-Zero's Forza packet (5 of 7; start is not in the packet);
- owner files were restored byte-exact.

Evidence: `E:\Source\_archive\2026-10-10\fzero-inject-040316\RESULT.md`.

What that run shows, and what it does not (Astra's review, 2026-10-10 04:09). It shows the game reading the applied
`[Controller.<guid>]` keys, driven through action commands. It is not a Wheelkit Apply closed loop:
- the profile was applied earlier (owner-provisioned at 00:50), not by production Wheelkit Apply in the run;
- `inject action` resolves through the same applied `[Controls]` store the game reads, so it cannot catch a wrong
  translation.
The next run applies with Wheelkit prepare-live/apply-live. It sends raw samples generated independently from the
original saved profile (Wheelkit `raw-workload`).

### Review fixes (after 8930d45)

- **No force for the whole process.** `inject.on` present at start latches "no force" before any other check
  (`src/fzero_output_latch.c`). It holds even when the session then refuses to arm. `FzeroFfbInit` refuses after that
  latch, even if `config.ini` says `Enabled = 1` by then (the launcher writes it between the two). The latch works
  the other way too: once force output has started, a test request is refused.
  - `fzero_ffb_injection_latch` covers the 0 -> 1 order, with a fake WheelFfb: nothing loads.
  - `fzero_ffb_force_first` covers the reverse order.
  - Each is a separate ctest process. A mutant without the `FzeroFfbInit` check aborts.
- **A bounded, one-session command file.**
  - `inject.on` must name the session: `nonce=` (8-64 letters/digits) and `expires=` (unix seconds, at most an hour
    ahead).
  - The first line of `inject.txt` must be `nonce=<that nonce>`. A file over 4096 bytes is not read. Only the first
    32 commands of a file are read, and a session accepts at most 2000.
  - Each version of the file is read once.
  - At expiry the running samples are dropped and injection is off.
- **No stale samples across a reconnect.** Every close of the raw wheel drops its running samples: detach, refresh
  and shutdown alike. So a reopened stick with the same SDL GUID starts clean.
- `fzero_controls` ctest: 187 checks, covering the file caps, the nonce, reopening, expiry and the `inject.on` grammar.
