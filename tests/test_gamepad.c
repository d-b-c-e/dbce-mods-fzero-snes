#include "fzero_gamepad.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s (%s)\n", __LINE__, #x, SDL_GetError()); exit(1); } } while (0)

#if SNESRECOMP_SDL3
static SDL_JoystickID path_selected, path_decoy;
static int path_mode;
static const char *const selected_path = "FAKE-SELECTED-INTERFACE";
const char *FzeroTestJoystickPath(SDL_Joystick *stick) {
  return path_mode && SDL_JoystickInstanceID(stick) == path_selected ? selected_path : NULL;
}
const char *FzeroTestJoystickPathForID(SDL_JoystickID id) {
  if (!path_mode) return NULL;
  if (id == path_selected || (path_mode == 2 && id == path_decoy)) return selected_path;
  return NULL;
}
static SDL_JoystickID attach(const char *name) {
  SDL_VirtualJoystickDesc desc;
  SDL_INIT_INTERFACE(&desc);
  desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
  desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
  desc.nbuttons = 15;
  desc.name = name;
  SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
  CHECK(id != 0);
  return id;
}

static void pump(SDL_GameController **pad) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) FzeroGamepadEvent(pad, &event);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  /* This fixture owns virtual controllers only. Never enumerate/open the rig's devices. */
  SDL_SetHint(SDL_HINT_JOYSTICK_DIRECTINPUT, "0");
  SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
  SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "0");
  SDL_SetHint(SDL_HINT_JOYSTICK_GAMEINPUT, "0");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
  SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "0");
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
  CHECK(SDL_Init(SDL_INIT_GAMEPAD));
  SDL_JoystickID decoy = attach("unselected controller");
  SDL_JoystickID selected = attach("selected controller");
  path_decoy = decoy; path_selected = selected;
  SDL_Joystick *joy = SDL_OpenJoystick(selected);
  CHECK(joy);
  char guid[40];
  SDL_GUIDToString(SDL_GetJoystickGUID(joy), guid, sizeof(guid));
  FILE *cfg = fopen("gamepad-test.ini", "w");
  CHECK(cfg);
  fprintf(cfg, "[GamepadMap]\nControls = DpadUp,DpadDown,DpadLeft,DpadRight,Back,Start,B,A,Y,X,Lb,Rb\n"
               "[Controller.%s]\nControls = ,DpadDown,DpadLeft,DpadRight,Back,Start,L2,X,Y,A,Lb,Rb\nDeadzone = 3\n", guid);
  fclose(cfg);
  FzeroGamepadConfigure("gamepad-test.ini", guid, 25);
  SDL_GameController *pad = NULL;
  FzeroGamepadRefresh(&pad);
  CHECK(pad && SDL_GetGamepadID(pad) == selected);
  FzeroGamepadIdentity force_identity;
  CHECK(!FzeroGamepadGetIdentity(pad, &force_identity)); /* absent SDL path refuses */
  path_mode = 1;
#if defined(_WIN32)
  CHECK(FzeroGamepadGetIdentity(pad, &force_identity) &&
        force_identity.instance == selected && !strcmp(force_identity.path, selected_path));
  CHECK(FzeroGamepadIdentityStillSelected(pad, &force_identity));
  path_mode = 2;
  FzeroGamepadIdentity duplicate_probe;
  CHECK(!FzeroGamepadGetIdentity(pad, &duplicate_probe) &&
        !FzeroGamepadIdentityStillSelected(pad, &force_identity)); /* duplicate interface refuses */
  path_mode = 1;
  CHECK(FzeroGamepadIdentityStillSelected(pad, &force_identity));
#else
  CHECK(!FzeroGamepadGetIdentity(pad, &force_identity) &&
        !FzeroGamepadIdentityStillSelected(pad, &force_identity)); /* unsupported platform refuses */
