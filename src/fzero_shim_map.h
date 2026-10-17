#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fzero_analog.h"
#include "fzero_host_buttons.h"
/* The pinned Windows ABI header is kept byte-exact; its import annotation is
 * irrelevant to the ROM-free pure mapper on other build hosts. */
#ifndef _WIN32
#define __declspec(x)
#endif
#include "../vendor/dbce-shim/dbce_shim.h"
#include "../vendor/dbce-shim/dbce_shim_extras.h"
#ifndef _WIN32
#undef __declspec
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ABI 2 has no SNES X/Y/L/R or host save/rewind
 * actions; those are deliberately NOT aliased onto unrelated racing actions. */
extern const char *const kFzeroShimActions[DBCE_SHIM_ACTIONS];
extern const char kFzeroShimUnsupported[];
/* Declaration order is also the four scalar slots: X, Y, L, R. No generic
 * action aliases or migration of legacy SDL button indices. */
extern const char kFzeroShimExtrasDeclaration[];
/* Explicit SharedHostControls selects six slots; the first four stay X/Y/L/R. */
extern const char kFzeroShimHostDeclaration[];
int FzeroShimExtrasWordWithHosts(const dbce_shim_frame *frame,
                       const dbce_shim_extras_frame *extras, int host_controls, uint32_t *word);
int FzeroShimExtrasWord(const dbce_shim_frame *frame,
                       const dbce_shim_extras_frame *extras, uint32_t *word);
typedef struct FzeroShimMap {
  FzeroAnalogSteering steering;
  uint64_t last_seq;
  int have_seq, range_percent, response_percent;
  float pedal_threshold;
} FzeroShimMap;

void FzeroShimMapInit(FzeroShimMap *s, int range_percent, int response_percent,
                      float pedal_threshold);
void FzeroShimMapNeutral(FzeroShimMap *s);
/* The same stock/Deluxe race-state boundary used by force and telemetry.
 * Before this boundary, only explicit digital nav may create menu directions. */
int FzeroShimGameplayState(const uint8_t *ram, size_t ram_size);
/* Called exactly once at each emulated simulation boundary, never on render
 * frames or overlay reads. Duplicate/out-of-order frames refuse. Outside the
 * verified race state, analog steering is suppressed and its PDM reset; the
 * independent digital navigation actions remain available. */
int FzeroShimMapFrame(FzeroShimMap *s, const dbce_shim_frame *frame, uint32_t *word,
                      int gameplay);
/* Ordinary host overlays poll digital navigation without advancing the steering
 * PDM or recording a guest frame. Same sequence/layout checks as gameplay. */
int FzeroShimMapOverlayFrame(FzeroShimMap *s, const dbce_shim_frame *frame, uint32_t *word);
/* SDL supplements buttons only. Keep SNES extras/host actions, plus generic
 * buttons explicitly Unbound in Core. Never fall back for missing/faulted
 * bindings; no legacy steering or pedal axes are read by this lane. */
uint32_t FzeroShimLegacyButtonMask(const dbce_shim_frame *frame);
/* The two game channels passed to dbce_shim_record AFTER RtlRunFrame. The input
 * is the final word after host filters, not an earlier adapter preview. */
void FzeroShimConsumedValues(uint64_t simulation_frame, uint32_t input, double out[2]);
/* Read-only post-frame menu evidence. Raw guest state is not a player-driving
 * verdict; a route must also establish its own expected transitions. */
enum { FZERO_SHIM_MENU_VALUES = 12 };
int FzeroShimMenuValues(const uint8_t *ram, size_t ram_size,
                       double out[FZERO_SHIM_MENU_VALUES]);
/* RtlRunFrame clears opposing direction pairs before consumption. Apply that
 * exact reduction before both diagnostic recorders see the final word. */
uint32_t FzeroShimGuestWord(uint32_t input);
/* Pending profile writes must never run on either replay path or the shim
 * path. The Native backend retains its existing startup writer. */
int FzeroControlsStartupMayWrite(int scripted_replay, int recorded_replay, int shim_requested);

#ifdef __cplusplus
}
#endif
