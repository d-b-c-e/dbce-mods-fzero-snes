# F-Zero selected-device/cardinal consumer contract, October 26

Base: retained integration b61e413 (32 commits beyond main a03c722).
Isolated branch: egon/fzero-cardinal-contract-20261026.
Shared reference: Wheelkit fd926fe90adad0f36217e5134fe2cfe5f37ae300.
That tip adds Gravel fixtures; its inherited FZeroControlProfile requires matching
instance and product identities, cardinal hats and one qualified R12 controller.
Correction (receipt reconciliation below): this retained writer supports receipt-gated Native Schema 2. Retained game Schema 2 support does not establish
helper/writer/package compatibility or promote a deployed build.

The existing device-free contract target directly includes production
fzero_controls.cpp. Added cases cover all eight common digital actions, all four
supported hats and cardinal directions, diagonal refusal, foreign instance and
conflicting product refusal, and unsupported clutch/neutral/gear/shift/restart
requests under both schemas. Restart is not a dedicated mapped game action;
existing start/select and save-state/rewind contracts remain distinct.

Initial run: 207 vectors, 913 checks, 16 failures. All failures were conflicting
product identities with the same device instance. Production compared only dev;
now it compares dev and prod, for digital and pedal mappings alike. Schema 1
reports/skips the mismatched action; Schema 2 refuses the transaction. This matches
the existing writer contract. No shared source or vendored header changed.

Final focused command (MinGW g++ available on PATH):

    g++ -std=c++17 -O2 -Isrc tests/test_controls_contract.cpp -o controls-contract.exe
    ./controls-contract.exe src/vendor/controls/controls-vectors.txt

Result: 207 vectors, 913 checks, 0 failures. No graphical build, package, install,
game launch, input injection, physical device, live save or rig access occurred.
Existing source-qualified menu/analog gates and Native host actions are inherited,
not newly runtime-qualified. No recorded-drive regression is warranted for this
planner identity fix; no valid binding behavior changed.

Standards ledger comparison: all 37 IDs remain represented. STD-019 unchecked,
STD-021/031 pending, STD-037 partial remain unchanged. Next useful milestone:
review this exact source, then reconcile the retained Schema-2 producer/helper
receipt with the selected-profile production Apply route before any new package.
The October 17 owner install, 377A package and held branches remain untouched.


## Refusal wording follow-up to DBC-316

Sandy DBC-315 passed the bounded identity correction. Darlene DBC-316 found
that same-instance/product-conflict refusal incorrectly said another device.
For that case, both schemas now say `<action>: wheel product mismatch; rebind
to selected wheel`. The reason is shared by pedal and digital branches, fits
Schema-1's 64-byte note even for saveStateMenu, and preserves existing
foreign-instance refusal wording and transaction behavior.

Exact-message and unmapped-key checks cover all eight common actions, throttle,
and saveStateMenu under both schemas. The final focused command above passes
207 vectors / 953 checks / zero failures. git diff --check passes.
No package, installed files, game runtime, input or physical acceptance changed.
The selected-profile Apply / retained Schema-2 helper receipt reconciliation
remains the next implementation milestone after this wording review.

Correction to the earlier ledger comparison: the current shared ledger now has
40 IDs. STD-038/039/040 were missing from this retained branch's adoption table;
they are recorded below in TOOLKIT-ADOPTION.md without claiming deployment.

## Retained producer/helper reconciliation after DBC-317

DBC-317 passed 6071572 wording; DBC-315 passed the underlying identity fix.
Source inspection on October 26 distinguishes these Wheelkit candidates:

| Source | Native producer contract |
| --- | --- |
| Main bee3dd7c35b2bb4b1e5516fd7c5b6f388caf63ab | Writes Schema 1, refuses existing Schema 2 |
| Retained fd926fe90adad0f36217e5134fe2cfe5f37ae300 | Writes receipt-selected Schema 1 or 2 |
| Package helper d0f333c60892dc44beb36e277b19bd6a650507b7 | Writes receipt-selected Schema 1 or 2 |

The earlier statement that fd926fe only emits Schema 1 was incorrect. Inspected
both retained FZeroControlProfile implementations directly with git show, rather
than substituting the current main checkout. Neither shared worktree was edited.

Game tools/Install-StreamDeck.ps1 declares nativeControls with schema 2,
adapter fzero-raw-wheel-2 and consumer FZeroSNESRecomp-wheel-launcher.exe.
The retained writers recognize that scoped object while retaining the top-level
Schema-1 compatibility declaration. They require the complete optional-action
set and verify launcher and WheelFfb hashes before selecting Schema 2. Existing
Schema-2 requests are editable only with that verified capability. For axes,
they require the qualified 0..65535 range; supported-action gaps refuse the
Schema-2 plan. Single R12 instance/product and cardinal hats remain required.

