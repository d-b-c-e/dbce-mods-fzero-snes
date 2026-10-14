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

## Review closure and BOM follow-up

Kiki's independent read-only review of `4856b02` found one P2: a UTF-8 BOM
before an Extras-first header made the new section scan miss that header.
Follow-up `fab3faa8b18a3625f5c230816c06d668bfef0437` uses one shared BOM offset
for both section scans. New
`extrasfirst` and `extrasbom` ordinary host fixtures exercise equivalent
section ordering with/without a BOM, exact Core profile bytes, combined ticks
and the actual tape record/verify path. MSVC Release builds passed without
compiler warnings; the final focused subset is **36/36**, 0.91 seconds,
with lease release confirmed. The earlier successful 34-case logs are retained
as `build-34-pass.log` and `tests-34-pass.log`.

Kiki inspected the follow-up diff and retained 36-case result and reported
no remaining correctness finding (hcom `106799`). The reviewer did not rerun
builds/tests; this closes source/offline review only.

## Read-only native-adoption inventory

The clean October 14 extras-capable native build receipt and log still exist:

```text
C:/Users/antho/AppData/Local/Temp/claude/E--Source-dbce-project-mgmt/
ed7ad792-5fc2-49eb-a015-cb326eca9270/scratchpad/schema-helper-0914/
receipt-x64.json
build-x64.log
```

The receipt names source `ea568c258d2373de55efabf1b16b0776990fb2cd`, x64,
static MT CRT, clang 19.1.5, `sourceChanged=false` and historical DLL SHA-256
`D26ADBF75323E0BB52E8F9CD09F3DEC7614E68B015219E0582C2402B80819085`.
It lists all five consumer-required extras exports among 26 exports, with
DINPUT8/bcrypt/KERNEL32 imports. The retained native log passes Core 192,
extras 72, provider 106, host 37, ABI 224 and bridge 77 checks. The receipt
proves that earlier build result; it does not provide the DLL bytes today.

Native closure (`native/shim`, `native/controls`, `native/session`) has no diff
between `ea568c2` and current toolkit `88ccd1d`. Nene's original transcript
exchange 1133 records removal of `extras-accepted-schema` after merge. Its
`schema-and-helper.ps1` copied receipts/logs, not the DLL. The original native
build directory is absent. No retained, hash-verifiable extras DLL was found
in the inspected toolkit main build/distribution paths, isolated worktrees or
hub native artifacts. Existing older binaries/receipts predate extras; the
x86 ordinary-only variant is also unsuitable for the x64 F-Zero consumer.
No dependency pin was changed from this inventory.

The smallest next native step is one serialized, clean **x64-only** toolkit
build at that source closure, retaining its DLL, clean receipt and exact
source/license provenance in an immutable private artifact. Rebuilding may
change the raw hash through link timestamps, so do not pin historical D26
from its receipt alone. A new payload then needs focused consumer validation
and explicit private CMake/stager/installer/receipt adoption. No shared-source
modification or x86 rebuild is needed for this F-Zero slice.
