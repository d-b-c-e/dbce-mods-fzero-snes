# Optional portable helper install ownership: source only

This isolated successor extends the existing Install-StreamDeck.ps1 with explicit
`-PrivateControlsSetup -PrivateSharedInput` opt-ins. It reuses the existing
tools/setup_bundle.py complete bundle admission through the configured Python
application, then requires the exact manifest/apphost identities in the linked
game binary. No second bundle parser/writer, new installer or force bridge.

The installer copies and owns every admitted controls-setup file, with hashes
in its existing receipt. It snapshots those exact paths in the existing backup
transaction. Rollback restores prior bytes or removes only files that did not
exist before, validates every path/link/hash before writing, and preserves
unowned files/directories. Implicit withdrawal or inventory-shrinking upgrade
refuses until the previous owned helper install is rolled back. First adoption
also refuses overwriting an existing unowned helper file.

Native and existing private Shared installs retain their defaults. This does
not change configuration, bindings, force/telemetry settings, saves, ROM data
or the existing Deluxe-only owner-install gate. The current staged stock77faadbe
candidate is not silently made eligible for that owner route. A future matching
Deluxe build/source stamp plus these opt-ins and the real qualified helper is
needed for a separately authorized owner install.

TOOLKIT-ADOPTION STD-032 ownership work is source-ready only. The authored
Test-InstallControlsSetup.ps1 uses inert files and covers canonical paths,
withdrawal guards, whole admission, linked identity mismatch and mutated payload.
No fixture, Python admission, installer, compiler or game was executed here.
No owner installation, merge or publication. Actual install/rollback validation
on a disposable folder and independent review remain pending a named slot.
