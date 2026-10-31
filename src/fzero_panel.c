#include "fzero_panel.h"

#include <stdio.h>

static const char *const labels[FZERO_PANEL_ROWS] = {
    "Triple screens", "Triple screen output", "Screen curve", "Widescreen",
    "Aspect", "Presentation FPS", "Show FPS", "Reduce crash flashes"};
static const unsigned curves[] = {0, 1000, 1500, 1800};
static const unsigned rates[] = {0, 60, 90, 120, 144, 165, 240, 360};
enum { kCurves = sizeof(curves) / sizeof(*curves), kRates = sizeof(rates) / sizeof(*rates) };

const char *FzeroPanelLabel(int row) {
  return row >= 0 && row < FZERO_PANEL_ROWS ? labels[row] : "";
}

bool FzeroPanelNextStart(int row) {
  return row == FZERO_PANEL_TRIPLE || row == FZERO_PANEL_OUTPUT || row == FZERO_PANEL_FLASH;
}

void FzeroPanelValue(const FzeroVideoSettings *s, int row, char *out, size_t size) {
  const char *on = "On", *off = "Off";
  switch (row) {
  case FZERO_PANEL_TRIPLE: snprintf(out, size, "%s", s->triple_screen ? on : off); break;
  case FZERO_PANEL_OUTPUT:
    snprintf(out, size, "%s", s->triple_output_mode == FZERO_TRIPLE_OUTPUT_SEPARATE ?
             "Separate monitors" : "Surround");
    break;
  case FZERO_PANEL_CURVE: {
    unsigned r = s->triple_curve_radius_mm;
    bool preset = false;
    for (int i = 0; i < kCurves; ++i) preset |= curves[i] == r;
    if (r == 0) snprintf(out, size, "Flat");
    else if (preset) snprintf(out, size, "%uR", r);
    else snprintf(out, size, "Custom (%u mm)", r);
    break;
  }
  case FZERO_PANEL_WIDESCREEN: snprintf(out, size, "%s", s->enhanced ? on : off); break;
  case FZERO_PANEL_ASPECT:
    snprintf(out, size, "%s", s->enhanced ? FzeroAspectName(s->aspect) : "4:3");
    break;
  case FZERO_PANEL_FPS:
    if (s->fps) snprintf(out, size, "%u", s->fps);
    else snprintf(out, size, "Auto");
    break;
  case FZERO_PANEL_SHOW_FPS: snprintf(out, size, "%s", s->show_fps ? on : off); break;
  case FZERO_PANEL_FLASH: snprintf(out, size, "%s", s->reduce_crash_flash ? on : off); break;
  default: if (size) out[0] = 0; break;
  }
}

/* The next preset in a direction; a custom radius steps to the nearest preset that way. */
static unsigned next_curve(unsigned current, int direction) {
  if (direction > 0) {
    for (int i = 0; i < kCurves; ++i) if (curves[i] > current) return curves[i];
    return current;
  }
  for (int i = kCurves - 1; i >= 0; --i) if (curves[i] < current) return curves[i];
  return current;
}

bool FzeroPanelAdjust(FzeroVideoSettings *s, int row, int direction) {
  if (direction == 0) return false;
  switch (row) {
  case FZERO_PANEL_TRIPLE: s->triple_screen = !s->triple_screen; return true;
  case FZERO_PANEL_OUTPUT:
    s->triple_output_mode = s->triple_output_mode == FZERO_TRIPLE_OUTPUT_SEPARATE ?
        FZERO_TRIPLE_OUTPUT_SPAN : FZERO_TRIPLE_OUTPUT_SEPARATE;
    return true;
  case FZERO_PANEL_CURVE: {
    /* The next preset this rig can use (flat always can); none left keeps the radius. */
    unsigned before = s->triple_curve_radius_mm, r = before;
    for (;;) {
      unsigned next = next_curve(r, direction);
      if (next == r) { s->triple_curve_radius_mm = before; return false; }
      s->triple_curve_radius_mm = r = next;
      if (r == 0 || FzeroTripleValidLayout(s)) return true;
    }
  }
  case FZERO_PANEL_WIDESCREEN:
    s->enhanced = !s->enhanced;
    if (s->enhanced && s->aspect == FZERO_ASPECT_STOCK) s->aspect = FZERO_ASPECT_FIT;
    return true;
  case FZERO_PANEL_ASPECT: {
    if (!s->enhanced) return false;
    /* 16:9, 21:9, 32:9, Fit: the launcher's list, wrapping. */
    static const FzeroAspect order[] = {FZERO_ASPECT_16_9, FZERO_ASPECT_21_9, FZERO_ASPECT_32_9, FZERO_ASPECT_FIT};
    int at = 3;
    for (int i = 0; i < 4; ++i) if (order[i] == s->aspect) at = i;
    s->aspect = order[(at + (direction > 0 ? 1 : 3)) % 4];
    return true;
  }
  case FZERO_PANEL_FPS: {
    int at = 0;
    for (int i = 0; i < kRates; ++i) if (rates[i] == s->fps) at = i;
    s->fps = rates[(at + (direction > 0 ? 1 : kRates - 1)) % kRates];
    s->fps_enabled = true;
    return true;
  }
  case FZERO_PANEL_SHOW_FPS: s->show_fps = !s->show_fps; return true;
  case FZERO_PANEL_FLASH: s->reduce_crash_flash = !s->reduce_crash_flash; return true;
  default: return false;
  }
}
