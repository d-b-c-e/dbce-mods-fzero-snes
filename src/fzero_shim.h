#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Optional [Input] Backend=Shared selects ordinary input. Native remains the
 * default. Environment opt-in: FZERO_SHIM_DIAGNOSTIC_RUN names a fresh absolute run
 * folder. Prepare runs before any launcher/force initialization and latches
 * force off even when diagnostic admission fails. Default Native does nothing. */
int FzeroShimPrepare(const char *config, int explicit_rom_without_launcher,
                     int recorded_replay, int scripted_replay, int explicit_recording);
int FzeroShimRequested(void);
/* Requested retains its diagnostic meaning. UsingInput includes the explicit
 * ordinary [Input] Backend=Shared option; it does not imply private saves,
 * mandatory capture, deadline or suppressed keyboard. Native is the default. */
int FzeroShimUsingInput(void);
int FzeroShimOrdinary(void);
int FzeroShimLatched(void);
int FzeroShimCompleted(void);
const char *FzeroShimSaveRoot(void);
/* For diagnostics only: called after the host has disabled save/debug callbacks and selected
 * the diagnostic save root. Start recomputes output latches independently. */
void FzeroShimHostFences(const char *selected_save_root, int debug_disabled);
const char *FzeroShimRecordingPath(void);
int FzeroShimStart(void *window, const char *legacy_sdl_guid);
int FzeroShimInput(uint32_t *word);
int FzeroShimOverlayInput(uint32_t *word);
/* Always zero until extras can share Core's existing input owner. */
uint32_t FzeroShimSupplementalMask(void);
int FzeroShimConsumed(uint64_t simulation_frame, uint32_t input, const uint8_t *ram, size_t ram_size);
void FzeroShimNeutral(void);
int FzeroShimStop(void);
#ifdef __cplusplus
}
#endif
