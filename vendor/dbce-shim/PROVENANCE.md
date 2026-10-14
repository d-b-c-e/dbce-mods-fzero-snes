# Shared input shim: private extras-capable candidate

The current optional private source/staging pin is the clean reusable wheel
toolkit source `ea568c258d2373de55efabf1b16b0776990fb2cd`, x64/MT,
clang 19.1.5. Its native closure is identical to toolkit `88ccd1d`; no F-Zero,
OutRun decomp, cartridge or other game data is included in that native source.
The retained native source/license snapshot and actual DLL are at
`E:/Source/_archive/2026-10-14/fzero-shim-ea568c2-bf6455e6`.

| File | SHA-256 |
| --- | --- |
| `dbce_shim.h` | `f590a9f77a81999cae8b4e7c6b8ecd83eade44724564bf35f3591d725ac0c0fa` |
| `dbce_shim_extras.h` | `1c65f15097e61bb3175f9ed3721458b775a05091515bfce208c407fc9c261912` |
| private `dbce_shim.dll` | `bf6455e63696a93b37c19dabf09b1e0606f26b0534bce828417b1df602dd8a7a` |

Both headers are byte-exact copies of that source's Windows checkout, retained
with `-text` so a fresh checkout preserves their hash. The base header is
10,098 bytes in CRLF form; normalized LF is 9,961 bytes, SHA-256
`429c343b83fc2f2192b9de62bf98cdb6127ceed12ecf2e3c3e4a945e208668c7`.
The extras header is unchanged from the earlier vendored header at toolkit
`88ccd1d` (last header change `ea568c2`). Base ABI 2, the 296-byte common frame,
128-byte x64 open args and the 31 common actions remain. The additive extras
ABI is 1 with a 296-byte frame. The base header adds the optional pure profile
inspection declaration and x86 layout notes; F-Zero remains an x64 consumer.

The actual native build passed device-free Core 192, extras 72, provider 106,
host 38, ABI 223 and bridge 78 checks. Its receipt lists all 26 declared exports,
clean source, compiler and DINPUT8/bcrypt/KERNEL32 imports. F-Zero's pure checker
validated its 14 required exports, five ABI/layout values, 31 common actions
and bounds without provider calls. This establishes a native artifact contract,
not physical bindings, a rendered game, force, installation or public delivery.

## Private staging and compatibility

The DLL is not tracked or included by default. Explicit `FZERO_SHIM_DLL`
accepts this exact retained hash for new private builds. The stock stager's
`--private-shared-input` and the optional private installer source use the
existing `PIN.json` route and own five files: DLL, base header, extras header,
PIN and this provenance. Receipts carry `extrasAbi=1` only for that verified
extra header inventory. These source changes do not install a game or rewrite
the owner's configuration.

Older private installs and immutable recordings retain the original direct
provider `9b8a036f664cf8f45cccf82b98e5e4376f93215b60eac50b6db965bf498015d8`
from `f49cfc559f065b778967a7c94068a75df538ec69`, with base header
`3c63b472fc78770e8b9fc90ae92debbf979eedd7f8c68800f56b8690f4f65b74`.
The updated companion still accepts their exact four-file ownership for
ordinary Shared with extras off. It refuses `SharedExtras=1` on such a receipt;
the game also refuses missing extension exports before opening a provider.
Native/default and replay do not open Shared. Enabled Shared FFB and the legacy
launcher remain refused. New X/Y/L/R bindings are explicit `[Extras]` actions,
not aliases or conversions of the older SDL bindings.

## Source terms and public boundary

The toolkit's `native/shim/PROVENANCE.md` describes DBCE authorship and the
source transfer from OutRun; these toolkit files use its MIT `LICENSE`.
That license, the exact native source ZIP and actual clean binary receipt are
retained with this candidate. This does not change F-Zero's public package
policy: the public packager still refuses a private receipt or DLL, and does
not admit embedded Deluxe/cartridge data. Public packaging/notices review and
consumer/runtime/physical qualification remain separate. See
`docs/2026-10-14-shared-extras.md` for build, review and next-action evidence.
