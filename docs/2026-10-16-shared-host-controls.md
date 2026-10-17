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

Qualified game runtime/test inputs are commit `937da7e7f85e358a72101f66cc536826c32b8c8a`.
The three existing fake-only targets built and nine selected Release CTests passed
in run-01. The existing package tests passed 3/3 and the dedicated host contract
test passed 1/1 in run-02. After refreshing a stale synthetic receipt, run-03 passed
47 installer checks and all 61 FZeroShimControlTests, including Lola's common
host-only dispatch case, with zero skips. Wheelkit tested source is exact
`c413d9b347beb6ee0cb8815413ef11091581a77a`, descending from the per-game
`2b740838b8a77ee358979740a56ca0ae6d2ad27b` implementation.

Results are retained under `E:/fzero-research/host-controls-nemo-20261016/`:

| Run | Result and corrections |
| --- | --- |
| run-01 | Three target builds and 9/9 CTests pass. Package import failed before tests because the runner omitted `tools` from PYTHONPATH. Safe release recorded. |
| run-02 | Package 3/3 and host 1/1 pass. Installer fixture fails on its old plain-text receipt against the existing JSON-aware rollback helper. Safe release recorded. `pythonPathRestored=false` remains literally false: the null/empty mismatch was confined to this separately launched, now-exited process; no parent/User/Machine environment writer exists. |
| run-03 | Installer 47 and Wheelkit 61/61 pass. PYTHONPATH presence/value restoration is true. Initial cleanup remains `childrenQuiescent=false`, `leaseReleased=false` because the build created an idle compiler server. |
| run-03/compiler-cleanup | Exact PID/creation/path/parent/pipe checked before graceful shutdown of that server. Exit 0; lease released at 2026-10-17T01:33:18.0666299Z. The first cleanup's seeded lineage suffered automatic DateTime string coercion; its defective receipt remains. `exact-identity-audit.json` independently checks every original identity plus the shutdown child using exact ISO strings and a complete CIM snapshot: all original identities absent, query safe, literal lease absent. |

The shutdown uses the [compiler server's explicit pipe and shutdown options](https://github.com/dotnet/roslyn/blob/main/src/Compilers/Server/VBCSCompiler/BuildServerController.cs).
No global build-server shutdown or forced termination was used. The source runner
now passes `UseSharedCompilation=false` for future batches; this recipe-only
prevention was not used to repeat passing checks. Compiler output included two
existing unrelated analyzer warnings and one collection-size assertion warning in
the new dispatch fixture; there were no compile errors or failed tests.

The existing runner's `-HostControlsOnly`, `-HostControlsResumeFrom` and
`-HostControlsPackageResult` scopes retain qualified earlier steps and validate
their relevant tracked inputs before resuming. No passing game/package batch was
repeated. The previous 52/52 Shared-owner and native six-step qualifications remain
separate and were not repeated. See `shared-host-controls-qualification.json` for
receipt hashes and source distinctions.

No graphical host rebuild, game launch, owner install, physical button/force test,
runtime normalization or public release is claimed. The ready Deluxe candidate,
paired dependency candidate, existing pins, saves and owner settings remain intact.
All 37 toolkit ledger IDs remain recorded. STD-019 remains unchecked;
STD-021/031 physical normalization and STD-037 schema 2 remain pending.
