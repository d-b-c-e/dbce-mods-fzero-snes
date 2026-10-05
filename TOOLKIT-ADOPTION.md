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
| STD-003 | Normalized FFB strength | pending | own formula strength*55/100/40 (fzero_ffb.cpp:120-128), default 40; toolkit force model/profile unused |
| STD-004 | Consistent settings UX | partial | settings in the recomp launcher Mods page; F6/F8 taken by save slots/rewind (sdl_main.c:2020,2481-2503) |
| STD-005 | Camera numpad layout 8/2 9/3 4/6 7/1 +/- 5 | n/a | no 3D camera rig; Mode 7 fixed chase view |
| STD-006 | Camera step sizes are settings | n/a | no adjustable camera mount |
| STD-007 | Triple screens in one wide window | partial | side panels draw ground, sky and now live opponents (712a7cf, billboard at world anchor; seen headless frame 2070); manual enable, no Auto; side-by-side test install "F-Zero (SNES Recomp) triple-test"; seen at the rig under Surround 2026-10-05 (attract demo, 60 fps; side ground aliases at distance) |
| STD-008 | Display changes: game applies once | partial | SDL FULLSCREEN_DESKTOP, no mode changes (likely compliant); watchdog run pending |
| STD-009 | Dashboard telemetry matches the HUD | partial | speed from position deltas at a guessed 0.25 m/unit, not the HUD; RPM derived from speed (1200-12000, documented); gear fixed at 1 |
| STD-010 | Install the latest build for testing | partial | Stream Deck key runs Launch-FZeroRecomp.bat -> FZeroSNESRecomp-wheel-launcher.exe, which is already the integration build (SHA-256 80dba7ff..., embeds BS Deluxe, backups under deployment-backups\); LaunchBox still runs the stock 1.7.0 FZeroSNESRecomp.exe; no backup-and-install script |

| STD-011 | Work lands on main | adopted | Packaging research starts from b7eb125 and returns to main; private ROMs, generated content and recordings remain outside release archives. |
