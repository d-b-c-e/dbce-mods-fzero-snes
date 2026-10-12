#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fzero_analog.h"
/* The pinned Windows ABI header is kept byte-exact; its import annotation is
 * irrelevant to the ROM-free pure mapper on other build hosts. */
#ifndef _WIN32
#define __declspec(x)
#endif
#include "../vendor/dbce-shim/dbce_shim.h"
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
typedef struct FzeroShimMap {
  FzeroAnalogSteering steering;
  uint64_t last_seq;
  int have_seq, range_percent, response_percent;
  float pedal_threshold;
} FzeroShimMap;

void FzeroShimMapInit(FzeroShimMap *s, int range_percent, int response_percent,
                      float pedal_threshold);
void FzeroShimMapNeutral(FzeroShimMap *s);
/* Called exactly once at each emulated simulation boundary, never on render
 * frames or overlay reads. Duplicate/out-of-order frames refuse, rather than
 * advancing the steering pulse twice. Only finite Live values are delivered. */
int FzeroShimMapFrame(FzeroShimMap *s, const dbce_shim_frame *frame, uint32_t *word);
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
