# BS Deluxe controls-helper admission (2026-10-17)

`tools/Install-StreamDeck.ps1` remains the only BS Deluxe producer. With `-PrivateSharedInput` its
`install-receipt.json` now also declares the Shared controls contract (the stock stager's partition from
`stage_unified.shared_controls_contract()`, with `FZeroSNESRecomp-wheel-launcher.exe` as consumer).
Wheelkit's helper admits a Deluxe copy only from that single receipt: renamed launcher, matching
`files[]` row and bytes, linked embedded Deluxe payload and Shared adapter, and the same ABI-2/extras
pins. A folder with both receipts is refused, and a plain `FZeroSNESRecomp.exe` beside the launcher is
never used. Wheelkit source `b959fab` is integrated in canonical `89878cd`. The older Deluxe candidate's
receipt (no controls contract) stays unadmitted and unchanged.

Qualified source `a649354`:
- `tests/test_private_shared_install.ps1`: 50 checks, including equality with the stock partition.
- The real Deluxe delivery on helper `89878cd`: fresh Deluxe host, real installer on a disposable copy of
  the older candidate, both actual-byte Wheelkit admission cases, the installed helper's PlanOnly, and a
  byte-exact `-Rollback`. Evidence and retained failures are in hub
  `artifacts/fzero-deluxe-admission-zule-20261017/RESULT.md`.

Not covered: game launch, rendered UI, physical controls/force, owner installation, distribution.
Deluxe payloads and built hosts stay local; this route never packages them.
