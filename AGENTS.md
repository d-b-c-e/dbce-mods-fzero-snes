# F-Zero (SNES) — agent guide

Portfolio rules: `E:\Source\dbce-project-mgmt\HOW-WE-WORK.md` (read first; it replaces older process, lease, review and testing instructions). Current focus: `E:\Source\dbce-project-mgmt\NOW.md`. Feature status lives in the portfolio grid (`python E:\Source\dbce-project-mgmt\tools\portfolio_grid.py plan fzero`), not in this file. Grid id: `fzero`.

## Project

Native PC static recompilation of F-Zero (SNES, USA) plus an optional embedded BS F-Zero Deluxe. Public fork of `mstan/FZeroSNESRecomp` (upstream 1.8.3; `VERSION` is the upstream base). Repo: `d-b-c-e/dbce-mods-fzero-snes` (PUBLIC; the owner pushes). Top-level source is MIT; the assembled product is not MIT-only (see `LICENSE`, `licenses/`).

## Build

- Stock: `.\build.ps1` (Ninja + MSVC, output in `build`). Needs your own USA ROM at `fzero.sfc` and generated sources (`bash tools/regen.sh`).
- Deluxe (what the Stream Deck copy runs): `.\build.ps1 -Deluxe -Test` builds in `build-merge`. Needs the private generated sources in `captures\bs-deluxe\gen` (`tools/regen_bs_deluxe.py`).
- Clone with `--recurse-submodules`.

## Test

- ROM-free suites: `ctest --test-dir build --output-on-failure` (or `.\build.ps1 -Deluxe -Test`).
- Needs a ROM and an interactive desktop: `tests/test_msu_integration.py`, `tests/test_desktop_integration.py`, `tests/test_rom_persistence.py`, `tests/test_launcher_options.py`.
- Save states and settings live beside the exe (`saves/`, see README "Where states are kept"). Back up and restore them around a run.

## Install / run on the rig

`.\tools\Install-StreamDeck.ps1 -Label <name>` installs the Deluxe build into the Stream Deck copy with a backup and receipt. Run the exe from that copy.

## Shared code

Toolkit code is in `lib/toolkit` and `vendor/dbce-shim`. Change shared code in the toolkit and re-sync; never edit the vendored copy.

## Project-specific rules

- No ROM, BS Deluxe data or other private assets in the repo (`fzero.sfc`, `captures/`, generated sources and builds are git-ignored).
- Deluxe is compiled with embedded cartridge data; a stock-only package cannot gain Deluxe by adding a patch.
- Do not substitute an upstream 1.8.3 ZIP for the DBCE build.
- MSU-1 music and shaders are user-supplied; nothing is bundled.

## Key docs

- `README.md` (setup, settings, build)
- `docs/UNIFIED-PRODUCT.md`, `docs/PRODUCT-IDENTITY.md`, `docs/PUBLIC-PACKAGING.md`
- `docs/FFB-COMPARISON.md`
- `TOOLKIT-ADOPTION.md`


<!-- BEGIN project-file-location -->
## Project files and temporary work

Keep project-related research, evidence, scripts, packages, and other working files inside the relevant project folder whenever practical. Keep disposable or machine-specific outputs git-ignored when they do not belong in version control; never ignore files that should be committed merely to hide them. When a project folder is unsuitable, use `E:\Source\temp\<project>` instead of creating scattered drive-root folders. Record the location of material needed by later work and update references when relocating it.

Clean up temporary files and disposable copies promptly once they are no longer useful. Preserve active work, user data, settings, saves, required evidence, backups, rollback artifacts, and worktrees until their retention or replacement is established. Follow existing project, rig, and data-loss safeguards; do not remove files merely because they are old or untracked.
<!-- END project-file-location -->
