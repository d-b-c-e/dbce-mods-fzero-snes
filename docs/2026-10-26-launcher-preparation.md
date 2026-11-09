# Virtual consumer preparation only

Follow-up to a9738d5 and Archie DBC-330 comment
84d85325-aa60-4ed4-8bc1-5b6b21e589e2. Helper-window, dynapi, Steam metadata
and driver isolation retain their reviewed source verdict. This change closes
the preparation portion of the remaining child-environment prerequisite.

`tools/sdl-virtual-only/prepare.py` reuses Admit.cmake to verify the retained
dependency source/config/archive and runtimeAllowed=false. The separate
approved-preparation.json pins the consumer after the hat-bound correction and
its exact link-provenance bytes. A mismatched executable or provenance refuses
before allocating output. These are retained artifact identities, not upstream
authenticity or a new independently proven link relationship.

The route allocates a fresh scratch child beneath an explicitly supplied parent.
It writes a data-only specification and exact executable bytes under
consumer.exe.disabled, outside an empty disposable cwd. It builds an environment
from scratch: LOCALAPPDATA, APPDATA, TEMP, TMP and USERPROFILE point inside that
child; DBCE_FFB_MUTE=1 is the sole fixture override. No parent variable is copied,
so SDL/FZERO/Steam names in any casing, mapping-file hints, VID/PID @file hints,
PATH and unrelated credentials are absent. No SystemRoot or other OS variables
are admitted yet. This environment is reviewed preparation data, not runtime
compatibility evidence. There is no launch method, command, add_test or runtime
hook. The CMake admission utility runs; the consumer never does.

Example (select an existing disposable scratch parent):

```powershell
python tools/sdl-virtual-only/prepare.py --sdl-dir .local/sdl-reviewed/build --consumer-dir .local/sdl-admission-consumer --output-parent .local --cmake "C:/Program Files/CMake/bin/cmake.exe"
```

Focused check: `python tools/sdl-virtual-only/check_preparation.py` PASS.
It calls the actual route with mixed-case poisoned inherited variables, checks
the exact environment allowlist, disposable paths, inert payload bytes and
disabled runtime flag, and refuses changed consumer and link-provenance bytes
before staging. TemporaryDirectory cleanup completed. No passed compiler or
fixture suite was rerun; no consumer, game, physical device, input, desktop,
live config/save/install or public push operation occurred. Local CPU/file I/O
only, no GPU/window/focus impact or rig lease. Parent environment restored.

Next unfinished milestone: source review of this preparation boundary, then
separately design/admit any eventual launcher. A prepared manifest is not an
execution capability: future implementation must revalidate all identities and
the exact environment immediately before execution; preparation cannot close
the check/use interval. runtimeAllowed=false remains mandatory. DBC-65 recovery
and Sandy rig ownership remain separate gates. No runtime authorization claimed.

All STD-001..040 IDs remain represented. STD-019 unchecked; STD-021/031/038
pending; existing partial adoption including STD-037 unchanged. This developer
preparation tool changes no player-facing settings, reader or controls meaning.
