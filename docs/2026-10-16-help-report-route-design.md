# F-Zero Help issue reporting: reusable builder, missing host handoff

Source-only investigation for request181987, October16. Isolated successor of game48313eb. Frozen game483/UI53 and queued Help build packet808ae1f5 remain unchanged. No implementation, build, test, browser, upload, issue submission, game or owner installation.

## Finding

The existing shared mechanism is reusable and already source-linked into qualified helperb887. Its GUI project references Wheelkit.App and Wheelkit.Core. The public ProblemReportWindow and ProblemReportViewModel use ProblemReport.Prepare, which delegates to portable Dbce.Wheel.Support.SupportReport. No new builder, report window, uploader or transport is needed.

There is no callable F-Zero route today: helper Program accepts setup/wait and smoke modes; the launcher feature provider has links but no report operation; native fzero_setup.cpp launches only the controls contract. A bare issues URL would bypass the shared review flow. Keep the small repository hyperlinks and do not advertise a report action until the missing command/capability wiring exists.

## Smallest implementation slice

1. Add one strict report-only dispatch in the existing standalone GUI, separate from setup/wait parsing. A small selected-mod descriptor carries catalog ID f-zero, fixed repository d-b-c-e/dbce-mods-fzero-snes, bounded active native version/revision, exact game folder and the existing inventory below. No arbitrary repository, executable, passthrough arguments or receipt-provided URL. Open the existing ProblemReportWindow and ProblemReportViewModel, without constructing SetupWindow or opening/saving controls preset stores.
2. Reuse the existing strings and flow: Report a problem; title; What happened, and what did you expect?; optional selected logs; Review report; preview; explicit review checkbox; Open GitHub draft. The existing window also has an explicit Submit to GitHub user action. Opening Help/dialog must not resolve a token, send HTTP, open a browser or upload anything. Shared SupportReport retains redaction, five-log/24KB bounds, immutable preview, 4096-character URL policy and complete local attachment fallback. Its existing instructions disclose that attaching uploads immediately.
3. Add an optional report capability/callback to the existing recomp feature provider and Support/Help renderer. Label the report action Submit GitHub Issue separately from small Mod on GitHub/Wheelkit on GitHub text links. The renderer calls the native host; it does not gain report preparation or native process logic. This descriptor targets the DBCE mod; feature package version1 is not the active application version.
4. Reuse fzero_setup.cpp no-link/hash verification, independently quoted argv and exact CreateProcessW application selection for the report verb. Keep controls exact-exit behavior unchanged. A read-only report window need not close/wait for the launcher or alter input ownership. Add an explicit manifest report contract and separate build-time report-qualified marker. The old b887 wait contract/supportedGames does not imply support for the new command; old bundles leave the action unavailable. A later matched helper/manifest/host rebuild and installer inventory is required before qualification.

## Inventory that exists, and gaps to preserve

- Version: native sdl_main.c kBuildVersion/SNESRECOMP_BUILD_VERSION plus generated source/framework/UI revisions. Diagnostics setup records already contain these values. Catalog mod.version is null; launcher package_get version1 is feature metadata. Pass the active host version, never either placeholder.
- Logs: catalog mod.log and mod.logStart are null, so ModLogReader supplies no default log. Do not invent game.log or imply stderr is persistent. Existing diagnostics/performance-*.jsonl contain hardware/settings/version and exist only when Diagnostics was enabled. Suggest eligible existing diagnostics for explicit selection; never enable Diagnostics or silently include the newest log. Existing Add log supports JSONL and explicit external logs.
- Config inventory: native host resolves config.ini beside the executable and fzero-video.ini beside it or via FZERO_VIDEO_CONFIG; existing isolated sessions can relocate keybinds.ini. Pass the actual resolved inventory, not paths guessed from the game folder. Show known names/presence and build/receipt identity in reviewable context. No automatic config contents. Shared selected-log extensions do not include .ini; do not disguise configs as logs or ZIP the game directory. There is currently no consolidated report descriptor for these native paths: this is thin host wiring still required.
- Receipt: install-receipt.json has sourceCommit/launcherSha256/files[{name,sha256}] and no semantic version. Generic ReceiptModStateReader reads source/version and path members, so it cannot prove this F-Zero inventory was validated. Reuse the F-Zero-specific installed-contract/manifest admission for the exact copy and active native version; do not relax schemas or rewrite receipt/configs for a report.
- Existing App --support-file creates a general Wheelkit support ZIP. It is not the selected-mod review/draft operation and collects a different inventory; do not route Help through it.
- Existing report UI reads bounded selected log excerpts at Review report and freezes the snapshot. Edits invalidate review. Missing/changed/unreadable suggested logs remain normal review errors; no background attachment or log upload.
- The packaged helper is Windows/WPF. The native launcher also supports Linux. Offer the action only with a qualified report-capable helper; a Linux report host remains a separate consumer gap. Do not duplicate the shared builder in C++.

## Focused verification when scheduled

Reuse existing ReportLogTests/shared SupportReport tests for redaction/review/browser/attachment behavior and setup launch fixtures for quoting/identity. Narrow additions cover report-only dispatch without controls-store construction, unchanged setup/wait parsing, old/wrong manifest capability unavailable, active version versus feature-version1 and selected-folder identity. Publish one matching helper in its shared slot, then build affected host/UI with a new actual tuple. Visual acceptance must observe correct mod destination, optional logs, preview and invalidation without posting/uploading. No gameplay/force baseline repeat; no such results exist yet.

## Standards

All37 ledger IDs have adoption rows. Pending STD-021/031/037 and unchecked STD-019 remain open. Touched STD-004/030 Help is partial: repository links exist in source; report route requires the shared command/capability/host wiring above and later matching packaging/rendered qualification. No new family-wide standard.
