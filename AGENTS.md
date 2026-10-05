Public packaging review (2026-10-05): read docs/PUBLIC-PACKAGING.md. Public fork main b7eb125 includes the integration; 24 package tests pass. No new binary release. Deluxe is compiled with embedded cartridge data; a stock-only ZIP cannot gain Deluxe just by adding a patch. Preserve owner builds/assets and confirm the distribution route before packaging Deluxe.

# Agent notes

## Toolkit standards

At the start of every session, compare the wheel toolkit's ledger
(`E:\Source\toolkits\dbce-wheel-mod-toolkit\STANDARDS.md`) with this repo's
`TOOLKIT-ADOPTION.md`. Report any entry that is `pending`, `unchecked` or missing
from the adoption file, and bring it in when your work touches that area. When you
adopt one (or find it does not apply), update `TOOLKIT-ADOPTION.md` in the same
commit. When you set a new family-wide standard, append it to the toolkit ledger
and commit it in the same turn; do not leave it only in an uncommitted file.
