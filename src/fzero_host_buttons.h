#pragma once
#include <stdint.h>

/* Host actions never enter either guest input port or its recording. Native
 * and Shared use the same edge/strip path before opening the existing UI. */
enum { FZERO_WHEEL_SAVE_MENU = 1u << 24, FZERO_WHEEL_REWIND = 1u << 25 };
#define FZERO_HOST_BUTTON_MASK (FZERO_WHEEL_SAVE_MENU | FZERO_WHEEL_REWIND)
static inline uint32_t FzeroHostButtonsTake(uint32_t input, uint32_t *held,
                                          int allowed, uint32_t *pressed) {
  uint32_t current = input & FZERO_HOST_BUTTON_MASK;
  *pressed = allowed ? current & ~*held : 0;
  *held = current;
  return input & ~FZERO_HOST_BUTTON_MASK;
}
