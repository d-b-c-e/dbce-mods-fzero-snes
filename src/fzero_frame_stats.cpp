#include "fzero_frame_stats.h"
#include "vendor/panel/dbce_frame_stats.hpp"
#include <cmath>

namespace {
dbce::panel::FrameWindow window;
double previous;
bool have_previous;
}
void FzeroFrameStatsReset(void) {
  window = dbce::panel::FrameWindow{};
  previous = 0;
  have_previous = false;
}
int FzeroFrameStatsPresented(double now) {
  if (!std::isfinite(now) || now < 0) { FzeroFrameStatsReset(); return 0; }
  if (!have_previous) { previous = now; have_previous = true; return 0; }
  const double ms = (now - previous) * 1000;
  previous = now;
  // Invalidate the displayed summary too; a paused window is not a fresh result.
  if (!(ms > 0) || ms > dbce::panel::FrameWindow::kGapMs) {
    FzeroFrameStatsReset(); previous = now; have_previous = true; return 0;
  }
  return window.Add(ms);
}
int FzeroFrameStatsGet(FzeroFrameSummary *out) {
  if (!out) return 0;
  const auto &s = window.Last();
  *out = {s.valid ? 1 : 0, s.frames, s.avgFps, s.lowFps, s.worstMs};
  return s.valid;
}
int FzeroFrameStatsLine(char *out, size_t size, const char *display) {
  if (!out || !size) return 0;
  if (!window.Last().valid) {
    std::snprintf(out, size, "Collecting 10 seconds of presentation timings");
    return 0;
  }
  dbce::panel::FormatFrameLine(out, size, window.Last(), display, -1);
  return 1;
}
