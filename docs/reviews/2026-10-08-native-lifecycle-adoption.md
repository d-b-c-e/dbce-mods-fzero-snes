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

At this source checkpoint, consumer build/install and muted launcher loading
remain pending. Hardware feel and calibration to Art remain attended checks.
Headless stock playthrough evidence is separate from the installed Deluxe build.
