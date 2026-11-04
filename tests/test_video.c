#include "fzero_video.h"
#include "fzero_replay.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

/* Do not use assert: Release builds must execute all validation. */
#define CHECK(expr) do { if (!(expr)) { \
  fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); \
} } while (0)

static void viewport_tests(void) {
  FzeroVideoSettings settings;
  FzeroVideoDefaults(&settings); /* shipped defaults: every mod on, Fit, Auto rate */
  CHECK(settings.enhanced && settings.aspect == FZERO_ASPECT_FIT && settings.fps == 0 &&
        settings.fps_enabled && settings.bs_deluxe && !settings.show_fps);
  CHECK(!settings.hd_mode7 && settings.hd_scale == 2 && !settings.diagnostics &&
        !settings.triple_screen &&
        settings.triple_output_mode == FZERO_TRIPLE_OUTPUT_SPAN);
  CHECK(FzeroTripleValidLayout(&settings) && settings.triple_panel_width_mm == 708 &&
        settings.triple_eye_distance_mm == 660 && settings.triple_left_yaw_deg == 70 &&
        settings.triple_right_yaw_deg == 70 && settings.triple_bezel_gap_mm == 8);
  CHECK(FzeroTripleSpanSupported(7680, 1440));
  CHECK(FzeroTripleSpanSupported(5760, 1080));
  CHECK(FzeroTripleSpanSupported(3840, 720));
  CHECK(!FzeroTripleSpanSupported(1920, 1080));
  CHECK(!FzeroTripleSpanSupported(5120, 1440));
  CHECK(!FzeroTripleSpanSupported(7681, 1440));
  FzeroVideoStock(&settings);
  CHECK(!settings.enhanced && settings.aspect == FZERO_ASPECT_STOCK && !settings.fps_enabled && !settings.bs_deluxe);
  FzeroViewport v = FzeroCalculateViewport(&settings, 5120, 1440);
  CHECK(!v.enhanced && v.width == 256 && v.extra == 0);
  settings.enhanced = true;
  const int expected[] = {256, 342, 448, 682};
  for (int i = 0; i < 4; ++i) {
    settings.aspect = (FzeroAspect)i;
    v = FzeroCalculateViewport(&settings, 800, 600);
    CHECK(v.width == expected[i]);
    CHECK(v.width == 256 + 2 * v.extra);
    CHECK(FzeroHudAnchorX(v, 8, -1) == 8);
    CHECK(FzeroHudAnchorX(v, 248, 1) == v.width - 8);
    CHECK(FzeroHudAnchorX(v, 128, 0) == v.width / 2);
  }
  settings.aspect = FZERO_ASPECT_FIT;
  v = FzeroCalculateViewport(&settings, 5120, 1440);
  CHECK(v.width == 682);
  CHECK(FzeroCalculateViewport(&settings, 10000, 100).width == 682);
  CHECK(FzeroCalculateViewport(&settings, 600, 900).width == 256);
  CHECK(!FzeroCalculateViewport(&settings, 800, 600).enhanced);
  CHECK(FzeroCalculateViewport(&settings, 0, 0).width == 256);
  CHECK(FzeroCalculateViewport(&settings, 1920, 1080).width == 342);
  FzeroRect r = FzeroDestination(v, 1920, 1080);
  CHECK(r.w == 1920 && r.h == 540 && r.y == 270);
  r = FzeroDestination(v, 0, 0);
  CHECK(r.w == 0 && r.h == 0);
  settings.enhanced = false;
  r = FzeroDestination(FzeroCalculateViewport(&settings, 1920, 1080), 1920, 1080);
  CHECK(r.w == 1440 && r.h == 1080 && r.x == 240);
}

