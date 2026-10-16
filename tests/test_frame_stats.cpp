#include "fzero_frame_stats.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%d: %s\n", __LINE__, #x); std::exit(1); } } while (0)

int main() {
  FzeroFrameSummary summary{};
  char text[192];
  FzeroFrameStatsReset();
  CHECK(!FzeroFrameStatsGet(&summary) && !summary.valid);
  CHECK(!FzeroFrameStatsLine(text, sizeof(text), "single window") && std::strstr(text, "Collecting"));
  CHECK(!FzeroFrameStatsPresented(0)); // first completion has no interval
  int closed = 0;
  for (int i = 1; i <= 320; ++i) closed += FzeroFrameStatsPresented(i * .03125);
  CHECK(closed == 1 && FzeroFrameStatsGet(&summary));
  CHECK(summary.frames == 320 && std::fabs(summary.average_fps - 32) < .001);
  CHECK(std::fabs(summary.low_fps - 32) < .001 && std::fabs(summary.worst_ms - 31.25) < .001);
  CHECK(FzeroFrameStatsLine(text, sizeof(text), "experimental separate windows"));
  CHECK(std::strstr(text, "32 fps average") && std::strstr(text, "separate windows"));
  CHECK(!std::strstr(text, "triples on"));
  // A paused/loaded epoch must not keep an old valid summary on screen.
  CHECK(!FzeroFrameStatsPresented(16));
  CHECK(!FzeroFrameStatsGet(&summary) && !summary.valid);
  CHECK(!FzeroFrameStatsLine(text, sizeof(text), "single window"));
  // Explicit host pause/resize reset, backward/duplicate times and bad clocks.
  const double invalid_times[] = {16.0, 15.0, NAN, INFINITY, -1.0};
  for (double invalid : invalid_times) {
    FzeroFrameStatsReset(); CHECK(!FzeroFrameStatsPresented(16));
    CHECK(!FzeroFrameStatsPresented(invalid) && !FzeroFrameStatsGet(&summary));
  }
  FzeroFrameStatsReset(); CHECK(!FzeroFrameStatsPresented(0));
  double now = 0;
  for (int i = 0; i < 320; ++i) {
    now += i == 0 ? .125 : .03125;
    FzeroFrameStatsPresented(now);
  }
  CHECK(FzeroFrameStatsGet(&summary));
  CHECK(summary.average_fps < 32 && summary.low_fps < summary.average_fps);
  CHECK(std::fabs(summary.worst_ms - 125) < .001);
  char tiny[2] = {'x','x'};
  CHECK(FzeroFrameStatsLine(tiny, sizeof(tiny), "single window") && tiny[1] == 0);
  CHECK(!FzeroFrameStatsGet(nullptr));
  CHECK(!FzeroFrameStatsLine(nullptr, 0, "single window"));
  FzeroFrameStatsReset(); CHECK(!FzeroFrameStatsGet(&summary));
  std::puts("F-Zero frame statistics adapter: boundaries, low FPS, reset and captions passed");
}
