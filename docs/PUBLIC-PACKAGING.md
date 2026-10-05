# Sharing the F-Zero and BS Deluxe mods — 2026-10-05

The DBCE source fork is already public at [dbce-mods-fzero-snes](https://github.com/d-b-c-e/dbce-mods-fzero-snes). It has no DBCE unified binary release. A source fork and a player-ready download are different deliverables.

## Recommended player package

Publish one Windows x64 portable ZIP with a distinct DBCE preview version, rather than several wheel/triple DLL downloads. The modifications are compiled into this native application and its launcher; they are not a plugin that can simply be dropped into an arbitrary upstream executable.

The ZIP should contain the graphical executable, matching headless replay executable, pinned WheelFfb.dll, exact required runtime DLLs, approved launcher assets/shaders, Setup.cmd, short setup/recovery instructions, complete third-party notices, a file manifest and an external SHA-256 checksum. Setup opens the existing launcher. Players extract to a new folder and select their own verified USA ROM. Keep updates separate until tested; preserve config.ini, fzero-video.ini, keybinds.ini, rom.cfg, wheel profiles and saves.

Do not include ROMs, recordings, owner configuration, music/PCM packs or imported CRT-Geom. The existing stock-only packager also excludes cover art, screenshots, IPS patches and generated Deluxe payloads. Use `stage_unified.py` and `package_unified.py`, not the historical `make_release.py` route. Retain upstream attribution and the full engine license: the root MIT license does not describe every component.

## Deluxe is the unresolved packaging decision

The current local Deluxe build compiles both a generated native module and a cartridge-data delta into the executable. `FZERO_DELUXE_GEN_DIR` selects the native module and `FZERO_DELUXE_DATA_FILE` supplies the embedded data. Removing a loose `.dat` or `.ips` file from a ZIP does **not** remove those embedded bytes.

| Route | What it offers | Work still needed |
|---|---|---|
| Stock-only DBCE preview | Existing package contract; wheel/camera/replay/experimental triples for the player's own stock ROM | Build a fresh stock-only candidate, inspect actual runtime closure/notices, test launcher and settings preservation. Deluxe must visibly say unavailable. |
| Deluxe-enabled DBCE preview | Matches the local Deluxe experience | Confirm the existing upstream authors' permission covers this fork and its redistributed generated module/data; retain the terms/credits in the package receipt. Review the actual fresh build and export contents. |
| User-imported Deluxe | Player supplies supported patch/data privately | Requires runtime/build work. Today's stock-only binary refuses Deluxe without its compiled module. A file picker or simply dropping in a patch is insufficient. Prototype explicit data-only loading/interpreter support or a local build tool, then qualify parity and save separation. |

The practical first deliverable is a stock-only preview while resolving the Deluxe-specific terms, or a contribution of our source changes to upstream for their existing distribution route. No message or pull request to upstream has been sent. Avoid calling a stock-only ZIP a Deluxe release.

Upstream's [current project](https://github.com/mstan/FZeroSNESRecomp) supplies a bring-your-own-ROM application with optional BS Deluxe. Its historical release notes assert author permission. That is useful evidence, but the scope/terms for a separately distributed DBCE build are not recorded locally. The pinned engine [license](https://github.com/d-b-c-e/snesrecomp/blob/59d2966fa71d0346f92c43753e09b1ba7a1956f5/LICENSE) permits noncommercial community use and contains an added profit-derived-use restriction. Preserve its exact text and avoid presenting the assembled application as MIT-only. This is a distribution inventory, not legal clearance.

## Verified tonight

- Main source checkpoint inspected: `b7eb125`; engine `59d2966`, launcher UI `5abcff8`. Earlier audit UI pin `0c15219` is historical.
- Existing package suite: **24 tests passed**. It checks allowlists, source/build receipts, import closure and omission of private data; it does not qualify an actual newly built ZIP.
- `src/fzero_deluxe.c` and CMake confirm the embedded-module requirement above.
- `.gitattributes` excludes patch/cover/screenshots from new source exports. Old history and clones still contain them; export-ignore is not history removal.
- The accepted installed/local build and all user assets were left intact.

## Next concrete release checks

1. Create a fresh clean stock-only build, with both Deluxe CMake paths empty, then stage/package it with its authentic build receipt. Never write a receipt for an old local executable.
2. Check the current UI dependency's asset notices and runtime DLL licenses at their exact pins. Carry unresolved imagery items from [DISTRIBUTION-AUDIT.md](DISTRIBUTION-AUDIT.md) forward explicitly; do not inherit an old pin's approval.
3. Test first launch with missing ROM, valid user ROM, launcher reopening, update without configuration loss, normal exit, and wheel output disabled by default for replay. Existing separate-window focus and incomplete side vehicles/effects remain preview limitations.
4. Audit both GitHub source archive formats at the final source commit and verify the uploaded ZIP/checksum after publication. Publish a support/limitations page with the same version identity.
5. Resolve Deluxe packaging independently. Keep generated cartridge content private until its distribution route is established.
