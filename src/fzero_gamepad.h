#pragma once

#include "desktop/sdl_compat.h"
#include "fzero_host_buttons.h"

/* Own the gameplay handle independently of the launcher's SDL lifetime. */
void FzeroGamepadConfigure(const char *config, const char *guid, int deadzone);
void FzeroGamepadRefresh(SDL_GameController **pad);
void FzeroGamepadEvent(SDL_GameController **pad, const SDL_Event *event);
uint32_t FzeroGamepadRead(SDL_GameController *pad);
/* Overlay navigation must not treat a held accelerator/brake as A/B. */
uint32_t FzeroGamepadReadOverlay(SDL_GameController *pad);
/* Digital-only native helper. Not an input-ownership boundary: opening SDL
 * can acquire exclusive access or reset FFB. Shared must not call this helper. */
uint32_t FzeroGamepadReadSupplemental(SDL_GameController *pad, uint32_t mask);
/* Host-only wheel buttons, outside the guest's 12-bit SNES word. */
void FzeroGamepadShutdown(SDL_GameController **pad);
