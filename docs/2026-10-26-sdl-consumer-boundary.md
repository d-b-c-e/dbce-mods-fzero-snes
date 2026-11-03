# DBC-325 SDL consumer qualification boundary

Candidate: ed90714, retaining exact b61e413..0df0e81 composition on
`egon/fzero-cardinal-integration-20261026`. No merge, promotion or public push.

## Result

INCOMPLETE runtime qualification; PASS independent source/compile boundary.
The existing tests/test_gamepad.c route was not executed. Its six physical
backend hints do not prove inert initialization with this cached Windows SDL
3.4.10 build. The compiled build_config/SDL_build_config.h enables DINPUT,
XINPUT, RAWINPUT, WGI, GAMEINPUT and HIDAPI, as well as VIRTUAL.

Exact source prerequisite: SDL_joystick.c registers SDL_WINDOWS_JoystickDriver
whenever DINPUT or XINPUT is compiled. In windows/SDL_windowsjoystick.c,
WINDOWS_JoystickInit (line 332) calls WIN_InitDeviceNotification, creates a
notification window/thread (lines 347-358), sets WindowsDeviceChanged and calls
WINDOWS_JoystickDetect (line 369). Disabling SDL_HINT_JOYSTICK_THREAD still
creates the notification window. Runtime hints disabling individual input
backends therefore do not meet a strict no-device/no-desktop requirement.
No SDL initialization, physical enumeration, window or test execution occurred.

An offline runtime needs a separately built and inspected SDL with only the
virtual joystick driver registered, physical backend code excluded and no
Windows device-notification path reached. That is a test dependency, not a
production reader change. Otherwise this route requires Sandy's existing rig
ownership/admission; this task grants no such access. Before runtime, give the
fixture a disposable working directory and private LOCALAPPDATA (the production
injection helper constructs its command/arm paths there), clear inherited SDL
and FZERO configuration, and verify the exact linked SDL. Do not run against
owner configuration. Process-local SDL virtual joysticks do not install an OS
virtual controller, but that fact alone does not isolate SDL initialization.

## Independent source boundary completed

Extended the existing SDL fixture, without adding a planner suite/framework:
attach an earlier foreign-GUID raw wheel, hold its hat and Start, require zero
selected output, then press Up/Right/Down/Left and RightUp on the selected wheel.
After every press, center requires zero through FzeroGamepadRead,
FzeroGamepadReadFrame(menu) and FzeroGamepadReadOverlay. The decoy remains active
through those assertions. Cleanup closes/detaches both owned virtual wheels.
These assertions are authored and compiled, NOT runtime passing evidence.

Production trace: FzeroGamepadRefresh opens candidates and compares their GUID
to s_preferred before assigning s_raw/load_raw_profile. gamepad_read snapshots
only that selected s_raw; each binding uses recomp_raw_hat_index/value and SDL
hat bit membership. Center has no bits; RightUp activates two cardinal bindings.
No concrete production defect was found. A real fixture compile defect was
found and corrected: Windows SDL_main declares int(int,char**), while the
fixture declared int(void). The fixture now has the required signature.

GCC 16.2 strict compile, exit 0 (no linking or execution):

```powershell
gcc -std=c11 -Wall -Wextra -Werror -DSNESRECOMP_SDL3=1 -DLNG_SDL3=1 -DFZERO_TEST_IDENTITY_PATH -Isrc -IE:/Source/games/dbce-mods-fzero-snes/snesrecomp/runner/src -IE:/Source/games/dbce-mods-fzero-snes/recomp-ui/src/common -IE:/Source/games/dbce-mods-fzero-snes/build-dine-extras-graphical-796cfeb/_deps/sdl3-src/include -c tests/test_gamepad.c -o .local/contract-build/test_gamepad-dbc325.obj
```

Initial compile failed solely on SDL_main's conflicting signature; final compile
passes with warnings treated as errors. Previous 207-vector/1,534-check planner
and decoder results remain inherited evidence and were not repeated.

Impact: compile only; no focus/input, visible window, GPU runtime, physical
device, live config/save, owner-state write or restore. Only local compiler
CPU and scratch object output. No runtime child or rig lease acquired.
DBC-65 preview/Apply/restore remains untouched. No installation, game launch,
input injection, package, public push or changed player meaning. Darlene's
conditional meaning-review gate is not triggered.

Next useful feature: qualify the process-local virtual-only SDL dependency,
then run this existing consumer route with its new selected-wheel/center
assertions. This is independent of DBC-65 and does not expand device support.
Ledger comparison remains STD-001..040 represented; STD-019 unchecked,
STD-021/031 pending and STD-037 partial. No adoption state is promoted.
