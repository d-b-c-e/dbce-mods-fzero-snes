#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct FzeroFrameSummary {
  int valid, frames;
  double average_fps, low_fps, worst_ms;
} FzeroFrameSummary;
/* Main-thread center-window presentation completions, in monotonic seconds.
 * Side-window swaps and simulation ticks must not call this function. */
void FzeroFrameStatsReset(void);
int FzeroFrameStatsPresented(double now_seconds);
int FzeroFrameStatsGet(FzeroFrameSummary *out);
/* Always writes a bounded caption when out/size are valid; 0 means collecting. */
int FzeroFrameStatsLine(char *out, size_t size, const char *display);
#ifdef __cplusplus
}
#endif
