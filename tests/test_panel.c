#include "fzero_panel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

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
  CHECK(FzeroPanelNextStart(FZERO_PANEL_TRIPLE) && FzeroPanelNextStart(FZERO_PANEL_FLASH) &&
        !FzeroPanelNextStart(FZERO_PANEL_CURVE) && !FzeroPanelNextStart(FZERO_PANEL_ASPECT));

  /* One triple-screen row (STD-022): Off, Surround, Separate monitors; Off keeps the output. */
  const char *keys[FZERO_PANEL_MAX_KEYS];
  char values[FZERO_PANEL_MAX_KEYS][16];
  CHECK(value_is(&s, FZERO_PANEL_TRIPLE, "Off"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_TRIPLE, +1) && s.triple_screen && value_is(&s, FZERO_PANEL_TRIPLE, "Surround"));
  CHECK(FzeroPanelKeys(&s, FZERO_PANEL_TRIPLE, keys, values) == 2 && !strcmp(keys[0], "TripleScreen") &&
        !strcmp(values[0], "1") && !strcmp(keys[1], "TripleOutputMode") && !strcmp(values[1], "Span"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_TRIPLE, +1) && value_is(&s, FZERO_PANEL_TRIPLE, "Separate monitors"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_TRIPLE, +1) && !s.triple_screen &&
        s.triple_output_mode == FZERO_TRIPLE_OUTPUT_SEPARATE);
  CHECK(FzeroPanelKeys(&s, FZERO_PANEL_TRIPLE, keys, values) == 1 && !strcmp(values[0], "0"));
  CHECK(FzeroPanelAdjust(&s, FZERO_PANEL_TRIPLE, -1) && value_is(&s, FZERO_PANEL_TRIPLE, "Separate monitors"));

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

  /* Each row's keys read back as the panel showed them. */
  CHECK(FzeroPanelKeys(&s, FZERO_PANEL_FPS, keys, values) == 2 && !strcmp(values[0], "Auto"));
  CHECK(FzeroPanelKeys(&s, FZERO_PANEL_WIDESCREEN, keys, values) == 2 && !strcmp(values[0], "0"));

  /* On disk only the edited rows' keys change: comments, unknown keys, other
   * sections, an unknown F9 pick and untouched settings stay byte-exact. */
  {
    static const char before[] = "; mine\n[FZeroVideo]\nAspect=21:9\nUnknownKey=7\nTripleCompareView=9\n"
                                 "ShowFPS=0\n[Other]\nShowFPS=5\n";
    static const char after[] = "; mine\n[FZeroVideo]\nAspect=21:9\nUnknownKey=7\nTripleCompareView=9\n"
                                "ShowFPS=1\nReduceCrashFlash=1\n[Other]\nShowFPS=5\n";
    FILE *f = fopen("test-panel.ini", "wb");
    CHECK(f && fputs(before, f) >= 0 && !fclose(f));
    const char *edit_keys[4], *edit_values[4];
    int n = FzeroPanelKeys(&s, FZERO_PANEL_SHOW_FPS, keys, values);
    edit_keys[0] = keys[0]; edit_values[0] = values[0];
    char flash[FZERO_PANEL_MAX_KEYS][16];
    const char *flash_keys[FZERO_PANEL_MAX_KEYS];
    n += FzeroPanelKeys(&s, FZERO_PANEL_FLASH, flash_keys, flash);
    edit_keys[1] = flash_keys[0]; edit_values[1] = flash[0];
    CHECK(n == 2 && FzeroIniSaveKeys("test-panel.ini", "FZeroVideo", 2, edit_keys, edit_values));
    char text[256];
    f = fopen("test-panel.ini", "rb"); CHECK(f);
    size_t got = fread(text, 1, sizeof(text) - 1, f); fclose(f); text[got] = 0;
    CHECK(!strcmp(text, after));
    FzeroVideoSettings back;
    FzeroVideoLoad(&back, "test-panel.ini");
    CHECK(back.show_fps && back.reduce_crash_flash && back.aspect == FZERO_ASPECT_21_9 &&
          back.triple_compare_view == 0);
#ifdef _WIN32
    /* A refused replacement writes nothing: all edited keys or none. */
    CHECK(SetFileAttributesA("test-panel.ini", FILE_ATTRIBUTE_READONLY));
    CHECK(!FzeroIniSaveKeys("test-panel.ini", "FZeroVideo", 2, edit_keys, edit_values));
    f = fopen("test-panel.ini", "rb"); CHECK(f);
    got = fread(text, 1, sizeof(text) - 1, f); fclose(f); text[got] = 0;
    CHECK(!strcmp(text, after));
    CHECK(SetFileAttributesA("test-panel.ini", FILE_ATTRIBUTE_NORMAL));
#endif
    remove("test-panel.ini");
  }
  puts("F-Zero F6 panel model passed");
  return 0;
}
