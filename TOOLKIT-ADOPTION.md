# Toolkit standards adoption

Which entries of the wheel toolkit's standards ledger
(`E:\Source\toolkits\dbce-wheel-mod-toolkit\STANDARDS.md`) this mod has brought in.
Update a row in the same commit that adopts it. Statuses: `adopted`, `partial`,
`pending`, `n/a` (say why), `unchecked` (nobody has looked yet).
Audited 2026-10-04 (read-only standards audit); STD-001 and STD-010 revised 2026-10-04 after the merge and install check.

| Standard | Title | Status | Notes |
|---|---|---|---|
| STD-001 | One mod per game | partial | wheel/FFB/telemetry/triple/recording merged with main on claude/fzero-main-merge (2026-10-04, merge of codex/triple-wheel-integration a1a1267; Release MSVC build with embedded BS Deluxe and 15/15 ctest pass); now merged to main b7eb125; previous staging branch is historical. codex/analog-wheel, codex/force-feedback, codex/telemetry hold superseded early versions (no unique work worth merging) |
| STD-002 | Recording and playback from launch | partial | deterministic input recorder + WRAM hash replay (fzero_playthrough.c, docs/PLAYTHROUGH_RECORDING.md) now on main b7eb125; env-var builds; the one stored case (wheel-drive-20260928-235320.fzpt) diverges at frame 155 on both the a1a1267 build and the merge build, so it needs re-recording |
| STD-003 | Normalized FFB strength | partial | Device-free production-model synthetic component trace; spring/damper/road remain distinct, default40 unchanged. Recorded reference and physical normalization pending. See docs/FFB-COMPARISON.md. |
| STD-004 | Consistent settings UX | partial | settings in the recomp launcher Mods page; F6/F8 taken by save slots/rewind (sdl_main.c:2020,2481-2503) |
| STD-005 | Camera numpad layout 8/2 9/3 4/6 7/1 +/- 5 | n/a | no 3D camera rig; Mode 7 fixed chase view |
| STD-006 | Camera step sizes are settings | n/a | no adjustable camera mount |
| STD-007 | Triple screens in one wide window | partial | side panels draw ground, sky and now live opponents (712a7cf, billboard at world anchor; seen headless frame 2070); manual enable, no Auto; side-by-side test install "F-Zero (SNES Recomp) triple-test"; seen at the rig under Surround 2026-10-05 (attract demo, 60 fps; side ground aliases at distance) |
| STD-008 | Display changes: game applies once | partial | SDL FULLSCREEN_DESKTOP, no mode changes (likely compliant); watchdog run pending |
| STD-009 | Dashboard telemetry matches the HUD | partial | speed from position deltas at a guessed 0.25 m/unit, not the HUD; RPM derived from speed (1200-12000, documented); gear fixed at 1 |
| STD-010 | Install the latest build for testing | adopted | `tools/Install-StreamDeck.ps1` backs up the replaced launcher/WheelFfb.dll to deployment-backups\<date>-<label>, installs build-merge's exe as FZeroSNESRecomp-wheel-launcher.exe (the Stream Deck target) and writes install-receipt.json; `-Rollback` restores. Installed 2026-10-07 from ce85080 (label surround-labels); not launched since (17/17 ROM-free tests) |

| STD-011 | Work lands on main | adopted | Packaging research starts from b7eb125 and returns to main; private ROMs, generated content and recordings remain outside release archives. |
| STD-012 | Reproduce the route and preserve original signals | partial | Deterministic input recorder and WRAM-hash replay (STD-002 row); grid REC/PLY 🔧, the replay test is to be re-recorded on current main |
| STD-013 | The installed build launches plainly | partial | The Stream Deck key launches the integration build with no extra arguments (grid PLN 🧪); triple screens start only when the Triple Screen mod is on in the launcher |
| STD-014 | Request reciprocal review when progress stalls | adopted | Process standard; reviews go through the portfolio inbox |
| STD-015 | Triples on Surround and on separate monitors | partial | Display layout Surround (one spanning fullscreen display) or Separate monitors (one borderless SDL window per display); Surround seen at the rig 2026-10-05, separate displays not yet at the rig |
| STD-016 | Telemetry: Forza Horizon layout, on by default | partial | Forza Horizon 324-byte UDP built in, but off by default (`[Telemetry] Enabled=1` in config.ini) |
| STD-017 | Hide settings pages that have nothing to offer | n/a | Settings live in the recomp launcher's Mods page; there is no family panel |
| STD-018 | Handling changes never reach online scores | n/a | No online play or leaderboards |
| STD-019 | Menus on the centre screen; side screens only in gameplay | unchecked | The centre window keeps the stock game, vehicle and HUD; whether the side windows stay black in menus is not yet checked |
| STD-020 | Standard feature checklist per game | adopted | Row in the portfolio grid (dbce-project-mgmt PROJECTS.md) |
| STD-021 | art of rally is the FFB reference | pending | Own condition-effect model (spring, damper, road), default 40; Strength 50 trace exported (docs/FFB-COMPARISON.md); physical comparison with art at 50 pending |
| STD-022 | One triple-screen selector | adopted | Launcher: the Triple Screen mod on/off is Off, then Display layout "Surround" / "Separate monitors" (labels aligned 2026-10-07; saved values stay Span/Separate) |
| STD-023 | Frame-rate readout in the settings panel and the log | unchecked | Ctrl+F7 sets the presentation frame rate; no measured readout yet |
| STD-024 | Optional on-screen frame-rate counter | unchecked | As STD-023 |
| STD-025 | One force model for every tyre game | n/a | Hover machines with no tyre model; the condition-effect model stays game-specific |
| STD-026 | Deliberate player tuning | partial | Launcher exposes steering/crash; saved legacy auxiliary level is retained. New candidate remains uninstalled pending review. |
| STD-027 | Independent steering and effect strengths | partial | October 8 candidate splits spring/fallback from damping/road; preserves legacy model 2 and adds model 3/headless header. Build, model/lifecycle/provider and adapter tests pass; rendering and feel pending. See docs/2026-10-08-steering-strength.md. |
| STD-028 | Complete owner registry snapshots | n/a | Native app uses INI/save files; current test and install paths do not export/delete Unity registry keys. |
