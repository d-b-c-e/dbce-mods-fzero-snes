# Retained runtime dynamic-load review

DBC-328 follow-up to 9c8ec30. Source/binary inspection only; runtimeAllowed=false.

All three DLL SHA256 values were verified against runtime-candidates.json before
inspection. The adjacent runtime-dynamic-load-evidence.json retains focused
objdump disassembly, target bytes and the toolchain version_info.txt with its
SHA256. Use CMAKE_OBJDUMP from .local/sdl-admission-consumer/CMakeCache.txt:
`objdump -d DLL` for references and `objdump -s --start-address=VA
--stop-address=VA DLL` for the referenced bytes. VAs below use preferred image
bases, not observed runtime addresses. Existing REA availability evidence was
reused; no registration repair or target execution was attempted.

## Concrete loading branch

Each candidate contains the same frame-registration routine at image RVA 0x1450.
libgcc labels it __gcc_register_frame. At RVA 0x1462 it calls
GetModuleHandleA("libgcc_s_dw2-1.dll"); at 0x146e a zero result branches to
0x14e0, selecting local weak frame-registration functions. Only a nonzero
result reaches LoadLibraryA with that same name at 0x1477. GetProcAddress then
resolves __register_frame_info and __deregister_frame_info through the existing
module handle. This is a conditional concrete load site, not evidence that the
DW2 library is required or that it should be copied into this SEH bundle.

Startup wrapper jumps to this routine occur at 0x1e0160270 (libgcc),
0x3bead0ad0 (libstdc++) and 0x2e365c3f0 (winpthread). Their absolute pointers
occur at file offsets 0x23730, 0x1d7898 and 0xc090 respectively. libgcc's
CRT calls __main at 0x1e0141150; __main leads to __do_global_ctors and its
indirect constructor traversal. These observations support startup reachability;
complete constructor traversal for the two stripped candidates and all indirect
call targets remain unproven. No fixture execution is inferred.

## Other bounded resolution sites

winpthread's routine around 0x2e3653e00 resolves Windows APIs through existing
module handles. At 0x2e3653ed5 it calls GetModuleHandleA("kernelbase.dll");
if present, GetProcAddress at 0x2e3653ef7 resolves SetThreadDescription.
This site does not call LoadLibrary. The nearby resolution also includes
GetSystemTimePreciseAsFileTime with a GetSystemTimeAsFileTime fallback.
libstdc++ resolves GetSystemTimePreciseAsFileTime from kernel32.dll at
0x3be96fb9c/0x3be96fbb2 and has the same fallback. Its GetModuleHandleExA
call at 0x3bea690a1 uses flags 6 and an address within the module; it is not
a named LoadLibrary request. Added this previously omitted imported API to
loaderImports; DLL identities and direct DLL imports are unchanged.

The focused scan includes IAT references loaded into registers before indirect
calls. It does not prove all possible indirect calls, function-pointer
propagation, system forwarders, or consumer-to-export reachability. Dynamic
closure remains false. No extra DLL is justified by this evidence.

## External baseline requirements

Retained version_info.txt names gcc-16.2.0-mingw-w64ucrt-14.0.0-r1,
Intel/AMD 64-bit, POSIX threads and UCRT, packaged 2026-08-09. Its stated
runtime prerequisite is Windows 10 or higher, or the Universal CRT update on
older supported systems; it excludes systems older than Windows 7 SP1 /
Server 2008 R2 SP1. This is toolchain metadata, not a consumer compatibility
verdict, a selected host build, or proof that every recorded API-set import
(including crt-private) resolves on that host.

Before integration, a separately assigned baseline outcome must name the intended
Windows version/build and x64 architecture, establish system/API-set/UCRT
resolution for the recorded imports and dynamic lookups, and define owned DLL
search/loading assumptions including whether foreign DW2 modules can be present.
No PATH, SystemRoot, WINDIR or other environment value is derived here.
windowsBaseline stays null. Minimum environment and use-time ownership remain
separate unfinished milestones.

## Validation and disposition

Pinned byte verification passed for all three candidates. Manifest correction
was checked against objdump's import table; focused inert staging/revalidation
and modified-manifest refusal passed. No unchanged compile suite was rerun.
No failed isolation hypothesis or runtime retry occurred; the two-failure
Archie escalation threshold was not reached. Next useful independent milestone:
select and evidence the named Windows/UCRT baseline before preparation integration.

Archie DBC-330 verdict, DBC-65 recovery and Sandy rig ownership are preserved.
No preparation integration, runtime hook, live config/save/install/game/input/
desktop/public push or player meaning change. STD-001..040 are represented;
STD-019 unchecked, STD-021/031/038 pending and existing partial states unchanged.
