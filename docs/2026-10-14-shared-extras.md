# F-Zero Shared X/Y/L/R source handoff

October 14, dine. Started from clean local `main` `2107d18`. This change is
source and ROM-free fixture work. No game, physical input/force, owner install,
ROM packaging, public push or triples qualification was attempted.

## Consumer behavior

Native and missing/zero `[Input] SharedExtras` keep the existing paths. Explicit
`Backend=Shared` plus `SharedExtras=1` admits the toolkit's existing declared
extras ABI 1 only when all required exports and layouts are available. Missing
or incompatible exports refuse before a provider opens. Ordinary Shared still
requires an explicit ROM and refuses enabled FFB. Diagnostics use the unchanged
base open/tick/capture path; replay opens no input backend.

The declaration is context-free and byte-exact:

```text
game fzero-snes
fzero-snes.x digital
fzero-snes.y digital
fzero-snes.l digital
fzero-snes.r digital
```

The final LF is part of its SHA-256:
`c4ffc4c85f1b7f72437e5871bdec57f9d5b997a62a9fdd3fd2208fefa0d98fb1`.
Slots 0..3 map to SNES bits 9, 1, 10, 11. These are guest buttons, retaining
their guest meanings in stock and Deluxe. They are not aliases of gears,
camera or any other common action. X/Y/L/R remain available in guest menus
and ordinary host overlays, as native digital buttons are. Host wheel
save/rewind are not declared here.

The consumer copies the exact bounded `[Controls]` and `[Extras]` sections to
Core, rejecting missing/duplicate Extras sections. Core owns binding identity,
product matching, object/range validation and rearming; no SDL GUID or legacy
button-index conversion is inferred. Its bounded extras plan is logged at
startup so invalid/unbound bindings remain visible. A valid common profile
alone is not proof that extras are bound or delivered.

Each gameplay or overlay read calls `dbce_shim_tick_extras` once for both
frames, using the same handle. Common and extras sequence/layout must match;
only Live, finite, normalized, uninjected extras can produce guest buttons.
Missing/rearming/faulted slots stay neutral. There is no fallback device open.
Keyboard words still combine with this output. The existing `.fzpt` writer
records the final consumed guest word and WRAM; no recording format or force
signal stream changed. The original 1,838-frame synthetic Practice pair is
preserved and was not rerun or relabeled as extras evidence.

## Dependency and next action

Only the existing extras header was vendored from toolkit `88ccd1d`; no shared
toolkit or Wheelkit paths were edited. See [provenance](../vendor/dbce-shim/PROVENANCE.md).
Base `PIN.json`, CMake's private runtime hash and package receipts retain
`9B8A036F...015D8`. That DLL predates extras, so the new opt-in route refuses
it. This candidate cannot yet be advertised or installed as working extra
buttons.

The next concrete slice is review of an extras-capable private native build,
then a distinct source/binary pin plus CMake/stager/installer/receipt adoption.
Bind the four declared actions explicitly through `[Extras]` schema 1 with
the declaration hash above and actual instance/product identities; preserve
the owner's Controls, legacy native keys and recordings. Wheelkit's extra
binding writer is separate coordinated work. Then use a serialized, muted
disposable-copy ordinary input/cold replay qualification. Physical bindings
and owner feel need attended acceptance. Launcher preview, Shared force
ownership and host save/rewind remain separate source gaps; triples remain
experimental with incomplete visual/physical acceptance.

## Verification

ROM-free MSVC Release configuration uses `FZERO_BUILD_GAME=OFF` and builds
only the mapper, ABI fake and ordinary/diagnostic host fixtures. The shared
rig lease covers the compilation/test batch; all paths are beneath
`build-dine-extras-offline`, with `configure.log`, `build.log`, `tests.log`,
`Testing/Temporary/LastTest.log` and `lease.json` retained there.

The first batch compiled and passed 31/34 cases. Two failures compared the
fake's CRLF text log to LF; one expected profile acceptance logging to wait
until the extras report passed admission. The fake now writes its tick spy
in binary mode and acceptance logging follows both validations. Failed
results are retained in `tests-first.log` and `build-first.log`.

The corrected follow-up MSVC Release build passed with no compiler warnings;
focused CTest passed **34/34**, in 1.36 seconds. Both batch leases were released.
Coverage
includes exact guest bits, unavailable/rearming statuses, sequence/count/ABI
refusals, non-finite/injected values, bounded extras reporting, exact declaration
and profile bytes, one combined tick per read, keyboard combination and actual
`.fzpt` record/verify. Existing Native/replay/FFB refusal and diagnostic capture
fixtures are included. This is an ABI fake, not real hardware or rendered
runtime evidence.
