**FFB comparison (2026-10-06):** `docs/FFB-COMPARISON.md` documents the new offline tools and remaining physical acceptance. No installed tune/default was changed. Coordinate normalization with the shared toolkit findings before changing gains.

Public packaging review (2026-10-05): read docs/PUBLIC-PACKAGING.md. Public fork main b7eb125 includes the integration; 24 package tests pass. No new binary release. Deluxe is compiled with embedded cartridge data; a stock-only ZIP cannot gain Deluxe just by adding a patch. Preserve owner builds/assets and confirm the distribution route before packaging Deluxe.

# Agent notes

October 13 private Shared checkpoint: clean local source `9604802` and unchanged
native `9B8A036F` have a muted ordinary Deluxe Practice drive from launch,
sealed at 1,838 frames, and a later cold replay that verified all 1,838 WRAM
checkpoints with normal exit. The route used explicitly synthetic keyboard
input; the first runner's neutral-tail guard refused before replay, and the
first replay-only wrapper was refused before game launch on SHA text case.
Those failed results remain separate evidence. Read
`docs/2026-10-12-recording-cli.md` and `docs/2026-10-12-delivery-readiness.md`
before further qualification. This is not physical binding, force, owner
installation, public distribution, or owner-attended visual acceptance.
Keep public origin untouched and private assets out of this source repository.

## Toolkit standards

At the start of every session, compare the wheel toolkit's ledger
(`E:\Source\toolkits\dbce-wheel-mod-toolkit\STANDARDS.md`) with this repo's
`TOOLKIT-ADOPTION.md`. Report any entry that is `pending`, `unchecked` or missing
from the adoption file, and bring it in when your work touches that area. When you
adopt one (or find it does not apply), update `TOOLKIT-ADOPTION.md` in the same
commit. When you set a new family-wide standard, append it to the toolkit ledger
and commit it in the same turn; do not leave it only in an uncommitted file.

<!-- BEGIN project-file-location -->
## Project files and temporary work

Keep project-related research, evidence, scripts, packages, and other working files inside the relevant project folder whenever practical. Keep disposable or machine-specific outputs git-ignored when they do not belong in version control; never ignore files that should be committed merely to hide them. When a project folder is unsuitable, use `E:\Source\temp\<project>` instead of creating scattered drive-root folders. Record the location of material needed by later work and update references when relocating it.

Clean up temporary files and disposable copies promptly once they are no longer useful. Preserve active work, user data, settings, saves, required evidence, backups, rollback artifacts, and worktrees until their retention or replacement is established. Follow existing project, rig, and data-loss safeguards; do not remove files merely because they are old or untracked.
<!-- END project-file-location -->
