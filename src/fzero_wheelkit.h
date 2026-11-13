#pragma once

/* The Wheelkit runtime (wheelkit.dll, loaded beside the program at run time): a
 * steering wheel and pedals as player 1's controller, the F6 settings panel,
 * dashboard telemetry and force feedback. Built only with -DWHEELKIT_SDK=<checkout>; without it,
 * or without the DLL beside the program, every call does nothing. */

#include <stdbool.h>
#include <stdint.h>

#ifdef FZERO_WHEELKIT
/* Once, with config.ini's path, before the window is created. */
void FzeroWheelkitStart(const char *config_path, const char *version);
/* True when the panel needs the OpenGL presenter. */
bool FzeroWheelkitWantsGl(void);
/* Once per simulated frame: player 1's input word with the wheel applied, and
 * the frame's telemetry and force from WRAM. */
uint32_t FzeroWheelkitFrame(uint32_t input, const uint8_t *ram);
/* Just before the OpenGL buffer swap, with the drawable's size. */
void FzeroWheelkitPresent(int width, int height);
/* Before the OpenGL context is destroyed. */
void FzeroWheelkitStop(void);
/* Three screens, from the rig profile (wk_triples_get): the borderless window over the three (x, y, width, height in
 * desktop pixels), or false for one screen. */
bool FzeroWheelkitSpan(int span[4]);
/* The centre screen within a drawable that many pixels wide: its width and left edge (the drawable and 0 on one
 * screen). The game's picture, its HUD and the menus are drawn there. */
int FzeroWheelkitCentreWidth(int drawable_width);
int FzeroWheelkitCentreX(int drawable_width);
/* In a race on three screens: the two side screens' pictures (w x h each, BGRA) and where they go in the drawable
 * ({x, y, w, h} from the top left), given where the game's picture was drawn on the centre screen. False outside a
 * race or on one screen: the sides stay black. */
bool FzeroWheelkitSides(int x, int y, int w, int h, int drawable_width, int drawable_height, int logical_width,
                        const uint32_t *pixels[2], int *width, int *height, int rects[2][4]);
#else
static inline void FzeroWheelkitStart(const char *c, const char *v) { (void)c; (void)v; }
static inline bool FzeroWheelkitWantsGl(void) { return false; }
static inline uint32_t FzeroWheelkitFrame(uint32_t input, const uint8_t *ram) { (void)ram; return input; }
static inline void FzeroWheelkitPresent(int w, int h) { (void)w; (void)h; }
static inline void FzeroWheelkitStop(void) {}
static inline bool FzeroWheelkitSpan(int s[4]) { (void)s; return false; }
static inline int FzeroWheelkitCentreWidth(int w) { return w; }
static inline int FzeroWheelkitCentreX(int w) { (void)w; return 0; }
static inline bool FzeroWheelkitSides(int x, int y, int w, int h, int dw, int dh, int lw, const uint32_t *p[2],
                                      int *pw, int *ph, int r[2][4]) {
  (void)x; (void)y; (void)w; (void)h; (void)dw; (void)dh; (void)lw; (void)p; (void)pw; (void)ph; (void)r;
  return false;
}
#endif