#endif
  pump(&pad);
  CHECK(SDL_SetJoystickVirtualButton(joy, SDL_GAMEPAD_BUTTON_WEST, true));
  pump(&pad);
  CHECK(FzeroGamepadRead(pad) == 1u); /* remapped SNES B */
  CHECK(SDL_SetJoystickVirtualButton(joy, SDL_GAMEPAD_BUTTON_WEST, false));
  CHECK(SDL_SetJoystickVirtualButton(joy, SDL_GAMEPAD_BUTTON_DPAD_UP, true));
  pump(&pad);
  CHECK(FzeroGamepadRead(pad) == 0); /* empty first field stays unbound */
  CHECK(SDL_SetJoystickVirtualButton(joy, SDL_GAMEPAD_BUTTON_DPAD_UP, false));
  CHECK(SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_LEFTX, 2000));
  CHECK(SDL_SetJoystickVirtualAxis(joy, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 32767));
  pump(&pad);
  CHECK(FzeroGamepadRead(pad) == (0x0080u | 0x0100u)); /* profile deadzone + L2 */
  CHECK(FzeroGamepadReadSupplemental(pad,0xffffffffu)==0x0100u); // button combo, no stick
  CHECK(FzeroGamepadReadSupplemental(pad,0x0e02u)==0); // bound generic button suppressed
  CHECK(SDL_DetachVirtualJoystick(decoy));
  pump(&pad);
  CHECK(pad && SDL_GetGamepadID(pad) == selected); /* other device removal */
  SDL_CloseJoystick(joy);
  CHECK(SDL_DetachVirtualJoystick(selected));
  pump(&pad);
  CHECK(!pad && FzeroGamepadRead(pad) == 0);
  selected = attach("selected controller");
  path_selected = selected;
  pump(&pad);
  CHECK(pad && SDL_GetGamepadID(pad) == selected); /* same GUID, new instance */
#if defined(_WIN32)
  CHECK(!FzeroGamepadIdentityStillSelected(pad, &force_identity)); /* same path cannot rearm after reconnect */
#else
  CHECK(!FzeroGamepadGetIdentity(pad, &force_identity));
