# Connected Shared launcher and existing controls setup

## Frozen source inputs

- Game source: isolated `dine/fzero-bounded-profile-20261015`, following
  `02a6261` provider suppression, `1cd8b02` counter and `149d914` bounded read.
- Paired recomp-ui: **217ce4457402555387b1a1d40ed5db75b7c68997**, at
  `E:/Source/games/.worktrees/beni-fzero-shared-launcher-ui-20261015`.
  The game gitlink is advanced to this reviewed source. Both sides must be
  rebuilt together; no old CGameInfo binary compatibility is asserted.
- Standalone helper source: **b8396ecc8c5b20e127c9cabad8f7e79c4ff69bc1**,
  at `E:/Source/toolkits/.worktrees/kiki-fzero-standalone-20261015`.
  This follows `ee71b625` cancellation with the one-line SizeToContent.Height dialog fix.
- The input DLL remains the existing private **BF6455E6** / `ea568c2`,
  base ABI 2, extras ABI 1 and exact C4 X/Y/L/R declaration. No snapshot ABI,
  new binding system, CLR hosting layer or force bridge is required.

**Everything here is source-only.** No compiler, fixture, publisher, package,
game, helper, device, display, install, merge or public push has run. No real
helper payload or manifest hash is invented from its source commit.

## Complete conditional route

1. A builder explicitly supplies a real qualified private portable bundle
   through `FZERO_SETUP_BUNDLE`. Its complete manifest inventory is verified
   at build/package time. Without it, generated `FZERO_SETUP_QUALIFIED=0`
   retains the previous ordinary Shared launcher refusal and no Configure
   action is offered. Recording/preview/isolation still require their previous
   direct-ROM route. Native/direct-ROM/replay behavior keeps its prior route.
2. A matching installed/staged folder has exactly the existing portable
   layout under `controls-setup/`, including `DbceModSetup.exe`, all of its
   dependencies/notices and sibling `bundle-manifest.json`.
3. Runtime eligibility reads only those **two bounded identity files**: the
   manifest (at most 1 MiB) and GUI apphost (at most 8 MiB). Their exact sizes
   and SHA256 must equal the build's qualified metadata. No recursive lookup,
   dependency rehash, JSON parser, environment path or PATH search runs during
   game/launcher startup.
4. Ordinary Shared can enter only this keyboard/mouse launcher route. The
   basic Mods provider suppresses the game's native enumeration/preview and
   Controller saves. Paired UI `disable_native_controllers` suppresses all
   of its SDL controller/sensor/raw/capture/monitor/navigation readers.
   `has_configure_controls` is true only for the matching available helper.
5. **Configure controls** returns result **4** only after UI/platform teardown
   and settings commit. The game rechecks both bounded identities, keeps
   their read handles open without write/delete sharing, obtains its own
   exact PID and `GetProcessTimes` creation FILETIME, and starts the exact
   apphost using `CreateProcessW` with a separate application path and quoted
   argv. No shell command is built. Parent/thread handles are not inherited.
6. The game returns normally before any Core input owner or game HWND opens.
   The helper waits up to **60 seconds** for that exact process to exit before
   SetupWindow/catalog/profile/device work. Its existing running-game guard
   then still protects any other game copy. Cancel, unreadable identity or
   timeout refuses; it never kills or pretends the parent has exited.
7. The helper reuses the existing F-Zero common/C4 editor, capture and checked
   writer. The user saves and relaunches F-Zero normally. If Play is selected
   instead of Configure, the game creates its HWND, rechecks current Shared
   Backend/SharedExtras/force settings and exact profile bytes, then opens
   **one** existing Core owner through `FzeroShimStart`.

The strict child arguments are:

```text
--game f-zero --game-dir ABSOLUTE_GAME_FOLDER
--wait-for-process PID --process-start-filetime UTC_FILETIME_DECIMAL
```

