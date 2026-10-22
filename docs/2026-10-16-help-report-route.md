# Shared report-only Help source implementation

Request182337 implements the smallest route from design45cb1518 on isolated successors. Frozen game483/UI53, helperb887 and current queued package808ae1f5 stay intact. This is source with authored fixtures, not a built/published/rendered report route.

The existing Support/Diagnostics Help renderer keeps the two small repository hyperlinks and offers Submit GitHub Issue only through optional selected-feature callbacks. FZero gates that action on a separately report-qualified existing helper bundle, normal non-isolated/non-replay launcher context and exact native manifest/apphost identities. Feature package version1 is not used as the mod version. The native caller passes current kBuildVersion, FZERO_SOURCE_REVISION and actual resolved video config path. No input/controller/force ownership or Diagnostics setting changes.

fzero_setup.cpp factors its existing exact CreateProcessW/hash/no-link/individually quoted argv path for a report verb. Controls setup still closes/waits with exact PID+FILETIME as before. Report dispatch leaves the launcher open and opens only the existing shared report window in a separate helper process; it does not construct controls setup. The helper validates the fixed FZero command, supplies active build/source and file-presence inventory, suggests optional existing performance JSONL logs unchecked by default, and uses ProblemReportWindow/VM -> SupportReport for title/description/redaction/review/browser fallback. No config/ROM/save/receipt contents are auto-included. Presence is explicitly not install/runtime verification. Current normal launcher uses executable-root keybinds; isolated sessions expose no report action. Cataloglog/version gaps are not filled with guesses or receipt-schema relaxation.

The helper publishes additive schema1 report contract fzero-reviewed-browser-report-1, games=[f-zero], browserDraftOnly=true. setup_bundle.py admits old controls-only bundles with report marker0; exact new capability emits marker1; malformed capability refuses packaging. Native runtime still rechecks manifest/apphost pins. Old helpers and Linux have no report action. No direct HTTP submission, automatic browser launch, new uploader, report builder/window or --support-file ZIP route is added.

Authored existing provider/native-fake-spawn/package fixtures cover selected Support feature, no toggles, unavailable host/reset, exact report argv/active version/video path, bad metadata and changed helper. Separate helper Core/GUI fixtures cover strict dispatch without setup stores, optional inventory/logs, shared preview/redaction/invalidation and draft-only mode. No tests or compilation run yet; Python/PowerShell AST, XML parsing and scoped whitespace checks only. Matching helper publish, affected host/UI build and later visual review need scheduled shared slots. No existing accepted gameplay/force baseline repeat.

Source review182882/182913 corrections: CMake no-bundle header now explicitly
sets report-qualified0; a focused Windows no-bundle target compiles the actual
generated header and checks controls/report unavailable without launching a child.
Explicit report:null now refuses with a package fixture; missing report remains
legacy controls-only. Original SDL UTF-8 bytes restored outside the exact intended
report additions (byte-for-byte subtraction checked). All new fixtures remain
unrun; no heavy build or current frozen packet change.

## Source integration (request190751)

An isolated successor combines this report source with the exact existing
mouse diagnostic from game ea7a674 / UI c36722cd. The combined UI is 6d9b715;
shared helper 4eac3037 is unchanged. Both UI changes and the host additions were
composed without changing report dispatch, controller suppression, Configure
behavior, force ownership, bundle admission or trace opt-in semantics.

This closes source composition only. The previously qualified A140BCF6 build
and original failed runtime05-07 evidence remain at their original paths.
The combined source has no matching binary/helper publication or live result;
those are later packaging/qualification work, not a claimed runtime cure.
Shared force ownership, dedicated host save/rewind and schema-2 calibration
remain separate deferred scope. Do not repeat the activation test unchanged.

## Private affected build complete (request190853)

Built game **19afaf25** with paired UI **6d9b715** and report helper
**32bfd0bd**. The helper adds only the missing System.IO import after the
first GUI publish exposed CS0103; the 16 passing Core tests were reused.
Private helper publish, production report-window smoke and seven existing
bundle tests passed. Five affected CTest checks passed: fzero_setup,
fzero_mods, Configure result, launcher model cache and actual no-bundle setup.
The real 528-file helper manifest is admitted with report-qualified1.

Launcher SHA256: 2C78019E59B1D11802C3AFC4454B6A0ACF62E8657F8E220369783365745CDB8C
(7,136,768 bytes). Helper ZIP SHA256:
B8D38B1244F030EDEB8E098D5B875942DDDD5FA90595ECE1EDC1904CE2241F60.
Original logs, the first publish failure and precompile command-quoting failure
are retained in hub artifacts/fzero-report-trace-build-dine-20261016;
native-01/result.json records the passing native build and cleanup.json closes
the exact lease after all 71 owned originals were absent. One exact tracked idle
VCTIP was stopped after its normal close returned false and clients had exited.
The slot was handed directly to Livo at 16:32:25Z before this documentation.

Existing HIBYTE redefinition and xUnit2031 warnings remain. This is private build
and offline behavior evidence only: no game, provider, owner install, browser
submission, rendered Help acceptance or public release. The A140BCF6 qualified
candidate and failed runtime05-07 evidence remain unchanged. These docs do not
relabel the built source revision or authorize another activation attempt.

## Offline recheck on main cfd4e26 (2026-10-22)

Fresh Release/Ninja configure in a temp directory outside the repo; targets fzero_mods_tests, fzero_setup_tests, fzero_setup_no_bundle_tests built with MSVC 2022 BuildTools.
ctest -R "fzero_mods|fzero_setup": fzero_setup, fzero_setup_no_bundle, fzero_mods all Passed (3/3).
python -I tests/test_setup_bundle.py: Ran 10 tests, OK (covers report marker 0 for old bundles, marker 1 for the exact capability, malformed or explicit null report refused).

Report action stays unavailable (source read plus the passing checks above):
- Old controls-only helpers: FZERO_SETUP_REPORT_QUALIFIED is 0, so FzeroReportAvailable() returns 0. The no-bundle test asserts this and that no child launches.
- Isolated sessions: launcher_report_available() in src/sdl_main.c also requires !g_isolated_owner_state and no replay/record input or env.
- Linux: FzeroReportAvailable() and FzeroReportLaunch() return 0 outside _WIN32.
Isolated-session and Linux gates were confirmed by source reading only; no executable test covers them.

No new report framework, uploader or direct HTTP submission. No failures to record. No game, helper publish or rendered check ran.
