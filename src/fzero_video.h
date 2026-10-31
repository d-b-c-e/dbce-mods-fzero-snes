#pragma once

#include <stdbool.h>
#include <stdint.h>

enum { FZERO_HEIGHT = 224, FZERO_STOCK_WIDTH = 256, FZERO_MAX_WIDTH = 684 };
enum { FZERO_HD_SCALE_MIN = 2, FZERO_HD_SCALE_MAX = 10 };
#define FZERO_SIMULATION_HZ 60.098811862

typedef enum FzeroAspect {
  FZERO_ASPECT_STOCK,
  FZERO_ASPECT_16_9,
  FZERO_ASPECT_21_9,
  FZERO_ASPECT_32_9,
  FZERO_ASPECT_FIT,
  FZERO_ASPECT_COUNT
} FzeroAspect;

typedef enum FzeroTripleOutputMode {
  FZERO_TRIPLE_OUTPUT_SPAN = 0,
  FZERO_TRIPLE_OUTPUT_SEPARATE = 1
} FzeroTripleOutputMode;

typedef struct FzeroVideoSettings {
  bool enhanced;
  FzeroAspect aspect;
  unsigned fps; /* 0 = display refresh (Auto). */
  bool fps_enabled;
  bool show_fps; /* Optional measured presentation summary; off by default. */
  bool bs_deluxe; /* Launch-time content selection. */
  bool hd_mode7; /* Independent, opt-in spatial resolution enhancement. */
  unsigned hd_scale; /* Integer 2..10; retained while disabled. */
  bool diagnostics; /* Opt-in local performance reports; off by default. */
  bool triple_screen; /* Experimental Surround ground projection. */
  FzeroTripleOutputMode triple_output_mode;
  unsigned triple_panel_width_mm; /* Visible chord width of one panel. */
  unsigned triple_eye_distance_mm; /* Eye to the center panel's glass center (equals its plane when flat). */
  unsigned triple_left_yaw_deg, triple_right_yaw_deg;
  unsigned triple_bezel_gap_mm; /* Physical gap at each panel hinge. */
  unsigned triple_eye_height_mm; /* Eye above panel center; zero at center. */
  unsigned triple_curve_radius_mm; /* 0 = flat; otherwise 300..20000 and at least half the width (1500R = 1500). */
  unsigned triple_compare_view; /* Last view-switching pick: 1 original, 2 angle, 3 angle + curve; 0 or unknown = none. */
  bool reduce_crash_flash; /* Hold the prior presentation over brief white impact frames. */
} FzeroVideoSettings;

typedef struct FzeroViewport {
  int width, extra;
  double aspect;
  bool enhanced;
} FzeroViewport;

typedef struct FzeroRect { int x, y, w, h; } FzeroRect;

void FzeroVideoDefaults(FzeroVideoSettings *settings); /* shipped: Fit; HD Mode 7 opt-in */
void FzeroVideoStock(FzeroVideoSettings *settings);    /* stock 4:3, no mods */
const char *FzeroAspectName(FzeroAspect aspect);
bool FzeroParseAspect(const char *text, FzeroAspect *aspect);
bool FzeroValidFps(unsigned fps);
bool FzeroValidHdScale(unsigned scale);
bool FzeroParseHdScale(const char *text, unsigned *scale);
bool FzeroTripleValidLayout(const FzeroVideoSettings *settings);
bool FzeroTriplePanelSupported(int width, int height);
bool FzeroTripleSpanSupported(int width, int height);
double FzeroPresentationHz(unsigned fps, double refresh);
FzeroViewport FzeroCalculateViewport(const FzeroVideoSettings *settings,
                                     int drawable_width, int drawable_height);
FzeroRect FzeroDestination(FzeroViewport viewport, int width, int height);
int FzeroHudAnchorX(FzeroViewport viewport, int x, int anchor);
bool FzeroVideoLoad(FzeroVideoSettings *settings, const char *path);
bool FzeroVideoSave(const FzeroVideoSettings *settings, const char *path);

/* Monotonic time in seconds. Simulation debt is never discarded here.
 * Hosts explicitly reset on pause/minimize/load, and bound catch-up batches
 * to keep pumping events when a machine cannot sustain the original rate. */
typedef struct FzeroClock {
  double next_simulation, next_presentation, presentation_hz;
  uint64_t simulation_frames, presentations, missed_presentations;
} FzeroClock;
void FzeroClockReset(FzeroClock *clock, double now, double presentation_hz);
bool FzeroClockSimulationDue(const FzeroClock *clock, double now);
void FzeroClockSimulationDone(FzeroClock *clock);
bool FzeroClockPresentationDue(const FzeroClock *clock, double now);
void FzeroClockPresentationDone(FzeroClock *clock, double now);
double FzeroClockAlpha(const FzeroClock *clock, double now);
double FzeroClockNextDeadline(const FzeroClock *clock);

/* View switching (camera policy rev 3): 1 original game view, 2 angle
 * correction, 3 angle + curve correction (only with a curve radius). The step
 * shown now for a saved pick (0 = none follows the settings), the step after
 * one press, and its on-screen name. */
unsigned FzeroCompareCurrent(unsigned pick, bool curve);
/* Writes one Key=value line in place (temp file, then replace): every other
 * line, comment, unknown key and section stays as it is; a missing key goes at
 * the end of [FZeroVideo] (created when absent). False leaves the file as it was. */
bool FzeroVideoSaveKey(const char *path, const char *key, const char *value);
unsigned FzeroCompareNext(unsigned pick, bool curve);
const char *FzeroCompareName(unsigned step);
