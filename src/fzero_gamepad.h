#pragma once

#include "desktop/sdl_compat.h"

/* Own the gameplay handle independently of the launcher's SDL lifetime. */
void FzeroGamepadConfigure(const char *config, const char *guid, int deadzone);
void FzeroGamepadRefresh(SDL_GameController **pad);
void FzeroGamepadEvent(SDL_GameController **pad, const SDL_Event *event);
uint32_t FzeroGamepadRead(SDL_GameController *pad);
/* Overlay navigation must not treat a held accelerator/brake as A/B. */
uint32_t FzeroGamepadReadOverlay(SDL_GameController *pad);
/* Shared backend compatibility: configured digital buttons/POVs only, no
 * native steering or pedal axes and no dev injection. Mask is Core's current
 * unbound-button policy plus the SNES/host extras missing from ABI2. */
uint32_t FzeroGamepadReadSupplemental(SDL_GameController *pad, uint32_t mask);
/* Host-only wheel buttons, outside the guest's 12-bit SNES word. */
enum { FZERO_WHEEL_SAVE_MENU = 1u << 24, FZERO_WHEEL_REWIND = 1u << 25 };
void FzeroGamepadShutdown(SDL_GameController **pad);
