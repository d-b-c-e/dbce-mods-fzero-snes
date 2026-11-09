# Native maintenance candidate

The owner requested the toolkit lifecycle fixes throughout the fleet. F-Zero
adopts the reviewed **41-export** native candidate `50ba139bcaee14aee080abe438d6beec4f6a2b47`
(tag `native-0.6.0-retained-identity`, API version 600), x64 SHA-256
`3F6CC51444879D01FEB900098B2DE94242594D093F523E3C34BD511D69C0D946`.

It fixes retained strict-device identity and carries the reviewed locking and
shutdown changes. A mismatched retained device is stopped/released and that
initialization is refused; independent input readers, watchdog and exit guards
survive. The later explicit initialization remains the consumer's decision.
Both architectures passed production fake-device and actual DLL ABI/lifecycle
checks; no physical device was acquired. Claude reviewed this native candidate
and separately passed the F-Zero accepted-zero startup and steering split.

The 48-export Art/Woden candidate is deliberately not substituted: enabling its
additional constant-burst path here would change impact delivery. The 41-export
surface, headers, force profiles, model arithmetic and owner settings are retained.
`VERSION` remains the headers/profiles base; `SOURCE-PROVENANCE.json` and the
current native build record identify the exact replacement. The detailed old
f8f0619 binary receipt is preserved in `docs/reviews/native-history/` as history,
not presented as evidence for these new bytes. No bit-identical rebuild claim.

The separate steering slider defaults to the runtime's existing 40 when no
Strength is saved (the old panel displayed 35 in that case). This intentionally
aligns fresh panel settings with the runtime; existing Strength remains the
fallback when SteeringStrength is absent. Crash intensity stays independent.

## Installed candidate

All 17 CTest suites, 31 replay/package Python tests and 11 dummy installer checks
passed. The installer refuses missing or stale staged native bytes without
changing the target. A native DLL change now triggers relinking/staging.

The exact clean `4ac6f6b2d1afc49a22bff4f23ac66bd7bf5f6e74` build was installed
with the game closed at 04:36 CT, October 8, using `Install-StreamDeck.ps1`.
The installed launcher and native DLL match the build; five protected root
configuration/ROM files stayed byte-identical. The target's `install-receipt.json`
records the payloads. Backup: `deployment-backups/2026-10-08-steering-native50ba`;
private pre-install hashes: `build/fleet-protected-before.json`.

This local launcher embeds private Deluxe data and is not a public distributable.
The 05:39 CT muted plain-launch check rendered the installed launcher and closed
normally (exit 0). Force feedback and telemetry were temporarily disabled in
the saved INI, then all 36 root/save/diagnostic files were restored byte-exact.
The game-window capture shows the launcher, verified ROM and Keyboard selected;
no settings navigation or device binding was tested. Evidence:
`SessionEvidence/fzero-muted-launcher-20261008-0539`. Steering-slider rendering,
hardware feel and calibration to Art remain pending.
No physical force was sent. The three complete synthetic stock playthrough
replays in `../2026-10-08-stock-replay-proof.md` are separate from this installed
Deluxe build and are excluded from normalization.
