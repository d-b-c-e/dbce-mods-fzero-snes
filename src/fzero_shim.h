#pragma once
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Environment opt-in: FZERO_SHIM_DIAGNOSTIC_RUN names a fresh absolute run
 * folder. Prepare runs before any launcher/force initialization and latches
 * force off even when admission fails. Normal launches do nothing here. */
int FzeroShimPrepare(const char *config, int explicit_rom_without_launcher,
                     int recorded_replay, int scripted_replay, int explicit_recording);
int FzeroShimRequested(void);
int FzeroShimLatched(void);
int FzeroShimCompleted(void);
const char *FzeroShimSaveRoot(void);
/* Called only after the host has disabled save/debug callbacks and selected
 * the diagnostic save root. Start recomputes output latches independently. */
void FzeroShimHostFences(const char *selected_save_root, int debug_disabled);
const char *FzeroShimRecordingPath(void);
int FzeroShimStart(void *window, const char *legacy_sdl_guid);
int FzeroShimInput(uint32_t *word);
int FzeroShimConsumed(uint64_t simulation_frame, uint32_t input, const uint8_t *ram, size_t ram_size);
void FzeroShimNeutral(void);
int FzeroShimStop(void);
#ifdef __cplusplus
}
#endif