static void clock_tests(void) {
  const unsigned rates[] = {30, 60, 90, 120, 144, 165, 240, 360};
  uint64_t expected_frames = (uint64_t)ceil(600 * FZERO_SIMULATION_HZ);
  for (unsigned i = 0; i < sizeof(rates) / sizeof(*rates); ++i) {
    FzeroClock c;
    FzeroClockReset(&c, 0, rates[i]);
    /* Ten minutes of independent simulation/display events, including
     * display rates below the simulation rate and non-integer ratios. */
    double now;
    while ((now = FzeroClockNextDeadline(&c)) < 600) {
      if (FzeroClockSimulationDue(&c, now)) FzeroClockSimulationDone(&c);
      if (FzeroClockPresentationDue(&c, now)) {
        CHECK(FzeroClockAlpha(&c, now) >= 0 && FzeroClockAlpha(&c, now) <= 1);
        FzeroClockPresentationDone(&c, now);
      }
    }
    CHECK(c.simulation_frames == expected_frames);
    CHECK(llabs((long long)c.presentations - 600LL * rates[i]) <= 1);
    CHECK(c.missed_presentations == 0);
  }
  FzeroClock c;
  FzeroClockReset(&c, 10, 240);
  FzeroClockSimulationDone(&c);
  double simulation_deadline = c.next_simulation;
  FzeroClockPresentationDone(&c, 10.05);
  CHECK(c.missed_presentations >= 11);
  CHECK(c.next_presentation > 10.05);
  CHECK(c.next_simulation == simulation_deadline);
  CHECK(FzeroClockSimulationDue(&c, 10.05));
  FzeroClockReset(&c, 900, 120); /* resume/load discards paused wall time */
  FzeroClockSimulationDone(&c);
  CHECK(!FzeroClockSimulationDue(&c, 900));
  CHECK(fabs(FzeroClockAlpha(&c, 900 + 0.5 / FZERO_SIMULATION_HZ) - 0.5) < 1e-8);
  CHECK(FzeroClockAlpha(&c, 901) == 1);
  CHECK(FzeroClockAlpha(&c, 899) == 0);
  CHECK(FzeroPresentationHz(0, 59.94) == 59.94);
  CHECK(FzeroPresentationHz(0, 500) == 360);
  CHECK(FzeroPresentationHz(0, NAN) == 60);
  CHECK(FzeroPresentationHz(0, 0) == 60);
  CHECK(FzeroPresentationHz(144, 60) == 144);
}

static int temporary_file_is(const char *path, const char *expected) {
  char got[2048];
  FILE *file = fopen(path, "rb");
  if (!file) return 0;
  size_t count = fread(got, 1, sizeof(got) - 1, file);
  int ok = !ferror(file) && feof(file);
  fclose(file); got[count] = 0;
  return ok && !strcmp(got, expected);
}

static void temporary_tests(void) {
  /* First save in this process: occupy candidate 1 as well as the legacy temp.
   * Both writers must reserve their own name instead of overwriting either. */
  const char *path = "test-video-temporary.ini";
  const char *legacy = "test-video-temporary.ini.tmp";
  char occupied[128], released[128];
#ifdef _WIN32
  unsigned long process = GetCurrentProcessId();
#else
  unsigned long process = (unsigned long)getpid();
#endif
  snprintf(occupied, sizeof(occupied), "%s.tmp.%lu.1", path, process);
  snprintf(released, sizeof(released), "%s.tmp.%lu.2", path, process);
  FILE *file = fopen(legacy, "wb");
  CHECK(file && fputs("legacy marker", file) >= 0 && !fclose(file));
  file = fopen(occupied, "wb");
  CHECK(file && fputs("occupied marker", file) >= 0 && !fclose(file));
  FzeroVideoSettings settings, back;
  FzeroVideoDefaults(&settings);
  CHECK(FzeroVideoSave(&settings, path));
  CHECK(temporary_file_is(legacy, "legacy marker"));
  CHECK(temporary_file_is(occupied, "occupied marker"));
  CHECK(FzeroVideoLoad(&back, path) && !back.show_fps);
  file = fopen(released, "rb");
  CHECK(!file); /* Successful replacement consumed only our candidate 2. */
  const char *const key[] = {"ShowFPS"}, *const on[] = {"1"};
  CHECK(FzeroIniSaveKeys(path, "FZeroVideo", 1, key, on));
  CHECK(temporary_file_is(legacy, "legacy marker") && temporary_file_is(occupied, "occupied marker"));
  CHECK(FzeroVideoLoad(&back, path) && back.show_fps);
#ifdef _WIN32
  char original[2048];
  file = fopen(path, "rb"); CHECK(file);
  size_t count = fread(original, 1, sizeof(original) - 1, file);
  CHECK(!ferror(file) && feof(file) && !fclose(file)); original[count] = 0;
  CHECK(SetFileAttributesA(path, FILE_ATTRIBUTE_READONLY));
  CHECK(!FzeroVideoSave(&settings, path));
  CHECK(!FzeroIniSaveKeys(path, "FZeroVideo", 1, key, on));
  CHECK(temporary_file_is(path, original));
  CHECK(temporary_file_is(legacy, "legacy marker") && temporary_file_is(occupied, "occupied marker"));
  char pattern[128]; snprintf(pattern, sizeof(pattern), "%s.tmp.%lu.*", path, process);
  WIN32_FIND_DATAA found;
  HANDLE search = FindFirstFileA(pattern, &found); CHECK(search != INVALID_HANDLE_VALUE);
  do { CHECK(!strcmp(found.cFileName, occupied)); } while (FindNextFileA(search, &found));
  CHECK(GetLastError() == ERROR_NO_MORE_FILES && FindClose(search));
  CHECK(SetFileAttributesA(path, FILE_ATTRIBUTE_NORMAL));
#endif
  CHECK(remove(path) == 0 && remove(legacy) == 0 && remove(occupied) == 0);
}

