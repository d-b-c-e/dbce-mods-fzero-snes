# Preparation revalidation (source only)

DBC-328 follow-up to 301ee5f. Preparation had no read-only recheck of staged
bytes, environment, paths or membership after staging. `prepare.py --revalidate
PREPARED_ROOT` now reuses the same dependency/consumer admission and expected
specification as preparation. Supply the same explicit --sdl-dir,
--consumer-dir, --output-parent and --cmake arguments.

The route refuses changed dependency/source/config/archive or consumer/link
provenance through existing admission, altered staged payload, manifest fields,
extra environment keys, inherited environment, arguments, runtime flag, path
escapes, unexpected tree members, populated disposable directories, hardlinked
files and symlink/reparse/junction redirects (including ancestor components).
The payload remains consumer.exe.disabled; runtimeAllowed=false. There is no
launch operation, consumer execution, add_test or runtime hook. A pass returns
no execution capability and does not persist an admission token.

This validates the state observed during a source-only check. It does not prove
staging history, directory object identity across replacement, upstream
authenticity or race-free use. An eventual separately admitted launcher must
repeat validation immediately at use and address concurrent mutation; it must
not trust this result as runtime authorization. No such launcher is implemented.

Focused changed-route command: python tools/sdl-virtual-only/check_revalidation.py.
No passed compile or fixture suite is repeated. Local CPU/file I/O and the
existing CMake admission utility only; no consumer/game/device/input/focus/window,
GPU, live config/save/install, desktop or public push operation. Disposable
TemporaryDirectory cleanup owns only test staging; no rig lease needed.

Archie DBC-330 preparation review remains separate. DBC-65 recovery and Sandy
ownership are preserved. Next milestone: review this read-only revalidation
boundary and separately resolve eventual runtime admission/use-time ownership.
No player-facing controls or meaning changed. Ledger STD-001..040 represented;
STD-019 unchecked, STD-021/031/038 pending and existing partial states unchanged.