#endif
  /* An opt-in analog pad pulses only in gameplay. In menus its stick is the plain threshold (steady, no pulse edges)
   * and the phase resets, so the next race repeats the same sequence. */
  cfg = fopen("gamepad-test.ini", "w");
  CHECK(cfg);
  fprintf(cfg, "[Controller.%s]\nAnalogSteering = 1\nDeadzone = 3\n", guid);
  fclose(cfg);
  FzeroGamepadShutdown(&pad);
  FzeroGamepadConfigure("gamepad-test.ini", guid, 25);
  FzeroGamepadRefresh(&pad);
  CHECK(pad && SDL_GetGamepadID(pad) == selected);
  SDL_Joystick *stick = SDL_OpenJoystick(selected);
  CHECK(stick);
  CHECK(SDL_SetJoystickVirtualAxis(stick, SDL_GAMEPAD_AXIS_LEFTX, 6000)); /* partial right, past the 3% deadzone */
  pump(&pad);
  uint32_t pad_race[60];
  int pad_pulses = 0;
  /* Past the profile's 3% zone but inside the player's 25%: no menu or overlay direction (a resting offset). */
  CHECK((FzeroGamepadReadFrame(pad, 0) & 0x00c0u) == 0);
  for (int i = 0; i < 60; ++i) { pad_race[i] = FzeroGamepadReadFrame(pad, 1) & 0x00c0u; pad_pulses += pad_race[i] == 0x0080u; }
  CHECK(pad_pulses > 0 && pad_pulses < 60);
  for (int i = 0; i < 60; ++i) CHECK((FzeroGamepadReadFrame(pad, 0) & 0x00c0u) == 0);
  CHECK((FzeroGamepadReadOverlay(pad) & 0x00c0u) == 0);
  for (int i = 0; i < 60; ++i) CHECK((FzeroGamepadReadFrame(pad, 1) & 0x00c0u) == pad_race[i]);
  CHECK(SDL_SetJoystickVirtualAxis(stick, SDL_GAMEPAD_AXIS_LEFTX, 12000)); /* a clear push past 25% still navigates */
  pump(&pad);
  CHECK((FzeroGamepadReadFrame(pad, 0) & 0x00c0u) == 0x0080u);
  CHECK((FzeroGamepadReadOverlay(pad) & 0x00c0u) == 0x0080u);
  SDL_CloseJoystick(stick);
  FzeroGamepadShutdown(&pad);
  CHECK(SDL_DetachVirtualJoystick(selected));

  SDL_VirtualJoystickDesc wheel_desc;
  SDL_INIT_INTERFACE(&wheel_desc);
  wheel_desc.type = SDL_JOYSTICK_TYPE_WHEEL;
  wheel_desc.naxes = 4;
  wheel_desc.nbuttons = 40;
  wheel_desc.nhats = 1;
  wheel_desc.name = "unselected raw wheel";
  SDL_JoystickID raw_decoy_id = SDL_AttachVirtualJoystick(&wheel_desc);
  CHECK(raw_decoy_id != 0);
  SDL_Joystick *raw_decoy = SDL_OpenJoystick(raw_decoy_id);
  CHECK(raw_decoy);
  wheel_desc.name = "raw racing wheel";
  SDL_JoystickID wheel_id = SDL_AttachVirtualJoystick(&wheel_desc);
  CHECK(wheel_id != 0);
  SDL_Joystick *wheel = SDL_OpenJoystick(wheel_id);
  CHECK(wheel);
  SDL_GUIDToString(SDL_GetJoystickGUID(wheel), guid, sizeof(guid));
  cfg = fopen("gamepad-test.ini", "w");
  CHECK(cfg);
  fprintf(cfg, "[Controller.%s]\nAnalogSteering=1\nDeadzone=3\n"
               "SteeringAxis=0\nAcceleratorAxis=2\nBrakeAxis=3\n"
               "ButtonB=1\nButtonX=2\nButtonY=3\nButtonL=4\nButtonR=5\n"
               "ButtonSaveStateMenu=6\nButtonRewind=7\nButtonStart=36\nButtonUp=128\nButtonRight=129\n"
               "ButtonDown=130\nButtonLeft=131\n", guid);
  fclose(cfg);
  FzeroGamepadConfigure("gamepad-test.ini", guid, 25);
  FzeroGamepadRefresh(&pad);
  CHECK(!pad);
  /* Production raw selection must ignore an earlier, active foreign GUID.
   * Exercise each cardinal through the real reader, then release to center. */
  CHECK(SDL_SetJoystickVirtualHat(raw_decoy, 0, SDL_HAT_RIGHTDOWN));
  CHECK(SDL_SetJoystickVirtualButton(raw_decoy, 36, true));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == 0);
  static const struct { Uint8 hat; uint32_t bits; } cardinals[] = {
    {SDL_HAT_UP, 0x0010u}, {SDL_HAT_RIGHT, 0x0080u},
    {SDL_HAT_DOWN, 0x0020u}, {SDL_HAT_LEFT, 0x0040u},
    {SDL_HAT_RIGHTUP, 0x0090u}
  };
  for (unsigned i = 0; i < sizeof(cardinals) / sizeof(cardinals[0]); ++i) {
    CHECK(SDL_SetJoystickVirtualHat(wheel, 0, cardinals[i].hat));
    SDL_UpdateJoysticks();
    CHECK(FzeroGamepadRead(pad) == cardinals[i].bits);
    CHECK(FzeroGamepadReadFrame(pad, 0) == cardinals[i].bits);
    CHECK(FzeroGamepadReadOverlay(pad) == cardinals[i].bits);
    CHECK(SDL_SetJoystickVirtualHat(wheel, 0, SDL_HAT_CENTERED));
    SDL_UpdateJoysticks();
    CHECK(FzeroGamepadRead(pad) == 0);
    CHECK(FzeroGamepadReadFrame(pad, 0) == 0);
    CHECK(FzeroGamepadReadOverlay(pad) == 0);
  }
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 0, 32767));
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 2, 32767));
  CHECK(SDL_SetJoystickVirtualButton(wheel, 36, true));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == (0x0080u | 0x0001u | 0x0008u));
  CHECK(FzeroGamepadReadOverlay(pad) == (0x0080u | 0x0008u));
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 3, 32767));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == (0x0080u | 0x0001u | 0x0002u | 0x0008u));
  CHECK(FzeroGamepadReadOverlay(pad) == (0x0080u | 0x0008u));
  const uint32_t extras=0x03000e02u;
  CHECK(FzeroGamepadReadSupplemental(pad,0xffffffffu)==0x8u); // pedal/steer axes absent
  CHECK(FzeroGamepadReadSupplemental(pad,extras)==0); // mapped Start excluded
  for(int i=2;i<=7;++i) CHECK(SDL_SetJoystickVirtualButton(wheel,i,true));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadReadSupplemental(pad,extras)==extras); // original SNES/host bindings kept
  for(int i=2;i<=7;++i) CHECK(SDL_SetJoystickVirtualButton(wheel,i,false));
  SDL_UpdateJoysticks();
  CHECK(SDL_SetJoystickVirtualButton(wheel, 1, true));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadReadOverlay(pad) == (0x0080u | 0x0001u | 0x0008u));
  CHECK(SDL_SetJoystickVirtualButton(wheel, 1, false));
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 3, 0));
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 0, 0));
  CHECK(SDL_SetJoystickVirtualHat(wheel, 0, SDL_HAT_UP));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == (0x0001u | 0x0008u | 0x0010u));
  CHECK(SDL_SetJoystickVirtualHat(wheel, 0, SDL_HAT_RIGHTUP));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == (0x0001u | 0x0008u | 0x0010u | 0x0080u));
  CHECK(SDL_SetJoystickVirtualHat(wheel, 0, SDL_HAT_DOWN));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == (0x0001u | 0x0008u | 0x0020u));
  CHECK(SDL_SetJoystickVirtualHat(wheel, 0, SDL_HAT_LEFT));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == (0x0001u | 0x0008u | 0x0040u));
  /* Analog steering pulses only in gameplay, as Shared. A menu frame never pulses and resets the phase; bound hats
   * still navigate. Overlay reads use the plain threshold, floored at the player's 25% deadzone, and never move the
   * race's phase. */
  CHECK(SDL_SetJoystickVirtualHat(wheel, 0, SDL_HAT_CENTERED));
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 0, 6000)); /* partial right, past the 3% deadzone */
  SDL_UpdateJoysticks();
  const uint32_t steer = 0x00c0u;
  uint32_t race[60];
  int pulses = 0;
  CHECK((FzeroGamepadReadFrame(pad, 0) & steer) == 0);
  for (int i = 0; i < 60; ++i) { race[i] = FzeroGamepadReadFrame(pad, 1) & steer; pulses += race[i] == 0x0080u; }
  CHECK(pulses > 0 && pulses < 60); /* proportional, not held */
  for (int i = 0; i < 60; ++i) CHECK((FzeroGamepadReadFrame(pad, 0) & steer) == 0);
  for (int i = 0; i < 60; ++i) {
    CHECK((FzeroGamepadReadOverlay(pad) & steer) == 0); /* inside the player's 25% zone: no overlay direction */
    CHECK((FzeroGamepadReadFrame(pad, 1) & steer) == race[i]); /* same phase from the menu reset */
  }
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 0, 12000));
  SDL_UpdateJoysticks();
  CHECK((FzeroGamepadReadOverlay(pad) & steer) == 0x0080u); /* a clear turn still navigates an overlay */
  CHECK((FzeroGamepadReadFrame(pad, 0) & steer) == 0);      /* and the wheel axis still does nothing in a menu */
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 0, 0));
  CHECK(SDL_SetJoystickVirtualHat(wheel, 0, SDL_HAT_RIGHT));
  SDL_UpdateJoysticks();
  CHECK((FzeroGamepadReadFrame(pad, 0) & steer) == 0x0080u);
  CHECK(SDL_SetJoystickVirtualHat(wheel, 0, SDL_HAT_CENTERED));
  FzeroGamepadShutdown(&pad);
  SDL_CloseJoystick(wheel);
  CHECK(SDL_DetachVirtualJoystick(wheel_id));
  SDL_CloseJoystick(raw_decoy);
  CHECK(SDL_DetachVirtualJoystick(raw_decoy_id));
  SDL_Quit();
  remove("gamepad-test.ini");
  puts("F-Zero gamepad: selection, bindings, deadzone, removal and reconnect passed");
  return 0;
}
#endif