static void config_tests(void) {
  FzeroVideoSettings a, b;
  FzeroVideoDefaults(&a);
  a.enhanced = true; a.aspect = FZERO_ASPECT_FIT; a.fps = 165;
  a.triple_screen = true;
  a.triple_output_mode = FZERO_TRIPLE_OUTPUT_SEPARATE;
  a.triple_panel_width_mm = 620;
  a.triple_eye_distance_mm = 780;
  a.triple_left_yaw_deg = 55;
  a.triple_right_yaw_deg = 65;
  a.triple_bezel_gap_mm = 12;
  a.triple_eye_height_mm = 45;
  a.reduce_crash_flash = true;
  a.show_fps = true;
  CHECK(FzeroVideoSave(&a, "test-video.ini"));
  CHECK(FzeroVideoLoad(&b, "test-video.ini"));
  CHECK(b.enhanced && b.aspect == FZERO_ASPECT_FIT && b.fps == 165 &&
        b.triple_screen && b.reduce_crash_flash && b.show_fps &&
        b.triple_output_mode == FZERO_TRIPLE_OUTPUT_SEPARATE);
  CHECK(FzeroTripleValidLayout(&b) && b.triple_panel_width_mm == 620 &&
        b.triple_eye_distance_mm == 780 && b.triple_left_yaw_deg == 55 &&
        b.triple_right_yaw_deg == 65 && b.triple_bezel_gap_mm == 12 &&
        b.triple_eye_height_mm == 45 && b.triple_curve_radius_mm == 0);
  /* Curve radius: saved and read back; 0 or 300..20000 and at least half the
   * width, with the eye in front of the curved glass center. */
  a.triple_curve_radius_mm = 1500;
  CHECK(FzeroVideoSave(&a, "test-video.ini"));
  CHECK(FzeroVideoLoad(&b, "test-video.ini"));
  CHECK(b.triple_curve_radius_mm == 1500 && FzeroTripleValidLayout(&b));
  b.triple_curve_radius_mm = 299; CHECK(!FzeroTripleValidLayout(&b));
  b.triple_curve_radius_mm = 309; CHECK(!FzeroTripleValidLayout(&b)); /* 620 mm chord needs R >= 310 */
  b.triple_curve_radius_mm = 310; b.triple_eye_distance_mm = 310; CHECK(!FzeroTripleValidLayout(&b)); /* sag 310 */
  b.triple_eye_distance_mm = 780; CHECK(FzeroTripleValidLayout(&b));
  /* View switching pick: saved and restored with the rest; none is not written;
   * an unknown value is no pick and never refuses the file. */
  for (unsigned pick = 1; pick <= 3; ++pick) {
    a.triple_compare_view = pick;
    CHECK(FzeroVideoSave(&a, "test-video.ini") && FzeroVideoLoad(&b, "test-video.ini"));
    CHECK(b.triple_compare_view == pick && b.triple_curve_radius_mm == 1500 &&
          b.triple_eye_distance_mm == 780 && b.aspect == a.aspect);
  }
  a.triple_compare_view = 0;
  CHECK(FzeroVideoSave(&a, "test-video.ini") && FzeroVideoLoad(&b, "test-video.ini") &&
        b.triple_compare_view == 0);
  {
    const char *bad[] = {"4", "0", "x", "99"};
    for (unsigned i = 0; i < 4; ++i) {
      FILE *f = fopen("test-video.ini", "w");
      CHECK(f && fprintf(f, "[FZeroVideo]\nTripleScreen=1\nTripleCompareView=%s\n", bad[i]) > 0 && !fclose(f));
      CHECK(FzeroVideoLoad(&b, "test-video.ini") && b.triple_compare_view == 0 && b.triple_screen);
    }
  }
  /* F9 writes only its own line: comments, unknown keys and other sections stay
   * byte-exact, in either line ending; a failed replacement keeps the file. */
  {
    static const struct { const char *before, *after2, *after3; } cases[] = {
      {"; mine\n[FZeroVideo]\nTripleScreen=1\nUnknownKey=7\n\n[Other]\nTripleCompareView=9\n",
       "; mine\n[FZeroVideo]\nTripleScreen=1\nUnknownKey=7\n\nTripleCompareView=2\n[Other]\nTripleCompareView=9\n",
       "; mine\n[FZeroVideo]\nTripleScreen=1\nUnknownKey=7\n\nTripleCompareView=3\n[Other]\nTripleCompareView=9\n"},
      {"[FZeroVideo]\r\nTripleCompareView = 1\r\nAspect=Fit\r\n",
       "[FZeroVideo]\r\nTripleCompareView = 2\r\nAspect=Fit\r\n",
       "[FZeroVideo]\r\nTripleCompareView = 3\r\nAspect=Fit\r\n"},
      {"; no section", "; no section\n[FZeroVideo]\nTripleCompareView=2\n",
       "; no section\n[FZeroVideo]\nTripleCompareView=3\n"},
      /* An empty section right before another gets the key, not a second section. */
      {"[FZeroVideo]\n[Other]\nX=1\n", "[FZeroVideo]\nTripleCompareView=2\n[Other]\nX=1\n",
       "[FZeroVideo]\nTripleCompareView=3\n[Other]\nX=1\n"},
      /* Duplicates: the reader takes the last, so the writer replaces the last. */
      {"[FZeroVideo]\nTripleCompareView=1\nTripleCompareView=1\n",
       "[FZeroVideo]\nTripleCompareView=1\nTripleCompareView=2\n",
       "[FZeroVideo]\nTripleCompareView=1\nTripleCompareView=3\n"},
      /* A legacy sectionless key is read; the new line after it wins. */
      {"TripleCompareView=1\n", "TripleCompareView=1\n[FZeroVideo]\nTripleCompareView=2\n",
       "TripleCompareView=1\n[FZeroVideo]\nTripleCompareView=3\n"},
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
      char text[512];
      FILE *f = fopen("test-video-key.ini", "wb");
      CHECK(f && fputs(cases[i].before, f) >= 0 && !fclose(f));
      CHECK(FzeroVideoSaveKey("test-video-key.ini", "TripleCompareView", "2"));
      f = fopen("test-video-key.ini", "rb"); CHECK(f);
      size_t n = fread(text, 1, sizeof(text) - 1, f); fclose(f); text[n] = 0;
      CHECK(!strcmp(text, cases[i].after2));
      CHECK(FzeroVideoLoad(&b, "test-video-key.ini") && b.triple_compare_view == 2);
      CHECK(FzeroVideoSaveKey("test-video-key.ini", "TripleCompareView", "3"));
      f = fopen("test-video-key.ini", "rb"); CHECK(f);
      n = fread(text, 1, sizeof(text) - 1, f); fclose(f); text[n] = 0;
      CHECK(!strcmp(text, cases[i].after3));
      CHECK(FzeroVideoLoad(&b, "test-video-key.ini") && b.triple_compare_view == 3);
    }
    remove("test-video-key.ini");
    CHECK(FzeroVideoSaveKey("test-video-key.ini", "TripleCompareView", "1"));
    CHECK(FzeroVideoLoad(&b, "test-video-key.ini") && b.triple_compare_view == 1);
#ifdef _WIN32
    /* A read-only target refuses the replacement: false, file unchanged, no temp left. */
    CHECK(SetFileAttributesA("test-video-key.ini", FILE_ATTRIBUTE_READONLY));
    CHECK(!FzeroVideoSaveKey("test-video-key.ini", "TripleCompareView", "2"));
    CHECK(FzeroVideoLoad(&b, "test-video-key.ini") && b.triple_compare_view == 1);
    CHECK(GetFileAttributesA("test-video-key.ini.tmp") == INVALID_FILE_ATTRIBUTES);
    CHECK(SetFileAttributesA("test-video-key.ini", FILE_ATTRIBUTE_NORMAL));
#endif
    remove("test-video-key.ini");
  }
  /* The cycle starts from what is shown; the curve step only with a radius. */
  CHECK(FzeroCompareCurrent(0, true) == 3 && FzeroCompareCurrent(0, false) == 2 &&
        FzeroCompareCurrent(3, false) == 2 && FzeroCompareCurrent(1, true) == 1);
  CHECK(FzeroCompareNext(0, true) == 1 && FzeroCompareNext(1, true) == 2 &&
        FzeroCompareNext(2, true) == 3 && FzeroCompareNext(3, true) == 1);
  CHECK(FzeroCompareNext(0, false) == 1 && FzeroCompareNext(2, false) == 1 &&
        FzeroCompareNext(3, false) == 1 && FzeroCompareNext(9, false) == 1);
  CHECK(!strcmp(FzeroCompareName(1), "Original game view") &&
        !strcmp(FzeroCompareName(3), "Angle + curve correction"));
  a.triple_curve_radius_mm = 0;
  a.fps = 0; a.aspect = FZERO_ASPECT_21_9;
  CHECK(FzeroVideoSave(&a, "test-video.ini")); /* atomic replacement */
  CHECK(FzeroVideoLoad(&b, "test-video.ini"));
  CHECK(b.aspect == FZERO_ASPECT_21_9 && b.fps == 0);
  for (unsigned scale = 2; scale <= 10; ++scale) {
    char text[8]; snprintf(text, sizeof(text), "%u", scale);
    CHECK(FzeroParseHdScale(text, &a.hd_scale) && a.hd_scale == scale);
    CHECK(FzeroVideoSave(&a, "test-video.ini"));
    CHECK(FzeroVideoLoad(&b, "test-video.ini") && b.hd_scale == scale);
  }
  const char *invalid[] = {"", "0", "1", "11", "-2", "+4", "4x", "6.5", "10junk", "4294967298", "999999999999999999999999"};
  for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
    unsigned scale = 4;
    CHECK(!FzeroParseHdScale(invalid[i], &scale) && scale == 4);
    FILE *bad = fopen("test-video.ini", "w"); CHECK(bad);
    fprintf(bad, "HDMode7Scale=%s\n", invalid[i]); fclose(bad);
    CHECK(!FzeroVideoLoad(&b, "test-video.ini") && b.hd_scale == 2);
  }
  FILE *f = fopen("test-video.ini", "w");
  CHECK(f);
  fputs("EnhancedRenderer=perhaps\nAspect=99:1\nPresentationFPS=120bad\nHDMode7=bad\nHDMode7Scale=9999\nShowFPS=bad\n", f);
  fclose(f);
  CHECK(!FzeroVideoLoad(&b, "test-video.ini"));
  CHECK(b.enhanced && b.aspect == FZERO_ASPECT_FIT && b.fps == 0); /* invalid fields fall back to shipped defaults */
  CHECK(!b.hd_mode7 && b.hd_scale == 2 && !b.show_fps);
  const char *invalid_triple[] = {
      "TripleOutputMode=Other",
      "TriplePanelWidthMm=0", "TriplePanelWidthMm=3001",
      "TripleEyeDistanceMm=-1", "TripleEyeDistanceMm=3001",
      "TripleLeftAngleDeg=90", "TripleRightAngleDeg=99999999999999999999",
      "TripleBezelGapMm=101", "TripleEyeHeightMm=1001"};
  for (unsigned i = 0; i < sizeof(invalid_triple) / sizeof(*invalid_triple); ++i) {
    f = fopen("test-video.ini", "w"); CHECK(f);
    fprintf(f, "TripleScreen=1\n%s\n", invalid_triple[i]); fclose(f);
    CHECK(!FzeroVideoLoad(&b, "test-video.ini"));
    CHECK(b.triple_screen && FzeroTripleValidLayout(&b));
    CHECK(b.triple_panel_width_mm == 708 && b.triple_eye_distance_mm == 660);
  }
  /* Only absent or invalid fields take a default: a setting the file turned
   * off stays off however corrupt its neighbours are. */
  f = fopen("test-video.ini", "w");
  CHECK(f);
  fputs("EnhancedRenderer=0\nBSDeluxe=0\nAspect=99:1\nPresentationFPS=nope\n", f);
  fclose(f);
  CHECK(!FzeroVideoLoad(&b, "test-video.ini"));
  CHECK(!b.enhanced && !b.bs_deluxe);
  CHECK(!b.show_fps); /* absent key preserves the existing no-counter appearance */
  CHECK(b.aspect == FZERO_ASPECT_FIT && b.fps == 0 && !b.fps_enabled);
  remove("test-video.ini");
  CHECK(FzeroVideoLoad(&b, "test-video.ini"));
  CHECK(b.enhanced && b.fps_enabled && b.bs_deluxe && b.aspect == FZERO_ASPECT_FIT); /* first run: all mods on */
  CHECK(!FzeroValidFps(61));
  FzeroAspect aspect;
  CHECK(FzeroParseAspect("32:9", &aspect) && aspect == FZERO_ASPECT_32_9);
  CHECK(!FzeroParseAspect("Garbage", &aspect));
}

