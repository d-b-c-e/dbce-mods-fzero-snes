#pragma once

#include "desktop/sdl_compat.h"
#include "fzero_host_buttons.h"

/* Own the gameplay handle independently of the launcher's SDL lifetime. */
void FzeroGamepadConfigure(const char *config, const char *guid, int deadzone);
void FzeroGamepadRefresh(SDL_GameController **pad);
void FzeroGamepadEvent(SDL_GameController **pad, const SDL_Event *event);
typedef struct FzeroGamepadIdentity {
  SDL_JoystickID instance;
  char path[260];
} FzeroGamepadIdentity;
/* SDL3 Windows path of the actual gameplay handle; zero means no exact identity. */
int FzeroGamepadGetIdentity(SDL_GameController *pad, FzeroGamepadIdentity *out);
int FzeroGamepadIdentityStillSelected(SDL_GameController *pad, const FzeroGamepadIdentity *expected);
uint32_t FzeroGamepadRead(SDL_GameController *pad);
/* The guest frame's read. gameplay: FzeroShimGameplayState for the RAM that frame reads (as the Shared route).
 * Outside it a raw wheel's analog steering is off and its pulse phase reset, so a wheel offset cannot pulse a menu
 * choice; bound buttons and hats still navigate. An analog gamepad's stick there is a plain threshold, with its phase
 * reset. Outside a race an analog profile's threshold is never below the player's deadzone (FzeroGamepadConfigure),
 * whatever the profile's own Deadzone. FzeroGamepadRead is this read in gameplay. */
uint32_t FzeroGamepadReadFrame(SDL_GameController *pad, int gameplay);
/* Overlay navigation must not treat a held accelerator/brake as A/B. Steering here is a plain threshold (for an analog
 * profile floored at the player's deadzone) and never advances or resets the race's pulse phase. */
uint32_t FzeroGamepadReadOverlay(SDL_GameController *pad);
/* Digital-only native helper. Not an input-ownership boundary: opening SDL
 * can acquire exclusive access or reset FFB. Shared must not call this helper. */
uint32_t FzeroGamepadReadSupplemental(SDL_GameController *pad, uint32_t mask);
/* Host-only wheel buttons, outside the guest's 12-bit SNES word. */
void FzeroGamepadShutdown(SDL_GameController **pad);
#ifdef FZERO_ORDINARY_R1
/* Explicit synthetic raw source; no SDL device is created/opened. Private diagnostic only. */
int FzeroGamepadDiagnosticStart(const char *config, const char *guid);
void FzeroGamepadDiagnosticSample(const int16_t axes[8], uint32_t buttons);
void FzeroGamepadDiagnosticObserved(double values[3], int canonical[3]);
#endif
