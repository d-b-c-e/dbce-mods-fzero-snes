# Matching private stock host-controls package

The stock private package now includes a standalone helper that exposes F-Zero's
declared X/Y/L/R, Save-state browser and Rewind bindings. Game build
`46ef8c3e31ec9ef1a23e027750df143b4063a1e6` embeds the actual manifest and apphost
hashes of delivery-qualified helper `b695fe39bcdfd1dbfebaca99b3eabfe036bb056c`.
The package uses the existing private `stage_unified.py` route with explicit
Shared input, host controls and controls setup options. No old candidate was
overwritten, and no native dependency was rebuilt.

Private ZIP:

`E:/fzero-research/host-controls-package-nemo-20261017/run-02/delivery/FZeroSNESRecomp-private-host-controls-46ef8c3e31ec-helper-b695fe39bcdf-win-x64.zip`

SHA-256: `5EAD2FC7EF2BC6DEC62CD8F6A8C5E82218E4549D36906D57D80BB4562D3D3C92`.
This is a stock-only private candidate. It contains neither a ROM nor the embedded
Deluxe module. It is not an owner installation or a public release.

## Actual helper contract

The earlier f115 helper contained the provider ancestry but omitted F-Zero from
its standalone game list, import route and delivery manifest. It was not adopted.
Lola restored the existing controls-only `f-zero` route, six-action import/editor
scope, exact process-exit wait, and public Preview/Apply/Verify behavior. The helper
waits for the launcher's exact PID and creation FILETIME, for at most 60,000 ms,
before opening setup state. Report capability is not advertised in this successor;
the game consequently compiles that optional route disabled.

The current common manifest uses `{id, scopes}` records. Package admission accepts
an explicit `f-zero` controls scope while retaining the legacy string-list route,
mandatory exact-exit contract and timeout, full file inventory verification, and
exact embedded helper hashes. Ambiguous or duplicate rows and unknown scopes
refuse. An actual package-size mismatch was also corrected in the helper: the
stock receipt inventory bound now admits a complete helper bundle while retaining
the 256 KiB receipt bound and duplicate/path/hash checks. Its public fixture uses
530 helper-shaped inventory members, not a compiled game claim.

The packaged helper manifest hashes to
`C9C285BC2798180E02691982BF3B471F4F53B3A10842217F9536D4FBB7E03BDC`;
its apphost hashes to
`84A582B79CE96D380498876F56BB876B7277BF17C7BB26CC6380B14C3D454ADE`.
Both exact values are linked into the new graphical consumer. The unchanged
native runtimes remain WheelFfb `3F6CC514` and private extras-capable shim `BF6455E6`;
this package does not adopt the separate force bridge dependency pair.

## Checks and retained failures

Lola's combined helper evidence is in portfolio hub
`artifacts/standalone-controls-successor-lola-20261017`: all 16 new cases passed
across retained runs, followed by one App/GUI+CLI publication and six delivery
checks. The original CS0122 compile failure and two fixture failures remain there.
Compiled production source is `218799257e619ed8706d047540469f14e99d0da9`; delivery
source `b695fe39` changes only the corrected fixtures.

Game/package evidence is under
`E:/fzero-research/host-controls-package-nemo-20261017`:

| Run | Result |
| --- | --- |
| run-01 | Three new scoped-manifest tests passed. Configure passed. The initial build failed because the headless shared owner lacked C++17 for the existing controls header. No package was created. Safe release is recorded. |
| run-02 | One-line target-feature fix, then incremental graphical and headless builds passed. Private staging verified source/dependency pins, stock-only consumer, six-action capability, exact linked helper, and runtime import closure. All 577 ZIP members were read back and matched the actual payload. The packaged root `controls-setup/Start-ModSetup.ps1 -Game f-zero -GameDir <payload> -PlanOnly` returned verified controls-only support with the exact helper source. |

The three manifest passes were retained without repetition. Previous native
52/6, game/package 3/1, installer 47 and Wheelkit 61 results remain separate and
were not rerun. No game, GUI, input provider, device, force, socket or owner install
was run. C++ compiler macro-redefinition warnings remain distinct from the passing
build result; no compiler errors remain.

Run-02 closed every scoped child, recorded a complete safe process query, changed
no parent environment, and released its exact lease at
`2026-10-17T03:00:54.6259491Z`. Nade received the direct handoff before documentation.
`host-controls-package-qualification.json` binds receipt hashes and artifact paths.

## Use and remaining acceptance

The existing opt-ins remain required in this copy's `config.ini`:

```ini
[Input]
Backend=Shared
SharedExtras=1
SharedHostControls=1

[ForceFeedback]
Enabled=0
```

Close the game before running `controls-setup/Start-ModSetup.ps1 -Game f-zero
-GameDir <extracted folder>`. Read installed controls, edit the separate local
draft, save, preview and explicitly Apply. The helper preserves saved settings and
does not silently enable either host-controls flag. Recorded/diagnostic requests
remain separate from these ordinary host actions.

Actual game startup, rendered setup/buttons, physical response and owner acceptance
remain untested for this candidate. The older ready Deluxe package and its
authentic install receipt remain intact. Deluxe portable admission is a distinct
remaining compatibility gap; this stock receipt does not claim it. STD-019 remains
unchecked; STD-021/031 physical normalization and STD-037 schema 2 remain pending.
