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

/* Diagnostic adapter only. ABI 2 has no SNES X/Y/L/R or host save/rewind
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
/* The two game channels passed to dbce_shim_record AFTER RtlRunFrame. The input
 * is the final word after host filters, not an earlier adapter preview. */
void FzeroShimConsumedValues(uint64_t simulation_frame, uint32_t input, double out[2]);
/* Pending profile writes must never run on either replay path or the shim
 * diagnostic path. A normal launch retains the existing startup writer. */
int FzeroControlsStartupMayWrite(int scripted_replay, int recorded_replay, int shim_requested);

#ifdef __cplusplus
}
#endif
