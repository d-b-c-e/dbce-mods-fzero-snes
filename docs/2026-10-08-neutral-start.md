# Require neutral FFB startup

Source follow-up to the independent steering change. Initialization now starts
the constant effect with `SetDeviceForcesXY(0, 0)`, without first calling
`StartEffect`, and requires acceptance before creating the auxiliary effects or
starting the watchdog. Refusal performs full consumer shutdown and leaves FFB
inactive until a deliberate new initialization. Existing strict-GUID selection
was already set before enumeration and remains unchanged.

The production-included fake-native lifecycle fixture covers strict selection
before open, neutral startup, refused neutral output, refused device open,
no effect/watchdog creation after refusal, no late frame or silence output, and
successful deliberate retry. Existing concurrent frame/drain/shutdown/unload
checks still pass. Model and lifecycle suites pass after rebuilding the launcher
and headless player. No force model arithmetic, native binary or owner setting
changed.

Negative controls use the old production source from 2c9a043: the new fixture
fails on its `StartEffect` call. A second control removes only that call from
the old source, and fails because refused neutral startup still enables FFB and
its watchdog. These compile and execute fake functions only. Evidence:
`%LOCALAPPDATA%/Dbce/StagePlayback/SessionEvidence/fzero-neutral-start-20261008`.

Not installed or hardware-tested. The separate native publication/identity
successor remains under review; this consumer fix does not certify the old
native DLL's shutdown contract or solve every failure during an active drive.
