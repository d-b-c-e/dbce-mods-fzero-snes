#include "fzero_controls.h"

#include "desktop/sdl_compat.h"

#include <stdio.h>

/* The startup half of fzero_controls.h: find the profile's wheel among the attached joysticks (SDL exposes USB
 * vendor/product, not the DirectInput instance GUID, so two identical wheels cannot be told apart here). */
int FzeroControlsApplyAtStartup(const char *config_path) {
  FzeroControlsPlan plan;
  if (!FzeroControlsPlanFile(config_path, &plan)) return 0;
  if (!plan.ok) {
    fprintf(stderr, "[fzero-controls] [Controls] not applied: %s\n", plan.error);
    return 0;
  }
  if (!FzeroControlsPending(config_path, &plan)) return 0;

#if SNESRECOMP_SDL3
  if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
#else
  if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != 0) {
#endif
    fprintf(stderr, "[fzero-controls] joystick scan unavailable: %s\n", SDL_GetError());
    return 0;
  }
  char guid[40] = {0};
  int matches = 0;
#if SNESRECOMP_SDL3
  int count = 0;
  SDL_JoystickID *ids = SDL_GetJoysticks(&count);
  for (int i = 0; ids && i < count; ++i) {
    if (SDL_GetJoystickVendorForID(ids[i]) != plan.vendor ||
        SDL_GetJoystickProductForID(ids[i]) != plan.product) continue;
    if (!matches++) SDL_GUIDToString(SDL_GetJoystickGUIDForID(ids[i]), guid, sizeof(guid));
  }
  SDL_free(ids);
#else
  for (int i = 0; i < SDL_NumJoysticks(); ++i) {
    if (SDL_JoystickGetDeviceVendor(i) != plan.vendor ||
        SDL_JoystickGetDeviceProduct(i) != plan.product) continue;
    if (!matches++) SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i), guid, sizeof(guid));
  }
#endif
  SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

  if (!matches) {
    fprintf(stderr, "[fzero-controls] profile '%s' revision %s: no attached joystick is %04x:%04x (%s); "
            "retrying at the next start\n", plan.profile, plan.revision, plan.vendor, plan.product, plan.device);
    return 0;
  }
  if (matches > 1)
    fprintf(stderr, "[fzero-controls] %d joysticks are %04x:%04x; using the first\n", matches, plan.vendor,
            plan.product);
  if (!FzeroControlsWrite(config_path, guid, &plan)) {
    fprintf(stderr, "[fzero-controls] could not write %s\n", config_path);
    return 0;
  }
  fprintf(stderr, "[fzero-controls] profile '%s' revision %s applied to %s (%s): %d keys\n", plan.profile,
          plan.revision, guid, plan.device, plan.nkeys);
  for (int i = 0; i < plan.nkeys; ++i)
    fprintf(stderr, "[fzero-controls]   %s = %d\n", plan.keys[i].key, plan.keys[i].value);
  for (int i = 0; i < plan.nnotes; ++i) fprintf(stderr, "[fzero-controls]   not applied: %s\n", plan.notes[i]);
  return 1;
}
