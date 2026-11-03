# Private F-Zero audio and mute evidence

The qualified ordinary runner requests `SDL_AUDIODRIVER=dummy`; it does not
exclude native SDL audio initialization or playback-device acquisition. The
retained ordinary logs/status do not independently identify the selected audio
driver or count device opens. Physical silence remains unqualified. This note
adds no runtime, audio/device inspection, import scan, build or test result.

## Consumer path and mute

In [sdl_main.c](../src/sdl_main.c), `snesrecomp_sdl_init` still includes
`SDL_INIT_AUDIO` when ordinary R1 is active or replay is selected. The conditional
only removes controller initialization. The SDL3 branch subsequently calls
`SDL_OpenAudioDeviceStream` and obtains its device; a failed open is fatal. The
SDL2 alternative calls `SDL_OpenAudioDevice`. The selected build uses SDL3,
as recorded in its retained configure command.

Audio is paused/resumed according to `launcher_settings.enable_audio` and
suspension. The callback renders guest samples through `RtlRenderAudio` and
submits them with `SDL_PutAudioStreamData`. Volume below100 scales sample values
after rendering; it does not avoid opening the device. Stream destruction closes
the device at shutdown. No ordinary/replay output-mute guard surrounds these
audio-init/open calls or `fill_audio`.

`--mute-outputs` establishes force and telemetry-delivery latches; replay session
status can declare `outputMuted=true`. Those fields do not prove audio exclusion.
Ordinary R1 separately uses a process no-force latch, controller exclusion and
its explicitly qualified loopback telemetry route. Its `forceBlocked` and
`controllers` fields are not audio measurements. Saved audio preference and
global speaker/device state were neither changed nor inspected for this note.

Both [ordinary](../tools/rig/Invoke-FzeroOrdinary.ps1) and
[replay](../tools/rig/Invoke-FzeroReadback.ps1) runners set the child process's
requested SDL driver to `dummy`. That is a launch request, distinct from a
recorded `SDL_GetCurrentAudioDriver()` result. This evidence does not establish
an unconditional source fence against host audio or certify changed/failing paths.

## Exact ordinary evidence

Retained worktree `E:/Source/_work/napo-fzero-readback-20261103`, source/handoff
`d3e5ad7e7b87a449c110334dfaa55640e3ee980b`. Compiler input
`59c5eba52a2e644e86f1a4f2de6aecbc122d1937`; its `src/sdl_main.c` Git blob
`1117586e826b5035891d4ffe61f90058a7462aaf` matches the inspected current source.
All following hashes are SHA-256; no artifact was rebuilt or rewritten.

| Retained item | Identity |
|---|---|
| `.local/ordinary-readback-09/app/FZeroSNESRecomp-wheel-launcher.exe` | `7AEFFF44C63A63D305F5DCAFD4EA229B6186559C8E12A593638DBAA72D133288` |
| `.local/ordinary-readback-09/prepared.json` | `E390F71D3A87AE52437B64A5BBCA93F9B930B9F62A6FF9CC9710A6B7AAD8C87C` |
| `.local/ordinary-readback-09/ready.json` | `F293B0C3E1394DB3920420DD0F74F8BE35A866A33624EF61EC2602FC6FDB8A8C` |
| `.local/ordinary-readback-09/live/result.json` | `8FD6A6409F90E4C3E39DB73CF3CF07A245296770A8C14B3900E17629248FB085` |
| `.local/ordinary-readback-09/live/game.stderr.txt` (916bytes) | `5758C88C7A1C33E8A9A470D3E98F51A25D6374F53410DE4AF98D62E5AD81568B` |
| `.local/ordinary-readback-09/live/game.stdout.txt` (111bytes) | `C53E83805EDA952771FA1DAA6459E0AF31D056DFE54697C554634C02564D9A0D` |
| `E:/Source/temp/fzero/napo-ordinary09/status.json` | `3B1F55B140C448696F2A2C71484C5571349534FAC4177B1C25768DBA91CCF258` |
| `tools/rig/Invoke-FzeroOrdinary.ps1` | `FB001F5901223C84CB94380BFF7C5CC73766B8EA3BE3C6E85C3082514467B175` |

[Ready09 live evidence](E:/Source/_work/napo-fzero-readback-20261103/.local/ordinary-readback-09/live/)
records GO502564's3300 ordinary frames, native exit0, calibrated software controls,
vehicle response, real standalone Apply, F6 panel/save, loopback delivery and
exact owner restoration/release. These accepted results remain as described in
[ordinary R1 qualification](2026-11-03-ordinary-r1.md).

Actual stderr reports no-force/controllers-excluded, telemetry, guest and
panel/presentation progress. Stdout reports keybind and ROM loading. Neither
contains an audio-driver or device-open result. Terminal ordinary status likewise
has no audio field. `fzero_diagnostics.c` contains a selected-driver writer, but
its session runs only when video diagnostics is enabled; that source capability
is not evidence of an emitted record in this qualified interval.

## Boundaries and next evidence

Replay's requested dummy driver and `outputMuted` declaration are separate from
its accepted simulation/capture result in
[provider-free readback](2026-11-03-provider-free-readback.md). This note does not
retroactively qualify every older binary or its backend selection.

No full PE/dependency/audio-surface inventory was generated here. API availability,
driver-selection source and force/input fields do not establish process-wide
audio absence. A future bounded qualification could record the actual selected
driver and successful stream/device identity for its exact candidate. That is
still separate from physical silence or speaker/output measurements. No such
run is requested by this documentation task.

Physical controls/FFB, USB selection, real keyboard reachability, installed
Wheelkit UI, clean game visuals, seating and owner acceptance remain open.
Existing failed/partial results, private binaries/assets and runtime evidence
remain retained. No public push, owner install or settings/default change.
