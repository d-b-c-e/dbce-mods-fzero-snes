# Retained consumer Windows loader evidence

DBC-328, follow-up to 6aa3119. Source-only outcome; runtimeAllowed=false.

## Observed

Read the retained consumer with the compiler toolchain's GNU objdump -p and -t.
The executable SHA256 matches approved-preparation.json:
F07A2E5A2A2AD711C240A326DCDEBE995FF2994FFCACB9CED4214D24D23B489C.
[Exact import inventory and hashes](2026-10-26-consumer-loader-imports.json)
records all 20 direct imports, zero delay-import directory, and the three
non-system runtime candidates from the compiler's bin directory. The tool path
came from .local/sdl-admission-consumer/CMakeCache.txt, CMAKE_OBJDUMP.
Reproduce with that tool's `-p PATH` option; no target execution is involved.

The non-system names are libgcc_s_seh-1.dll, libstdc++-6.dll and
libwinpthread-1.dll. The candidate libstdc++ imports the other two; libgcc
imports libwinpthread. Their remaining imports include Windows/UCRT API sets.
Candidate identity does not establish which file Windows would resolve.
The present prepare.py writes only the executable, manifest and empty scratch
directories. It supplies no DLLs and no PATH. Thus the prepared output does not
contain a complete application-owned dependency closure. Static SDL linkage
alone does not establish a self-contained executable.

The consumer import table includes LoadLibraryA, LoadLibraryW, LoadLibraryExW
and GetProcAddress. Retained symbols include WIN_IsWindowsVistaOrGreater,
WIN_IsWindows7OrGreater and later version helpers. Copied SDL_windows.c uses
LoadLibraryEx for combase.dll with LOAD_LIBRARY_SEARCH_SYSTEM32 (line 227),
LoadLibrary for ntdll.dll (297, 341), and uxtheme.dll (508).
These source locations and import/symbol presence are bounded evidence, not
proof that every path executes in the fixture. Sources for excluded drivers
also contain loads; merely finding those files is not linked reachability.

## Concrete missing input and policy boundary

Exact executable loader policy is only partially established. Before a future
runtime-admitted launcher, supply a reviewed dependency manifest choosing the
three runtime DLL bytes and their app-local placement (or a separately assigned
static-runtime relink with a new consumer identity). Also establish the intended
Windows host baseline, API-set/system resolution assumptions and the reachable
dynamic-load closure, including calls made by the selected runtime DLLs.
No current evidence pins that closure or demonstrates its use-time ownership.
Do not compensate by adding the compiler directory to PATH or copying inherited
environment values: that would introduce unreviewed dependencies.

The exact minimum fixed environment is unresolved by import tables. The current
six entries remain preparation data: LOCALAPPDATA, APPDATA, TEMP, TMP,
USERPROFILE, DBCE_FFB_MUTE. No evidence here demonstrates a requirement for
SystemRoot, WINDIR, PATH or any additional variable, nor proves the six suffice
for startup. No variable is added based on a guess. This outcome records missing
input as the assignment permits; it does not invent a new admission constraint
or claim a working launcher. Existing runtimeAllowed=false and disabled payload
remain the enforceable boundary. No production source or preparation code changed.

Archie DBC-330's helper/dynapi/Steam/backend verdict remains intact; this work
covers the separate loader evidence only. No compile suite, fixture, game,
input, desktop, live config/save/install or public push ran. Read-only binary
inspection plus local evidence writes only. DBC-65 recovery and Sandy ownership
remain gates. All STD-001..040 are represented; STD-019 unchecked,
STD-021/031/038 pending, existing partial adoptions unchanged.
