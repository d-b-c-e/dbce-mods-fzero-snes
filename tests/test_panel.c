#include "fzero_panel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Do not use assert: Release builds must execute all validation. */
#define CHECK(expr) do { if (!(expr)) { \
  fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); exit(1); \
} } while (0)

static int value_is(const FzeroVideoSettings *s, int row, const char *text) {
  char out[48];
  FzeroPanelValue(s, row, out, sizeof(out));
  return !strcmp(out, text);
}

int main(void) {
  FzeroVideoSettings s;
  FzeroVideoDefaults(&s);
  for (int row = 0; row < FZERO_PANEL_ROWS; ++row) CHECK(FzeroPanelLabel(row)[0]);
  CHECK(FzeroPanelNextStart(FZERO_PANEL_TRIPLE) && FzeroPanelNextStart(FZERO_PANEL_OUTPUT) &&
        !FzeroPanelNextStart(FZERO_PANEL_CURVE) && !FzeroPanelNextStart(FZERO_PANEL_ASPECT));

  /* Triple screens and output toggle. */
  CHECK(value_is(&s, FZERO_PANEL_TRIPLE, "Off") && value_is(&s, FZERO_PANEL_OUTPUT, "Surround"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_TRIPLE, +1) && s.triple_screen);
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_OUTPUT, -1) && value_is(&s, FZERO_PANEL_OUTPUT, "Separate monitors"));

  /* Curve presets in order, then stop at the ends; flat is reachable from any radius. */
  CHECK(value_is(&s, FZERO_PANEL_CURVE, "Flat") && !FzeroPanelAdjust(&s, FZERO_PANEL_CURVE, -1));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_CURVE, +1) && value_is(&s, FZERO_PANEL_CURVE, "1000R"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_CURVE, +1) && value_is(&s, FZERO_PANEL_CURVE, "1500R"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_CURVE, +1) && value_is(&s, FZERO_PANEL_CURVE, "1800R"));
  CHECK(!FzeroPanelAdjust(&s, FZERO_PANEL_CURVE, +1) && s.triple_curve_radius_mm == 1800);
  s.triple_curve_radius_mm = 1200;
  CHECK(value_is(&s, FZERO_PANEL_CURVE, "Custom (1200 mm)"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_CURVE, +1) && s.triple_curve_radius_mm == 1500);
  s.triple_curve_radius_mm = 1200;
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_CURVE, -1) && s.triple_curve_radius_mm == 1000);
  /* A preset the rig cannot use is skipped: a 2100 mm wide panel needs R >= 1050. */
  s.triple_panel_width_mm = 2100; s.triple_eye_distance_mm = 2000; s.triple_curve_radius_mm = 0;
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_CURVE, +1) && s.triple_curve_radius_mm == 1500);
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_CURVE, -1) && s.triple_curve_radius_mm == 0);
  FzeroVideoDefaults(&s);

  /* Widescreen and aspect: aspect cycles the launcher's list only while widescreen is on. */
  CHECK(value_is(&s, FZERO_PANEL_ASPECT, "Fit"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_ASPECT, +1) && value_is(&s, FZERO_PANEL_ASPECT, "16:9"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_ASPECT, -1) && value_is(&s, FZERO_PANEL_ASPECT, "Fit"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_WIDESCREEN, +1) && !s.enhanced &&
        value_is(&s, FZERO_PANEL_ASPECT, "4:3") && !FzeroPanelAdjust(&s, FZERO_PANEL_ASPECT, +1));

  /* FPS cycles Auto and the supported rates, wrapping both ways. */
  CHECK(value_is(&s, FZERO_PANEL_FPS, "Auto"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_FPS, -1) && s.fps == 360 && FzeroValidFps(s.fps));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_FPS, +1) && s.fps == 0);
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_SHOW_FPS, +1) && s.show_fps);
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_FLASH, +1) && s.reduce_crash_flash);

  /* Every change survives the existing writer and reader. */
  CHECK(FzeroVideoSave(&s, "test-panel.ini"));
  FzeroVideoSettings back;
  CHECK(FzeroVideoLoad(&back, "test-panel.ini"));
  CHECK(back.show_fps && back.reduce_crash_flash && !back.enhanced && back.fps == 0);
  remove("test-panel.ini");
  puts("F-Zero F6 panel model passed");
  return 0;
}
