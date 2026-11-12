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
#else
static inline void FzeroWheelkitStart(const char *c, const char *v) { (void)c; (void)v; }
static inline bool FzeroWheelkitWantsGl(void) { return false; }
static inline uint32_t FzeroWheelkitFrame(uint32_t input, const uint8_t *ram) { (void)ram; return input; }
static inline void FzeroWheelkitPresent(int w, int h) { (void)w; (void)h; }
static inline void FzeroWheelkitStop(void) {}
#endif