int main(void) {
  temporary_tests();
  FzeroVideoSettings replay;
  FzeroVideoDefaults(&replay);
  CHECK(FzeroReplayConfigure("0:8,10-20:1,15:256", "0:16:9,12:32:9,22:4:3"));
  CHECK(FzeroReplayHasInput());
  CHECK(FzeroReplayInput(0) == 8 && FzeroReplayInput(9) == 0);
  CHECK(FzeroReplayInput(15) == 257 && FzeroReplayInput(20) == 1);
  CHECK(FzeroReplayInput(21) == 0);
  CHECK(FzeroReplayViewport(0, &replay) && replay.enhanced);
  CHECK(!FzeroReplayViewport(1, &replay));
  CHECK(FzeroReplayViewport(12, &replay) && replay.aspect == FZERO_ASPECT_32_9);
  CHECK(FzeroReplayViewport(22, &replay) && !replay.enhanced);
  CHECK(!FzeroReplayConfigure("-1:1", NULL));
  CHECK(!FzeroReplayConfigure(" -1:1", NULL));
  CHECK(!FzeroReplayConfigure("20-10:1", NULL));
  CHECK(!FzeroReplayConfigure("0:4096", NULL));
  CHECK(!FzeroReplayConfigure("0:1,", NULL));
  CHECK(!FzeroReplayConfigure(NULL, "1:16:9,1:32:9"));
  CHECK(!FzeroReplayConfigure(NULL, "1:16:9,"));
  CHECK(FzeroReplayConfigure(NULL, NULL) && !FzeroReplayHasInput());
  int width = 0, height = 0;
  CHECK(FzeroReplayConfigure(NULL, "10:Fit@1280x720,20:Fit@5120x1440"));
  CHECK(!FzeroReplayWindow(9, &width, &height));
  CHECK(FzeroReplayWindow(10, &width, &height) && width == 1280 && height == 720);
  CHECK(FzeroReplayWindow(20, &width, &height) && width == 5120 && height == 1440);
  CHECK(!FzeroReplayConfigure(NULL, "0:Fit@0x720"));
  CHECK(!FzeroReplayConfigure(NULL, "0:Fit@1280x0"));
  viewport_tests();
  clock_tests();
  config_tests();
  puts("F-Zero video: viewport, anchors, clock, and config checks passed");
  return 0;
}
