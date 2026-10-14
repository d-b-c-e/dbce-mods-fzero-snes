# Shared input shim: private diagnostic dependency

The ABI 2 header source is pinned to `d-b-c-e/outrun2-decomp`,
`codex/shim-device-path`, **f49cfc559f065b778967a7c94068a75df538ec69**.
This is the same reviewed native build used by DRIVE; it includes the exact
owned-handle HID interface-path report on top of `eea0660`.

| File | SHA-256 |
|---|---|
| `dbce_shim.h` | `3c63b472fc78770e8b9fc90ae92debbf979eedd7f8c68800f56b8690f4f65b74` |
| private `dbce_shim.dll` | `9b8a036f664cf8f45cccf82b98e5e4376f93215b60eac50b6db965bf498015d8` |

The header byte pin is the reviewed Windows **CRLF** representation (8,698
bytes), retained exactly in Git with `-text`; fresh Windows checkouts must keep
the same hash. The upstream Git blob uses LF (8,573 bytes, SHA256
`ce9c25b9d5c9386b5eacc4763cfbcbdc1d8af9d6326b524a009cb1eee2f7b5e3`).
Normalizing CRLF to LF yields that upstream blob exactly; no declarations differ.
This byte-reproducibility note supplies no additional license or distribution grant.

The DLL is **not tracked or included by default**. For a private candidate,
pass `-DFZERO_SHIM_DLL=<reviewed DLL>`; CMake verifies that hash and stages it
beside the graphical executable. The loader checks ABI, struct sizes and every
action name before opening. A diagnostic launch without the DLL refuses.
Default Native launches do not load it. The opt-in ordinary Shared option
also loads this pinned ABI, without capture or injection. Shared+FFB remains
refused until force/read ownership is integrated and qualified.

`PIN.json` provides the same exact hashes for private staging. The stock-only
stager admits it only with `--private-shared-input`, retains this provenance and
the header, and marks the receipt private. The public packager refuses both
that receipt and `dbce_shim.dll`. The ordinary Shared runtime has narrow private
at-rest recording/replay proof, not full owner-control or force qualification;
see `docs/2026-10-12-ordinary-shared-input.md`.

The target builds `windows/shim` and `windows/host` with the shared controls and
session headers, independently of OutRun's reconstructed runtime. Its reviewed
build used Visual Studio 2022 ClangCL x64, Release, `-DOR2_XBOX=ON`, target
`dbce_shim`. Shared native checks at the pin: Core 163, provider 106, ABI 132.

No redistribution grant for this provisional target is asserted. F-Zero's root
license does not license this dependency. Extraction into the reusable toolkit
and complete source/license notices remain required before public distribution.
Do not add this DLL to the public player-package allowlist on this evidence.

## October 14 declared-extras source seam

`dbce_shim_extras.h` is copied byte-for-byte from the reusable wheel toolkit's
`native/shim/shim_dll/dbce_shim_extras.h` at
`88ccd1d86fc1ab7088986ca279adcb7d44f9ccb6` (last header change `ea568c2`).
Its SHA-256 is
`1c65f15097e61bb3175f9ed3721458b775a05091515bfce208c407fc9c261912`.
This is the additive extras ABI 1 header, alongside base ABI 2. The `-text`
attribute preserves the inspected Windows bytes. No DLL or base PIN changed.

The current pinned private DLL predates this extension. `SharedExtras=1`
therefore refuses that runtime before opening a provider. An extension-capable
private DLL needs its own source/binary review and explicit pin/staging update
before this opt-in consumer can be delivered. The source seam and an ABI fake
do not establish physical input, force or redistribution qualification.
