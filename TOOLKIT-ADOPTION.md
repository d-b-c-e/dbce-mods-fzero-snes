# Toolkit standards adoption

Which entries of the wheel toolkit's standards ledger
(`E:\Source\toolkits\dbce-wheel-mod-toolkit\STANDARDS.md`) this mod has brought in.
Update a row in the same commit that adopts it. Statuses: `adopted`, `partial`,
`pending`, `n/a` (say why), `unchecked` (nobody has looked yet).
Audited 2026-10-04 (read-only standards audit); statuses below are from that audit.

| Standard | Title | Status | Notes |
|---|---|---|---|
| STD-001 | One mod per game | partial | wheel/FFB/telemetry/triple all on codex/triple-wheel-integration (a1a1267, 125 ahead of main, worktree in _archive\2026-10-04); main is the stock port; 3 stale unmerged branches |
| STD-002 | Recording and playback from launch | partial | deterministic input recorder + WRAM hash replay (fzero_playthrough.c, docs/PLAYTHROUGH_RECORDING.md) on the integration branch only; env-var builds |
| STD-003 | Normalized FFB strength | pending | own formula strength*55/100/40 (fzero_ffb.cpp:120-128), default 40; toolkit force model/profile unused |
| STD-004 | Consistent settings UX | partial | settings in the recomp launcher Mods page; F6/F8 taken by save slots/rewind (sdl_main.c:2020,2481-2503) |
| STD-005 | Camera numpad layout 8/2 9/3 4/6 7/1 +/- 5 | n/a | no 3D camera rig; Mode 7 fixed chase view |
| STD-006 | Camera step sizes are settings | n/a | no adjustable camera mount |
| STD-007 | Triple screens in one wide window | partial | span mode when three panels wide (manual bool, no Auto); side panels draw ground and sky only, no vehicles/effects |
| STD-008 | Display changes: game applies once | partial | SDL FULLSCREEN_DESKTOP, no mode changes (likely compliant); watchdog run pending |
| STD-009 | Dashboard telemetry matches the HUD | partial | speed from position deltas at a guessed 0.25 m/unit, not the HUD; RPM derived from speed (1200-12000, documented); gear fixed at 1 |
| STD-010 | Install the latest build for testing | pending | Stream Deck copy never replaced; BS Deluxe compatibility blocker; no backup-and-install script |
