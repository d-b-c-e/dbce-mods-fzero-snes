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
The original source checkpoint retained `9B8A036F...015D8`; the private
adoption follow-up below changes the new-build source pin to the archived
`BF6455E6...DD8A7A` candidate. Existing installed receipts retain their exact
old pin. This candidate cannot yet be advertised or installed as qualified
working extra buttons.

An extras-capable private native build is now retained and checked as described
below. The next concrete slice is reviewed, explicit source/binary pin plus
CMake/stager/installer/receipt adoption.
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

## Retained x64 native candidate

After coordinator assignment `106960` and explicit slot handoff `107087`,
dine built clean detached source `ea568c258d2373de55efabf1b16b0776990fb2cd`
in `E:/Source/toolkits/.worktrees/dine-fzero-shim-ea568c2-20261014`, without
source edits, using the existing command:

```powershell
pwsh -NoProfile -File native/shim/build_test.ps1 -Arch x64 -Crt MT
```

The new DLL SHA-256 is
`BF6455E63696A93B37C19DABF09B1E0606F26B0534BCE828417B1DF602DD8A7A`.
The actual receipt records x64, MT, clang 19.1.5, clean source, all 26 declared
exports and DINPUT8/bcrypt/KERNEL32 imports. Actual device-free results:

| Fixture | Checks passed |
| --- | ---: |
| Core | 192 |
| Extras | 72 |
| DirectInput fake provider | 106 |
| Provider host | 38 |
| Native ABI | 223 |
| Copied-provider bridge | 78 |

Polling/scheduling assertions can vary these counts; use this actual build log,
not the historical D26 counts. The DLL is not hash-identical to that removed
historical binary and is not assigned its pin.

`tools/check_shim_dll_contract.py` verified the real artifact against its clean
receipt: 14 consumer-required exports, five pure ABI/layout queries, all 31
ordered common action names and their out-of-range bounds. Base ABI is 2,
frame 296 bytes, open args 128 bytes; extras ABI is 1, frame 296 bytes. The
checker never invokes provider open/tick/record/stop. This contract check is
separate from the consumer's 36/36 ABI-fake fixtures, physical bindings and any
rendered game.

The batch retained its originals under `build-dine-extras-offline/native-ea568c2`.
Read-only, byte-verified archival copies are at:

```text
E:/Source/_archive/2026-10-14/fzero-shim-ea568c2-bf6455e6
```

The archive includes `dbce_shim.dll`, `build-receipt.json`, `build-tests.log`,
`source.zip` of the tracked native closure and license, `LICENSE`, the exact
checker, `consumer-contract.json`, its log, lease receipt and `SHA256SUMS`.
Source ZIP SHA-256:
`d2c06a4447d4d5ef1c08fa97d0c677513b6806af37c19894dcae28b33aa5aab9`.
Receipt SHA-256:
`071bbf06ba493b63d8829f72bbd1d8ae61588f3af958651220ae625897da463e`.
The archived checker is exactly the one used for this result.

Lease `edaacc0e3b234abc9aab990baf101c7b` was acquired at
`2026-10-14T15:29:50.9077080-05:00` and released by the wrapper's `finally`.
The source worktree is clean and retained. No shared source edits, consumer
repin, installation, game launch, real provider/input/force call or UI takeover
occurred. The owner's updated advance-notice restriction was honored.
Next at this checkpoint: reviewed explicit private pin/staging/receipt adoption,
then a separately authorized muted disposable runtime handoff; attended
hardware remains open.

## Private source/staging adoption

Coordinator assignment `107274` authorizes source/provenance/staging adoption
of the exact retained BF6455 artifact. The base header is refreshed byte-exact
from clean `ea568c2` (Windows CRLF SHA-256 `F590A9F7...AC0C0FA`); the already
vendored extras header remains `1C65F150...261912`. `PIN.json` version 1 gains
the additive header hash and `extrasAbi=1`. CMake's optional private DLL check
uses BF6455; no DLL is added to Git or the public payload.

The existing stock stager and optional private installer source now verify and
own five files when that extras pin is present. Legacy two-entry PINs still
produce the original four-file inventory. The companion checks exact owned
files, source/runtime/header hashes and matching extras ABI metadata. It
retains four-file legacy admission with extras off, refuses opted-in extras
against that old inventory before a child launch, and requires the fifth
owned/header-pinned file for a new candidate. Native/default routing, force
refusal, configuration bytes and original tape/state/descriptor are preserved.
There is no installation or new full game build in this source/staging slice.

| Dependency | Common ordinary Shared | `SharedExtras=1` | New private build pin |
| --- | --- | --- | --- |
| Old 9B8A / four files | Legacy exact receipt remains admissible with extras off | Companion refuses missing ownership; game refuses missing exports | Refused by the new CMake hash pin |
| New BF6455 / five files | Same common ABI contract, source/offline checks only | Requires explicit bindings, owned extras header and ABI 1; runtime still unqualified | Exact retained private candidate |

Focused package fixtures cover both inventories, missing/mutated extras
header, unsupported extras ABI, old-receipt extras refusal and temporary
rollback preservation. Public packaging still refuses private receipts/DLLs
and embedded Deluxe. The queued validation batch uses only these focused
fixtures, the existing ROM-free consumer subset and dependency-only staging
through `private_shared_files`; it does not produce a full player package,
invoke owner installation or start a game. Final batch results follow after
the coordinator's serialized slot handoff.
