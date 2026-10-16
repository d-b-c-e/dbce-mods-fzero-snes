# Private installer integration and package handoff

Branch `beni/fzero-private-integration-20261015` merges reviewed
`a79989131938a5e5a9827cd2ce7301614134f770` into an isolated checkout based on
local main `796cfeb137260b5c3a4747650e5ce86478186903`. Merge
`f22d09e8ed836ee277461a947ba068c3b52f91e7` has both as parents and exactly
the reviewed source tree. The following commit adds documentation only.
The original main and frozen source/candidate checkouts were not advanced.

The integrated stack retains game `77faadbe661f74d8fdf287d6c6de23131c44c197`
as an ancestor, UI gitlink `217ce4457402555387b1a1d40ed5db75b7c68997`, and
engine gitlink `59d2966fa71d0346f92c43753e09b1ba7a1956f5`.

Internal installer failure recovery uses the already-admitted helper inventory
even when final receipt output is truncated. External manual rollback retains
its current-inventory, backup, path, link and hash guards. Full helper install
receipts alone use the existing duplicate-field validator with a 2 MiB bound;
ordinary session/status JSON keeps its 64 KiB bound.

One execution of Test-ControlsSetupRecovery passed 14 focused inert-file cases
at 2026-10-16 04:48:16.686-04:48:20.386 UTC. Separate closure at 04:48:28.303
verified all three original process births Absent, zero scoped processes,
complete query, unchanged pins and free lease. Exact installer, Recording,
Private-ControlsSetup and test bytes are unchanged in this integration; those
passing results are reused without repeating tests. The broader authored
Test-InstallControlsSetup suite remains unrun.

Evidence in `E:/Source/dbce-project-mgmt`, commit
`dfef24637e18704674904e6bd09c17cb106235a8`:

- `artifacts/fzero-controls-install-fixes-beni-20261015/run-01/result.json`:
  `D39B25DBDD76705AC4D50D51974CD7DC0281A68ABA1CFD2376D46574F69B36E8`.
- `run-01/independent-exit.json` in the same folder:
  `E19F24DBFD4005ED85CE037DDCFA3F4B2555FA57C57295BBCFA573234830EA6F`.
- `READY.md`, `RESULT.json` and `MERGE-RELEASE.md` distinguish historical
  preparation, actual qualification and remaining delivery gates.

This source is ready for a later matching private package build, with no binary
package created here. Use the existing stage/install routes and their explicit
PrivateSharedInput/PrivateControlsSetup options; preserve source stamps and the
qualified helper manifest. The current stock 575-file payload cannot satisfy
the Deluxe owner-install gate. A matching Deluxe build, disposable real
install/rollback validation and separately authorized owner/runtime handoff
remain necessary. Preserve the private Shared/Deluxe distribution restrictions
in PUBLIC-PACKAGING.md. No public push, owner install, game attempt, force tune,
store change or heavy rebuild accompanied this integration. Original live
01-03 results and the frozen 575-file payload remain unchanged.
