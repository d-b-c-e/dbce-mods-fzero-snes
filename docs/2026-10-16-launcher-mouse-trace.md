# Launcher Configure mouse diagnostic - source only

Request185827 follows preserved Help attempt05 (hub1099bd10, actual594DF53A / independent1B2F1ACC). Configure was clicked once through documented Sky window-relative coordinates; activation/native foreground passed, immediate refresh stayed in the launcher, and no helper appeared. Cause remains unestablished. No coordinate change, input rewrite or retry follows.

## Existing path

1. The ImGui renderer drains SDL_WaitEventTimeout/SDL_PollEvent. Existing scale_mouse_event converts SDL window coordinates to logical units using platform input_scale. Controller suppression is separate. try_capture can consume an event before ImGui.
2. LNG_ImplSDL_ProcessEvent resolves to ImGui_ImplSDL3_ProcessEvent for this build. Motion queues AddMousePosEvent. Down/up verifies the event window maps to its viewport, then queues AddMouseButtonEvent. A true backend return means recognition, not Configure activation. Focus events queue AddFocusEvent. No vendored backend change.
3. SDL NewFrame and apply_logical_display run before ImGui::NewFrame. On the existing global-mouse path, global position minus SDL window origin divided by input_scale can queue the final logical position. ImGui processes/trickles queued edges. Event coordinates and the final frame MousePos are distinct evidence.
4. draw_external_controls draws Configure once. A true ImGui::Button result assigns LNG_ACTION_CONFIGURE_CONTROLS. The loop returns it; launcher_ng_capi.c maps it to RECOMP_LAUNCHER_RESULT_CONFIGURE_CONTROLS only for the admitted external-control provider.
5. Host sdl_main.c handles that result with the existing exact helper launch. Success logs setup started and returns normally before Core. Failure already logs its reason and displays the existing error. No lifecycle/input rewrite is justified by this trace.

## Opt-in existing log

Only exact child environment LNG_MOUSE_TRACE=1 enables the diagnostic. Other/unset values leave it off. It writes existing stderr, creates no file/writer/persisted setting, and drives no input. A future otherwise-useful combined attempt would pin/rebuild the matching host/UI and opt in only for its owned child; this source change grants no build/runtime slot.

UI logs an enabled marker and up to32 received down/up edges: raw SDL and scaled logical coordinates, event/launcher SDL window IDs, input/display scale, logical/pixel sizes, SDL keyboard/mouse focus, capture-consumed vs ImGui route and backend recognition. Configure logical bounds, final ImGui position/down/click/release/hover/active/pressed/action, focus-lost/capture state are logged once initially and for up to8 Configure draws after each retained edge. This follows down/up arriving in one drain through queued frames. Final backend action/quit and host mapped result are separate. Timestamps are monotonic SDL milliseconds, not UTC; retain actual operator action/state UTC and existing parent native admission for correlation.

SDL focus is labeled precisely; actual OS foreground/HWND/PID remains the existing parent's native proof. No new native focus/activation route or provider-coordinate origin adjustment is introduced.

## Reading a future trace

- Enabled but no edges: no evidence SDL received the click; cause still unknown.
- capture-consumed or wrong event-window/backend recognitionfalse: distinguish capture from ImGui admission.
- Accepted edges but different final position/bounds: investigate recorded transforms, without guessed offsets.
- Bounds/hover/active/pressed over the retained frames distinguish hit-testing from button activation.
- pressed=true/actionConfigure, backend result, host mapped result and existing helper success/error isolate action propagation from helper lifetime.

Source-only checks: scoped diff whitespace passed; SDL/ImGui API shapes inspected in local dependency sources. No build/compiler/executable fixture/provider/native/game/install ran. Matching host/UI rebuild and actual observation remain unrun. Failed05 source/receipts and separate report successor are unchanged.
