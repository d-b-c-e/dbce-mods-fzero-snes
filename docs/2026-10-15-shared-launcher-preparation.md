# Shared launcher input suppression preparation

Source-only follow-up to coordinator `169489`, after bounded-read/counter
changes `149d914` / `1cd8b02`. **The ordinary Shared explicit-ROM restriction
remains in `FzeroShimPrepare`.** This prepares the game side of a connected
route; it does not admit or advertise an unfinished launcher/helper path.

Historical preparation checkpoint `02a6261`: the subsequent
[connected implementation](2026-10-15-shared-launcher-connected.md) supersedes
the unconditional restriction only when a real matching bundle has been
qualified and is available. No-bundle default retains the refusal. The paired
UI is now pinned to `217ce445`; the helper still has no built artifact here.

## Implemented game-side selection

`FzeroModsConfigureLauncherInput` selects the existing basic presentation
Mods provider for Shared and sets the paired UI's
`RecompLauncherCGameInfo.disable_native_controllers` before `run_window`.
This path never invokes the supplied FFB device-list or SDL steering-preview
callbacks, exposes no legacy wheel/force options or controller selection
callback, and persists only the existing video/mod options. The generic
launcher Controller GUID/source/deadzone save is also skipped for ordinary
Shared. Native uses its existing wheel provider and callbacks, with the
new UI field explicitly zero.

`launcher_steering_axis` independently refuses while Shared/diagnostic input
is selected. The source does not open a second reader or create any Core
owner during launcher initialization. Beni owns the paired UI's platform,
gamepad, raw capture/monitor and navigation suppression.

The existing `fzero_mods` fixture is extended with callback spies that fail
if Shared invokes native enumeration, preview or persistence. It checks the
seven-feature basic provider, omitted wheel/FFB options and selection hook,
video-only commit, then restoration of the nine-feature Native provider and
its preview. These checks are authored, not executed.

## Paired dependency and remaining connection

The existing `5abcff8` recomp-ui header has no such field and is not a build
dependency for this successor. Beni's reviewed source successor must be
frozen and supplied through `RECOMP_UI_ROOT` before compiling these game
changes. CGameInfo has no size/version negotiation; appending the field
requires **both caller and launcher rebuilt from the paired headers/source**.
Zero-initialized rebuilt Native callers retain zero/default behavior. This
is not compatibility with old shorter binary callers.

Kiki's source check found that the current standalone helper rejects F-Zero;
the full Wheelkit already owns BF645 ABI2/extras1/C4 validation and editing.
Kiki owns minimal standalone registration/import/editor reuse. Its intended
strict invocation, only once a matching bundle actually exists, is:

```text
DbceModSetup.exe --game f-zero --game-dir <absolute game folder>
```

The existing running-game guard correctly recognizes the game/launcher/headless
process names. There is no existing parent-wait option. Configure must require
actual launcher exit before the helper opens its controls, or use an explicit
exit followed by manual launch of a genuinely bundled setup entry point.
Neither game nor helper may label the same live PID as absent. No unsupported
external-tool action, arbitrary timed delay or guard exception is added here.

Once that UI/action/bundle path is connected and reviewable, the next game
delta can admit **only that** non-direct ordinary route, defer profile parsing
until the game window exists and recheck current Backend/SharedExtras/force
settings before the existing single `FzeroShimStart`. Diagnostic explicit-ROM,
replay no-owner, Shared force-off and recording gates stay separate.

## Verification scope

Source diff inspection only; no compiler, test, game, helper, device, lease,
installation, merge or public push. The earlier counter-only build recipe at
`docs/2026-10-15-presentation-counter-build.md` remains a recipe for its
`1cd8b02` code scope. For this successor it must use Beni's exact paired
UI source rather than the historical `5abcff8` dependency. The existing
provider case will include these spies when the combined focused batch runs.
