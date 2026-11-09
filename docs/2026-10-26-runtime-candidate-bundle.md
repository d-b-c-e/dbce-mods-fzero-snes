# App-local runtime candidate bundle (source review only)

DBC-328 follow-up to 1a1e3e6. runtimeAllowed=false.

The three retained toolchain DLL identities and direct imports are now pinned in
`tools/sdl-virtual-only/runtime-candidates.json`, reusing the recorded SHA256
inventory. All three import GetModuleHandleA, GetProcAddress and LoadLibraryA;
libstdc++ additionally imports GetModuleHandleExA. GNU objdump -p establishes
these imports and the absent delay-import directory without executing targets.
The tool path is CMAKE_OBJDUMP in the retained consumer CMakeCache.txt.

The static non-system import graph closes over these three candidates:
consumer -> libgcc, libstdc++, libwinpthread; libstdc++ -> libgcc and
libwinpthread; libgcc -> libwinpthread. Remaining direct imports are the
recorded Windows and UCRT/API-set names. This is a bounded static graph, not
Windows loader closure or proof of startup behavior.

Read-only strings inspection also found libgcc_s_dw2-1.dll in libgcc and
libstdc++, and kernelbase.dll in libwinpthread. String presence does not prove
reachable loads. No additional DLL is copied based on these strings. Dynamic
closure, supported Windows version/build/architecture and API-set/UCRT mapping,
DLL search/resolution assumptions and minimum environment remain external
review prerequisites. No Windows baseline, PATH or SystemRoot value is invented.

`runtime_bundle.py --runtime-dir TOOLCHAIN_BIN --output-parent SCRATCH` verifies
all three source files before staging. It writes only SHA-pinned `.dll.disabled`
bytes and the exact manifest in a fresh disposable directory. Proposed eventual
app-local names are recorded in `name`; inert review names in `stagedName`.
`--revalidate BUNDLE --output-parent SCRATCH` checks exact manifest, membership,
bytes, selected parent, regular-file/hardlink status and ancestor redirects.
Both routes reuse prepare.py's digest and redirect checks. Neither loads a DLL,
launches anything, returns runtime permission, changes environment or joins the
consumer preparation tree. This separate bundle can be reviewed before any
future preparation integration. No use-time ownership/race guarantee is claimed.

Focused checks PASS: exact staging/revalidation; changed source and staged bytes
for each of three DLLs refused (six cases); extra loadable DLL refused; changed
runtime authorization refused; wrong parent refused; restored bundle and
TemporaryDirectory cleanup passed. git diff --check passed. No compile suite
repeated or fixture execution. REA tools were absent; the CLI doctor
reported missing native engines and session registration drift. Inspection used the existing
GNU toolchain; no tool configuration changed.

Next distinct feature: review reachable runtime dynamic loads and establish a
named Windows/UCRT baseline before integrating this bundle into consumer
preparation. Minimum fixed environment and eventual use-time ownership remain
separate unfinished milestones. No failed closure hypothesis or runtime retry
was attempted; the two-failure Archie escalation threshold was not reached.
Archie DBC-330 verdict, DBC-65 recovery and Sandy rig ownership remain intact.
No live config/save/install/game/input/desktop/public push or player meaning
change. STD-001..040 remain represented; STD-019 unchecked, STD-021/031/038
pending and existing partial states unchanged.