Game tools/package_unified.py pins NATIVE2_HELPER_SOURCE to d0f333c, validates
that exact helper source and the scoped native capability; stage_unified.py
records the same pin. This source inspection establishes agreement of the
retained producer, helper pin and game receipt shape. It does not establish the
bytes of a newly built helper/package or a selected-profile Apply execution.
RigPresetConfigurator uses FZeroControlProfile.For for F-Zero preview targets
and exposes ApplyAsync; that is the production route for the next focused check.

Next bounded milestone: use an isolated retained Wheelkit candidate fd926fe and
synthetic disposable install to exercise selected-profile preview/Apply/restore
with the scoped Schema-2 receipt, then feed its emitted Controls request to the
6071572 production consumer contract. Include calibrated pedals/steering,
cardinal navigation, host actions, identity conflict and receipt drift. Reuse
existing FZeroControlProfileTests and Native profile fixtures. Preserve main's
uncommitted AGENTS.md and all held branches. No shared source defect is claimed
from main lacking the retained feature, and no shared change is made here.

Ledger remains 40 represented IDs; STD-019 unchecked, STD-021/031 pending,
and partial adoption/physical/UI gates remain. Documentation-only reconciliation:
git diff --check is the applicable check; earlier 953 consumer checks remain
recorded results, not rerun results. No push, merge, package replacement,
installation, launch/input, live-save access or physical acceptance occurred.

## DBC-322 independent direction and neutral qualification

Disposition: no additional product defect found. f28453a retains 83b678b's
instance-and-product correction and 6071572's reviewed refusal text. Current
primary game source a03c722 does not contain that retained correction or the
retained Schema-2 consumer. It is not equivalent to this candidate. Wheelkit
primary bee3dd7 and retained fd926fe both require matching instance/product and
cardinal hats; only the retained producer has the scoped Native2 receipt route
described above. No source branch was merged or promoted.

Trace: FZeroControlProfile.For validates the selected qualified R12 identity,
cardinal angle and receipt-selected schema. FzeroControlsPlanLines parses the
request and digitalIndex emits 128 + hat*4 + direction (up/right/down/left).
The owned fzero_gamepad.c loop decodes this through recomp_raw_hat_index/value
and tests SDL hat bit membership. The pinned recomp-ui source
6d9b71526f82c0182f85ec47610eb559303a77b8 decodes those directions to 1/2/4/8.
Center (SDL value 0) activates none. A diagonal held state activates each of its
two cardinal bindings; this differs from a diagonal *binding request*, which
is refused. Explicit unbind writes -1 and cannot retain a hat direction.
Neutral here describes hat release, not the unsupported transmission action.

The existing device-free fixture now consumes that real launcher header and
checks emitted bindings for both schemas, four admitted hats, every cardinal,
all 16 bit states, explicit clears and decoder bounds. It performs the same
bit-membership operation as the game loop; it does not execute the SDL loop,
enumerate/open even virtual devices, or establish physical direction/index
equivalence. Existing identity/refusal assertions remain in the same target.
No planner, reader, shared toolkit, vendor header or user-facing text changed.

Focused results: GCC 16.2, 207 vectors / 1,534 checks / zero failures. Fresh
temporary CMake/Ninja build with FZERO_BUILD_GAME=OFF and RECOMP_UI_ROOT pointing
to the clean exact pinned checkout above: fzero_controls_contract passes 1/1.
The fixture's INI roundtrip uses its own temporary directory and removes it.
git diff --check passes. Reproduce:

    cmake -S . -B <scratch> -G Ninja -DFZERO_BUILD_GAME=OFF -DRECOMP_UI_ROOT=<pinned-recomp-ui>
    cmake --build <scratch> --target fzero_controls_contract_tests
    ctest --test-dir <scratch> -R '^fzero_controls_contract$' --output-on-failure

The header include path is now available to this target even with game builds
disabled. Initialize the pinned submodule or supply its exact checkout; there
is no copied/fake decoder fallback.

Independent next outcome: review/integrate this source-only consumer coverage
with the retained identity fix. Selected-profile preview/Apply/restore remains
the separate existing DBC-65 recovery-gated outcome, not executed or replaced
here. No live config/save access, package/install, launch/input, physical claim,
public push, recovery retry or support expansion occurred. Darlene's conditional
review gate is not triggered because no player-facing text changed. Standards
comparison remains 40 represented IDs with the earlier incomplete states intact.
