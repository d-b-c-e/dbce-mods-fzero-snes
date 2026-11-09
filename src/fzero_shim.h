#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Environment opt-in: FZERO_SHIM_DIAGNOSTIC_RUN names a fresh absolute run
 * folder. Prepare runs before any launcher/force initialization and latches
 * force off even when diagnostic admission fails. Shared input is the default. */
int FzeroShimPrepare(const char *config, int explicit_rom_without_launcher,
                     int recorded_replay, int scripted_replay, int explicit_recording,
                     int qualified_keyboard_mouse_launcher);
int FzeroShimRequested(void);
/* Requested retains its diagnostic meaning. UsingInput includes the ordinary
 * shared input option; it does not imply private saves, mandatory capture,
 * deadline or suppressed keyboard. */
int FzeroShimUsingInput(void);
int FzeroShimOrdinary(void);
/* Presentation capability only: the admitted ordinary handle has the four-slot declaration.
 * This does not certify a binding, live device or physical input response. */
int FzeroShimHasExtras(void);
int FzeroShimHasHostControls(void);
int FzeroShimLatched(void);
int FzeroShimCompleted(void);
const char *FzeroShimSaveRoot(void);
/* For diagnostics only: called after the host has disabled save/debug callbacks and selected
 * the diagnostic save root. Start recomputes output latches independently. */
void FzeroShimHostFences(const char *selected_save_root, int debug_disabled);
const char *FzeroShimRecordingPath(void);
int FzeroShimStart(void *window, const char *legacy_sdl_guid);
int FzeroShimInput(uint32_t *word, const uint8_t *ram, size_t ram_size);
int FzeroShimOverlayInput(uint32_t *word);
/* Legacy SDL supplements stay off; declared extras use Core's existing input owner. */
uint32_t FzeroShimSupplementalMask(void);
int FzeroShimConsumed(uint64_t simulation_frame, uint32_t input, const uint8_t *ram, size_t ram_size);
void FzeroShimNeutral(void);
int FzeroShimStop(void);
#ifdef __cplusplus
}
#endif
