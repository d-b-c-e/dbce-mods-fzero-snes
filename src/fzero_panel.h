#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "fzero_video.h"

/*
 * The in-game F6 settings panel's model (STD-030): the launcher's video and
 * triple-screen choices a player changes while playing, as rows. No SDL here,
 * so it is tested without a window; sdl_main.c draws it and, when it closes,
 * writes only the edited rows' keys (FzeroPanelKeys, FzeroIniSaveKeys).
 */
typedef enum FzeroPanelRow {
  FZERO_PANEL_TRIPLE,      /* Off / Surround / Separate monitors (STD-022; next start) */
  FZERO_PANEL_CURVE,       /* Flat, 1000R, 1500R, 1800R; any other saved radius shows as Custom */
  FZERO_PANEL_WIDESCREEN,
  FZERO_PANEL_ASPECT,
  FZERO_PANEL_FPS,
  FZERO_PANEL_SHOW_FPS,
  FZERO_PANEL_FLASH,       /* reduce crash flashes (next start) */
  FZERO_PANEL_ROWS
} FzeroPanelRow;

const char *FzeroPanelLabel(int row);
/* The row's current value as shown, e.g. "1500R" or "Custom (1200 mm)". */
void FzeroPanelValue(const FzeroVideoSettings *s, int row, char *out, size_t size);
/* One step left (-1) or right (+1). Returns true when the setting changed; a
 * step that would leave an unusable triple layout is skipped. */
bool FzeroPanelAdjust(FzeroVideoSettings *s, int row, int direction);
/* True for rows whose change takes effect at the next game start. */
bool FzeroPanelNextStart(int row);
/* The fzero-video.ini keys a changed row owns, with their values as
 * FzeroVideoLoad reads them; returns how many (at most 2). Only these are
 * written when the panel closes, so untouched settings stay as they are. */
enum { FZERO_PANEL_MAX_KEYS = 2 };
int FzeroPanelKeys(const FzeroVideoSettings *s, int row,
                   const char *keys[FZERO_PANEL_MAX_KEYS], char values[FZERO_PANEL_MAX_KEYS][16]);
