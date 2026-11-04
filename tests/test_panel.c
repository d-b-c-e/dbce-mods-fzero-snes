#include "fzero_panel.h"
#include "fzero_hotkeys.h"

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

static void write_file(const char *path, const char *text) {
  FILE *f = fopen(path, "wb");
  CHECK(f && fputs(text, f) >= 0 && !fclose(f));
}

static int file_is(const char *path, const char *text) {
  char got[512];
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  size_t n = fread(got, 1, sizeof(got) - 1, f); fclose(f); got[n] = 0;
  return !strcmp(got, text);
}

static int fullscreen(const char *path) {
  int value = 0;
  return FzeroIniReadInt(path, "Graphics", "Fullscreen", &value) && value != 0;
}

static int value_is(const FzeroVideoSettings *s, int row, const char *text) {
  char out[48];
  FzeroPanelValue(s, row, out, sizeof(out));
  return !strcmp(out, text);
}

int main(void) {
  FzeroVideoSettings s;
  FzeroVideoDefaults(&s);
  /* STD-044: the real panel save owns the scalar, not its line's formatting or
   * comment. Mixed endings, the original EOF and other sections stay exact. */
  {
    static const struct { const char *before, *after; } cases[] = {
      {"; mine\r\n[FZeroVideo]\n \tShowFPS \t= \t0 \t; keep me\r\n[Other]\nValue=7",
       "; mine\r\n[FZeroVideo]\n \tShowFPS \t= \t1 \t; keep me\r\n[Other]\nValue=7"},
      {"[FZeroVideo]\r\nShowFPS = 0 \t",
       "[FZeroVideo]\r\nShowFPS = 1 \t"},
      {"\xef\xbb\xbf; UTF-8 comment\n[FZeroVideo]\nShowFPS=0 # note\n",
       "\xef\xbb\xbf; UTF-8 comment\n[FZeroVideo]\nShowFPS=1 # note\n"},
      {"[FZeroVideo]\nShowFPS = 0 ; first stays\nShowFPS\t= 0 \t# last wins\n",
       "[FZeroVideo]\nShowFPS = 0 ; first stays\nShowFPS\t= 1 \t# last wins\n"},
    };
    FzeroVideoSettings chosen = s;
    chosen.show_fps = true;
    for (unsigned i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
      write_file("test-panel-preservation.ini", cases[i].before);
      CHECK(FzeroPanelSave(&chosen, 1u << FZERO_PANEL_SHOW_FPS,
                          "test-panel-preservation.ini", "unused-config.ini", FzeroIniReadInt) == FZERO_PANEL_SAVED);
      CHECK(file_is("test-panel-preservation.ini", cases[i].after));
      FzeroVideoSettings back;
      CHECK(FzeroVideoLoad(&back, "test-panel-preservation.ini") && back.show_fps);
    }
    remove("test-panel-preservation.ini");
    /* Turning triples on also edits the launcher's file through the same writer. */
    chosen.triple_screen = true;
    write_file("test-panel-preservation.ini", "[FZeroVideo]\nTripleScreen=0\n");
    write_file("test-panel-fullscreen.ini", "[Graphics]\r\n \tFullscreen \t= \t0 \t; owner\n[Other]\nX=9");
    CHECK(FzeroPanelSave(&chosen, 1u << FZERO_PANEL_TRIPLE,
                        "test-panel-preservation.ini", "test-panel-fullscreen.ini", FzeroIniReadInt) == FZERO_PANEL_SAVED);
    CHECK(file_is("test-panel-fullscreen.ini", "[Graphics]\r\n \tFullscreen \t= \t1 \t; owner\n[Other]\nX=9"));
    CHECK(fullscreen("test-panel-fullscreen.ini"));
    remove("test-panel-preservation.ini");
    remove("test-panel-fullscreen.ini");
  }
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
  /* Closing the panel: video keys first; turning triples on then makes the
   * game's own reader see Fullscreen on, or reports that it could not. */
  {
    FzeroVideoSettings on;
    FzeroVideoDefaults(&on);
    CHECK(FzeroPanelAdjust(&on, FZERO_PANEL_TRIPLE, +1) && on.triple_screen);
    const unsigned triple = 1u << FZERO_PANEL_TRIPLE;
    /* No Fullscreen yet: written, and read back on. */
    write_file("test-panel-video.ini", "[FZeroVideo]\nAspect=Fit\n");
    write_file("test-panel-config.ini", "; launcher\n[Graphics]\nWindowScale=3\n");
    CHECK(FzeroPanelSave(&on, triple, "test-panel-video.ini", "test-panel-config.ini", FzeroIniReadInt) == FZERO_PANEL_SAVED);
    CHECK(fullscreen("test-panel-config.ini") &&
          file_is("test-panel-config.ini", "; launcher\n[Graphics]\nWindowScale=3\nFullscreen=1\n"));
    CHECK(file_is("test-panel-video.ini", "[FZeroVideo]\nAspect=Fit\nTripleScreen=1\nTripleOutputMode=Span\n"));
    /* Already on (any case, first match): config.ini is not touched. */
    write_file("test-panel-config.ini", "[graphics]\nfullscreen=1\n");
    CHECK(FzeroPanelSave(&on, triple, "test-panel-video.ini", "test-panel-config.ini", FzeroIniReadInt) == FZERO_PANEL_SAVED);
    CHECK(file_is("test-panel-config.ini", "[graphics]\nfullscreen=1\n"));
    /* A spelling the writer does not own still reads off: reported, not claimed. */
    write_file("test-panel-config.ini", "[graphics]\nfullscreen=0\n");
    CHECK(FzeroPanelSave(&on, triple, "test-panel-video.ini", "test-panel-config.ini", FzeroIniReadInt) ==
          FZERO_PANEL_FULLSCREEN_FAILED && !fullscreen("test-panel-config.ini"));
    /* Turning triples off leaves fullscreen alone. */
    FzeroVideoSettings off = on;
    off.triple_screen = false;
    write_file("test-panel-config.ini", "[Graphics]\nFullscreen=0\n");
    CHECK(FzeroPanelSave(&off, triple, "test-panel-video.ini", "test-panel-config.ini", FzeroIniReadInt) == FZERO_PANEL_SAVED);
    CHECK(file_is("test-panel-config.ini", "[Graphics]\nFullscreen=0\n"));
#ifdef _WIN32
    /* The video file refuses: nothing is written anywhere, config.ini included. */
    write_file("test-panel-config.ini", "[Graphics]\nWindowScale=3\n");
    CHECK(SetFileAttributesA("test-panel-video.ini", FILE_ATTRIBUTE_READONLY));
    CHECK(FzeroPanelSave(&on, triple, "test-panel-video.ini", "test-panel-config.ini", FzeroIniReadInt) == FZERO_PANEL_SAVE_FAILED);
    CHECK(file_is("test-panel-config.ini", "[Graphics]\nWindowScale=3\n"));
    CHECK(SetFileAttributesA("test-panel-video.ini", FILE_ATTRIBUTE_NORMAL));
#endif
    remove("test-panel-video.ini");
    remove("test-panel-config.ini");
  }
  puts("F-Zero F6 panel model passed");
  return 0;
}