The build manifest must retain schema 1, kind `private-standalone-helper`,
win-x64/self-contained metadata, a 40-hex source commit and the existing exact
files/path/bytes/SHA inventory. It must also declare:

- `supportedGames` includes `f-zero`;
- `waitForProcessContract` is `fzero-exact-process-exit-1`;
- `waitForProcessTimeoutMs` is `60000`.

## Existing build and private stager

`tools/setup_bundle.py` uses the standard Python JSON/hash machinery for
build/package admission. It rejects linked/escaping/duplicate paths, missing
or changed inventory and absent compatibility claims. It emits only approved
source identity plus the two runtime hash/length pairs in the generated
`fzero_setup_identity.h`; it embeds no payload or hundreds-file runtime table.
CMake copies the whole qualified portable directory beside the fresh game
build. Manifest changes trigger reconfiguration.

The existing `tools/stage_unified.py` gains an explicit
`--private-controls-setup`, requiring `--private-shared-input`. It reuses that
same admission module, requires the exact helper and manifest hashes linked
into the game, copies the whole portable folder and records
`privateControlsSetup` in its existing private build receipt. It checks the
portable bundle independently of the game's native PE import closure. The
public packager already refuses the required private Shared receipt.

This does not extend the owner installer or withdraw any previously owned
files. Initial qualification uses the fresh private staged payload. A later
owner installation needs explicit helper-folder inventory/rollback in the
existing installer; no current owner install is promoted by source readiness.

## Authored checks; all execution queued

- `fzero_setup`: production bounded identity/locking/argv/spawn code against
  inert files and a fake CreateProcess callback; Unicode, spaces, terminal
  backslashes and embedded quote round-trips through CommandLineToArgvW;
  mutation, size drift, creation failure and missing identity refuse. No real
  helper/game process is started by this fixture.
- `test_setup_bundle.py`: five focused package/contract/header/linked-pin
  cases. Generated fake fixture pins live only in the test header.
- Existing ordinary Shared host gains `launchersaved`, `launcherempty`,
  `launcherbackendchanged` and `launcherbadextras`, plus retains the original
  unqualified `launcher` refusal and Native/force/replay coverage. Missing
  profile Configure/quit never opens a provider; a subsequent Play reads
  fresh current bytes and cannot fall back to another backend.
- Existing provider spies cover game-side suppression. Counter/video/read
  boundary checks remain in the same upcoming focused batch.
- Carry Beni's reviewed source plus authored SDL2/3 input-policy, C API result
  and actual-model checks. Carry Kiki's exact-process/cancellation, existing
  F-Zero C4/importer/writer and locked/private GUI checks. Do not repeat a
  broad toolkit/native/force baseline.

## One later named batch

Use the existing shared lease/process helpers. First build/test the affected
helper source as documented in its guide, and publish its existing portable
bundle once to fresh output. Preserve actual source/manifest/helper hashes.
Then use that actual stage path in the game's prepared Release/Ninja route:

```text
-DRECOMP_UI_ROOT=E:/Source/games/.worktrees/beni-fzero-shared-launcher-ui-20261015
-DFZERO_SETUP_BUNDLE=<actual verified portable publish stage>
```

Build the desktop/provider targets and, for the existing private unified
stager only, its required `FZeroSNESRecompHeadless` target. The headless target
is compiled, never launched. Stage once with both explicit private flags.
The no-bundle path remains useful for the ROM-free fake fixtures. The earlier
counter build guide lists the current 16-case game subset and five Python
cases. Neither that recipe nor this document is a runtime grant.

A later named muted observation must establish the rendered keyboard/mouse
launcher, no SDL input readers, Configure exit-before-edit, helper wait/normal
guard, saved profile relaunch and one Core owner after its game HWND. Source,
compiled bytes and fake/process identity results are separate evidence.

Native launch semantics follow Microsoft's [CreateProcessW contract](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw)
and [Windows argument quoting rules](https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?view=msvc-170).
