# Controls capability receipt

The Stream Deck installer now requires the clean checkout's `git describe` stamp
both in `generated/fzero_build.h` and as a NUL-terminated string in the linked
launcher, plus the linked `[fzero-controls]` startup tag. Reconfiguration without relinking is refused. The existing native pin
check remains. These are build provenance checks, not a security signature.

`install-receipt.json` retains its existing fields and adds schemaVersion 1,
controlsProfileSchema 1, controlsProfileAdapter `fzero-raw-wheel-1`, launcher
filename, launcherSha256 and wheelFfbSha256. Wheelkit must match both installed
files before trusting the capability. The adapter is R12-only; it translates
into the existing raw-wheel config, retaining the game's digital pedal threshold.
This receipt does not certify full analogue pedal calibration or observed input.

Every install backs up both owned runtime files and the exact previous receipt
(including absent-before). Rollback verifies all backup hashes before touching
files and restores that receipt together with the runtime. Historical backups
without a receipt withdraw the current receipt rather than label the old binary
with a newer capability. Settings are backed up for reference, never restored or
written by this installer. Matching binaries with an old/missing receipt refresh
only the receipt with a reversible backup.

Validation: `tools/Test-InstallNativePin.ps1` uses inert launcher fixtures and
copies, but never loads, the native DLL. It checks stale/missing build stamps,
native mismatch refusal, receipt-only refresh, no-op, absent/existing receipt
rollback, corrupt-backup refusal and owner-settings preservation. Run under
Windows PowerShell 5.1 after committing the source (installer requires clean HEAD).

No game launch, installation or force test is part of these fixtures. A clean
Deluxe build/install is coordinated separately with Claude; no embedded cartridge
payload is distributed by this change.
