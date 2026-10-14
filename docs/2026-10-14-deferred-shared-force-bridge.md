# Deferred Shared input and force ownership integration

Read-only source design. Coordinator deferred implementation in favor of the
smaller private consumer build/HUD slice. No native ABI, force gain, effect
duration, installed payload or settings changed. Save/rewind declaration extension
is also deferred; ordinary Shared already retains Select+R/L gestures.

## Verified restrictions

Game source `369c926` rejects Shared plus enabled FFB in `fzero_shim.cpp`, takes
the permanent no-force latch and skips `FzeroFfbInit`. The existing force path
owns WheelFfb's module and exclusive handle, uses unique configured device-name
resolution plus strict GUID selection, and has serialized shutdown. Its pinned
native source is `50ba139bcaee14aee080abe438d6beec4f6a2b47`, native 600/41 exports,
DLL `3f6cc51444879d01feb900098b2de94242594d093f523e3c34bd511d69c0d946`.
That pin has no qualified snapshot API.

Toolkit master `d072281` explicitly refuses extras plus a copied-input bridge
in `native/shim/shim_dll/dbce_shim.cpp`. The copied `ProviderHost` constructor
does not pass an extras declaration to Core. Existing bridge tick has no combined
common/extras output. Removing the refusal alone would not deliver extras.

## Existing snapshot contract to reuse

- `codex/art-shim-snapshots` at `82c40c43d76db3076695aa25f2850b5aa113a351`:
  `GetReadDeviceInfoV1`, `ReadDeviceSnapshotV1`, `GetReadDevicePathV1`.
  Metadata/sample layouts are 144/232 bytes at pack 8. Native source retains
  external-session, injection and reentry guards. Original snapshot commits are
  `ecc49bd` and owned-path `64ccf67`.
- `mufa/native-091-read-snapshots` at
  `126a9921d375d4a4f3cb8815c89b20a58c170b6d` ports those contracts onto `c8ec2ee`.
  Its `docs/READ-SNAPSHOT-BRIDGE.md` records the Romi 32682 function audit:
  86/91 functions identical, five invalidation/generation additions, all 32
  force/init/send/effect entry points unchanged against that baseline. Mixed
  borrowed-wheel/auxiliary readers, failed reads, same-address reopen and
  invalidation mutants are documented. Native 901/51-export candidate hash:
  `44e46799945438bc07df738472602db1e3ecc04c3f77cad5b12fcacf49c69272`.
  The port excludes external/injection guards because its baseline lacks them.
- `codex/fzero-lease-read` at `ad81439` only changes rig-lease read assertions;
  it provides no input snapshot ABI.

These are inspected source and retained documentation receipts, not a new test
execution or proof of binary compatibility with F-Zero's native-600 pin. Both
snapshot branches expose the same V1 header. Adoption needs an explicit source
delta audit and candidate provenance; do not merge either divergent branch wholesale.

The snapshot qualifies identity/caps/ranges from the actual owned handle. Each
sample carries original QPC interval, source generation/sequence, separate
HRESULT/classification and validity. It never acquires, retries or injects.
Failed reads end qualification. Reentry invalidation may return successful
HRESULTs with validity false; preserve that failure rather than inventing fresh
neutral data. Only the original owner may recover.

## Smallest future source integration

1. Toolkit additive ordinary bridge/extras open and tick exports; pass the same
   four-slot C4 declaration through the copied host constructor. Copy one sample
   batch, resolve once and return matching common/extras sequences. Preserve
   base ABI 2/extras ABI 1 and every existing handle rule. No latched/capture extras.
2. Game-owned adapter keeps one WheelFfb module/force owner and bounded read
   slots for actual profile GUID/product identities. Steering and force GUIDs
   must match; the wheel slot must identify the borrowed FFB handle. Auxiliary
   slots remain owned by that module. Preflight capacity and capabilities.
3. Stop the copied bridge before closing read slots, then perform existing
   PanicStop, effect release, FreeDirectInput and module unload. The call gate
   excludes late readers. Diagnostic/replay mute remains permanent; ordinary
   bridge capability stays explicit and default off until separately qualified.

Proposed toolkit claims: `native/shim/shim_dll/dbce_shim_bridge.h`,
`dbce_shim.cpp`, `host/provider_host.hpp`, bridge/ABI tests and bridge docs/export
inventory. Proposed game claims: new `src/fzero_shared_owner.cpp/.h`, existing
shim/FFB headers and implementations, SDL startup/exit hunks, CMake and joint
fake-lifecycle fixtures. Pins/header adoption and private receipt inventory follow
only after the candidate audit. No current claims or implementation start.

Fake loaders/sinks would verify one owned wheel handle, one source Poll/state
per tick, independent auxiliary generations, identity/failure/reopen refusal,
stale delivery and rearm, pause/overlay behavior, late callbacks and shutdown
ordering. Existing force arithmetic and emitted command shapes would be compared
without loading a real force provider. Physical acceptance remains separate.

This spans toolkit ABI, native candidate adoption and game lifecycle work. The
selected smaller release blocker is a real clean graphical consumer whose linked
declaration/marker and private receipt are verified, plus honest Shared HUD wording.
