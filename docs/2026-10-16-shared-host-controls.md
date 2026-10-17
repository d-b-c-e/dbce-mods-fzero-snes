# Optional Shared save-state browser and rewind bindings

This source slice adds dedicated wheel buttons for the existing save-state browser
and rewind browser during ordinary Shared play. It uses the existing extras tick,
host handlers, panel loop and transactional controls writer. It opens no additional
device reader and changes no force calculation, ABI or default setting.

## Contract

Both `[Input] SharedExtras=1` and `SharedHostControls=1` must be selected explicitly.
The original four-slot declaration remains the default and retains its exact
`c4ffc4c85f1b7f72437e5871bdec57f9d5b997a62a9fdd3fd2208fefa0d98fb1` hash.
The optional declaration preserves X/Y/L/R in slots 0-3 and appends:

```text
fzero-snes.saveMenu digital
fzero-snes.rewind digital
```

Its exact ASCII declaration, including the final newline, hashes to
`761789a517b35b88395d6b3ba06e2d2ba60628168abda6637e69d581a7a51cf7`.
The receipt adds `extras.optIn` with `section=Input`, `key=SharedHostControls`,
`value=1`, `default=0` (all strings) and `appliesAfterRestart=true`.

`tools/stage_unified.py --private-host-controls` and installer
`-PrivateHostControls` require their existing private Shared switch, the verified
extras-capable five-file inventory, and the six-slot declaration plus opt-in
markers in the linked consumer. The installer refuses to withdraw an existing
host-controls capability during an ordinary upgrade; rollback remains available.
The public route and old private receipt remain unchanged by default.

The bounded Wheelkit successor adds receipt admission, the six-slot identity and
host-only Apply/clear/history support in the two FZero configure files. It requires
both saved flags and verifies the selected declaration against the linked image.
It retains the existing no-force Apply gate and BF645 dependency pin. Adoption of
the separate 83DD/EFA9 force pair by the configuration helper is a different slice.
Lola owns the common dispatch/import/standalone caller integration, including
`IdentityFor(gameControlPreset)` for host-only presets.

## Host behavior

Only Live, non-injected digital values can set host bits 24/25. The shared edge
helper strips these before guest processing and prevents a held button reopening
a panel. SaveMenu opens the browser; it does not immediately save or load a slot.
The existing panel loop cancels pending Shared input before polling Core for UI
navigation, silences force, freezes the guest, and resumes with a new simulation
sample after closing. Overlay reads mask host bits. Recording requests and
diagnostic/capture mode refuse the opt-in; replay/scripts do not open the backend.
No recording contains host bits. Native button indices and handlers remain intact.

## Qualification scope

Pending the queued affected batch: three existing fake-only CMake targets, nine
selected CTests, existing package and synthetic installer fixtures, and the
FZeroShimControlTests class plus Lola's focused common dispatch check. The source
runner has an explicit `-HostControlsOnly` scope and retains per-step process and
lease receipts. The previous 52/52 Shared-owner and native six-step qualifications
remain separate; they are not repeated for this change.

No graphical host rebuild, game launch, owner install, physical button/force test,
runtime normalization or public release is claimed. The ready Deluxe candidate,
paired dependency candidate, existing pins, saves and owner settings remain intact.
All 37 toolkit ledger IDs remain recorded. STD-019 remains unchecked;
STD-021/031 physical normalization and STD-037 schema 2 remain pending.
