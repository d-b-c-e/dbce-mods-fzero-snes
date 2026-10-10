#include "fzero_controls.h"
#include "fzero_gamepad.h"
#include "fzero_hotkeys.h"
#include "fzero_inject.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int s_checks;
#define CHECK(x) do { ++s_checks; if (!(x)) { fprintf(stderr, "%d: %s (%s)\n", __LINE__, #x, SDL_GetError()); exit(1); } } while (0)

/* A rig profile as Wheelkit writes it (synthetic instance GUIDs; the product GUIDs are the R12's and the DS-8X's). */
#define WHEEL "dev={11111111-2222-3333-4444-555555555555} prod={0006346e-0000-0000-0000-504944564944}"
#define SHIFTER "dev={66666666-7777-8888-9999-aaaaaaaaaaaa} prod={05310483-0000-0000-0000-504944564944}"
static const char *const kProfile[] = {
    "Schema = 1",
    "Profile = Test Rig",
    "Revision = rev-1",
    "Transmission = Sequential",
    "steer = axis 0 " WHEEL " range=0..65535 rest=32768 travel=-1 name=\"MOZA R12 Base\"",
    "throttle = axis 2 " WHEEL " range=0..65535 rest=0 travel=+1 name=\"MOZA R12 Base\"",
    "brake = axis 5 " WHEEL " range=0..65535 rest=1743 travel=+1 name=\"MOZA R12 Base\"",
    "clutch = axis 6 " WHEEL " range=0..65535 rest=0 travel=+1 name=\"MOZA R12 Base\"",
    "handbrake = axis 7 " WHEEL " range=0..65535 rest=0 travel=+1 name=\"MOZA R12 Base\"",
    "shiftUp = button 9 " SHIFTER " name=\"DS-8X Shifter\"",
    "confirm = button 31 " WHEEL " name=\"MOZA R12 Base\"",
    "back = button 18 " WHEEL " name=\"MOZA R12 Base\"",
    "start = button 35 " WHEEL " name=\"MOZA R12 Base\"",
    "select = button 22 " WHEEL " name=\"MOZA R12 Base\"",
    "navUp = hat 0 0 " WHEEL " name=\"MOZA R12 Base\"",
    "navDown = hat 0 18000 " WHEEL " name=\"MOZA R12 Base\"",
    "navLeft = hat 0 27000 " WHEEL " name=\"MOZA R12 Base\"",
    "navRight = hat 0 9000 " WHEEL " name=\"MOZA R12 Base\"",
    "camera = button 32 " WHEEL " name=\"MOZA R12 Base\"",
};
enum { kProfileLines = (int)(sizeof(kProfile) / sizeof(kProfile[0])) };

static int key(const FzeroControlsPlan *p, const char *name) {
  for (int i = 0; i < p->nkeys; ++i)
    if (!strcmp(p->keys[i].key, name)) return p->keys[i].value;
  return -1000; /* absent */
}

static int noted(const FzeroControlsPlan *p, const char *prefix) {
  for (int i = 0; i < p->nnotes; ++i)
    if (!strncmp(p->notes[i], prefix, strlen(prefix))) return 1;
  return 0;
}

static void plan_one(const char *line, FzeroControlsPlan *p) {
  const char *lines[] = {"Schema = 1", "steer = axis 0 " WHEEL " range=0..65535 rest=32768 travel=-1", line};
  FzeroControlsPlanLines(lines, line ? 3 : 2, p);
}

static char *slurp(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *s = (char *)calloc((size_t)n + 1, 1);
  if (fread(s, 1, (size_t)n, f) != (size_t)n) { free(s); s = NULL; }
  fclose(f);
  return s;
}

static void write_file(const char *path, const char *text) {
  FILE *f = fopen(path, "wb");
  CHECK(f);
  fputs(text, f);
  fclose(f);
}

static void plan_tests(void) {
  FzeroControlsPlan p;
  FzeroControlsPlanLines(kProfile, kProfileLines, &p);
  CHECK(p.ok);
  CHECK(!strcmp(p.revision, "rev-1") && !strcmp(p.profile, "Test Rig"));
  CHECK(p.vendor == 0x346E && p.product == 0x0006);
  CHECK(!strcmp(p.device, "MOZA R12 Base"));
  CHECK(key(&p, "SteeringAxis") == 0);
  CHECK(key(&p, "AcceleratorAxis") == 2 && key(&p, "AcceleratorInvert") == 0);
  CHECK(key(&p, "BrakeAxis") == 5 && key(&p, "BrakeInvert") == 0);
  CHECK(key(&p, "ButtonA") == 31 && key(&p, "ButtonB") == 18);
  CHECK(key(&p, "ButtonStart") == 35 && key(&p, "ButtonSelect") == 22);
  CHECK(key(&p, "ButtonUp") == 128 && key(&p, "ButtonRight") == 129);
  CHECK(key(&p, "ButtonDown") == 130 && key(&p, "ButtonLeft") == 131);
  CHECK(key(&p, "ButtonX") == -1000 && key(&p, "PedalThreshold") == -1000); /* not the profile's to set */
  CHECK(noted(&p, "clutch: no F-Zero control") && noted(&p, "handbrake: no F-Zero control"));
  CHECK(noted(&p, "shiftUp: no F-Zero control") && noted(&p, "camera: no F-Zero control"));
  CHECK(p.nkeys == 13);

  /* Pedal direction: F-Zero's invert flag follows travel and the contract's inverted flag. */
  plan_one("throttle = axis 2 " WHEEL " range=0..65535 rest=65535 travel=-1", &p);
  CHECK(key(&p, "AcceleratorInvert") == 1);
  plan_one("throttle = axis 2 " WHEEL " range=0..65535 rest=0 travel=+1 inverted", &p);
  CHECK(key(&p, "AcceleratorInvert") == 1);
  plan_one("brake = axis 5 " WHEEL " range=0..65535 rest=65535 travel=-1 inverted", &p);
  CHECK(key(&p, "BrakeAxis") == 5 && key(&p, "BrakeInvert") == 0);

  /* What F-Zero cannot represent is reported, never approximated. */
  plan_one("navUp = hat 0 4500 " WHEEL, &p);
  CHECK(key(&p, "ButtonUp") == -1000 && noted(&p, "navUp: hat diagonal"));
  plan_one("start = button 130 " WHEEL, &p); /* DirectInput has buttons 0..127: the shared parser refuses it */
  CHECK(key(&p, "ButtonStart") == -1000 && noted(&p, "start: bad-index"));
  plan_one("confirm = button 3 " SHIFTER, &p);
  CHECK(key(&p, "ButtonA") == -1000 && noted(&p, "confirm: on another device"));
  plan_one("throttle = axis 2 " SHIFTER " range=0..65535 rest=0 travel=+1", &p);
  CHECK(key(&p, "AcceleratorAxis") == -1000 && noted(&p, "throttle: on another device"));
  plan_one("navRight = hat 1 9000 " WHEEL, &p);
  CHECK(key(&p, "ButtonRight") == 133);
  plan_one("throttle = button 31 " WHEEL, &p); /* STD-033 allows button pedals; F-Zero's pedals are axes */
  CHECK(key(&p, "AcceleratorAxis") == -1000 && key(&p, "AcceleratorInvert") == -1000 &&
        noted(&p, "throttle: a button pedal"));
  plan_one("brake = button 7 " WHEEL, &p);
  CHECK(key(&p, "BrakeAxis") == -1000 && noted(&p, "brake: a button pedal"));
  plan_one("select =", &p);
  CHECK(key(&p, "ButtonSelect") == -1);
  plan_one("brake =", &p);
  CHECK(key(&p, "BrakeAxis") == -1 && key(&p, "BrakeInvert") == -1000);
  plan_one("start = button 35", &p); /* missing dev/prod: the shared parser rejects it */
  CHECK(key(&p, "ButtonStart") == -1000 && p.nnotes == 1 && noted(&p, "start: missing-dev"));

  const char *inverted[] = {"Schema = 1", "steer = axis 0 " WHEEL " range=0..65535 rest=32768 travel=-1 inverted",
                            "confirm = button 31 " WHEEL};
  FzeroControlsPlanLines(inverted, 3, &p);
  CHECK(p.ok && key(&p, "SteeringAxis") == -1000 && noted(&p, "steer: inverted steering"));

  const char *schema2[] = {"Schema = 2", "steer = axis 0 " WHEEL " range=0..65535 rest=32768 travel=-1"};
  FzeroControlsPlanLines(schema2, 2, &p);
  CHECK(!p.ok && strstr(p.error, "Schema 2"));
  /* A device whose SDL numbering was never compared with DirectInput's is refused, not guessed. */
  const char *other[] = {"Schema = 1", "steer = axis 0 dev={11111111-2222-3333-4444-555555555555} "
                         "prod={56781234-0000-0000-0000-504944564944} range=0..65535 rest=32768 travel=-1",
                         "confirm = button 31 dev={11111111-2222-3333-4444-555555555555} "
                         "prod={56781234-0000-0000-0000-504944564944}"};
  FzeroControlsPlanLines(other, 3, &p);
  CHECK(!p.ok && p.nkeys == 0 && strstr(p.error, "1234:5678 is not qualified"));
  const char *nosteer[] = {"Schema = 1", "confirm = button 31 " WHEEL};
  FzeroControlsPlanLines(nosteer, 2, &p);
  CHECK(!p.ok && strstr(p.error, "no steer"));

  /* Without a Revision the section's content identifies it. */
  const char *a[] = {"Schema = 1", "steer = axis 0 " WHEEL " range=0..65535 rest=32768 travel=-1"};
  const char *b[] = {"Schema = 1", "steer = axis 1 " WHEEL " range=0..65535 rest=32768 travel=-1"};
  FzeroControlsPlan pa, pb;
  FzeroControlsPlanLines(a, 2, &pa);
  FzeroControlsPlanLines(b, 2, &pb);
  CHECK(!strncmp(pa.revision, "fnv1a:", 6) && strcmp(pa.revision, pb.revision));
}

static void ini_tests(void) {
  const char *path = "controls-ini-test.ini";
  write_file(path, "; owner comment\r\n[Graphics]\r\nFullscreen = 1\r\n\r\n[Controller]\r\nSourceP1 = 1\r\n\r\n"
                   "[Sound]\r\nVolume = 100\r\n");
  CHECK(FzeroControlsIniSet(path, "Controller", "GuidP1", "abc"));
  CHECK(FzeroControlsIniSet(path, "graphics", "fullscreen", "0"));
  CHECK(FzeroControlsIniSet(path, "Controller.abc", "ButtonA", "31"));
  char *text = slurp(path);
  CHECK(text && !strcmp(text, "; owner comment\r\n[Graphics]\r\nfullscreen = 0\r\n\r\n[Controller]\r\nSourceP1 = 1\r\n"
                              "GuidP1 = abc\r\n\r\n[Sound]\r\nVolume = 100\r\n\r\n[Controller.abc]\r\nButtonA = 31\r\n"));
  free(text);
  remove(path);
  /* A UTF-8 BOM hides the first section from the game's own reader: the profile is refused, not applied. */
  write_file(path, "\xEF\xBB\xBF[Controls]\nSchema = 1\nsteer = axis 0 " WHEEL " range=0..65535 rest=32768 travel=-1\n");
  FzeroControlsPlan bom;
  CHECK(FzeroControlsPlanFile(path, &bom) == 1 && !bom.ok && strstr(bom.error, "BOM"));
  remove(path);
  CHECK(FzeroControlsIniSet(path, "Controls", "Schema", "1")); /* a missing file is created */
  text = slurp(path);
  CHECK(text && !strcmp(text, "[Controls]\nSchema = 1\n"));
  free(text);
  remove(path);
}

static int exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (f) fclose(f);
  return f != NULL;
}

/* A failed write leaves config.ini's bytes as they were and no temporary file behind. */
static void fault_tests(void) {
  const char *path = "controls-fault-test.ini";
  const char *backup = "controls-fault-test.ini.before-profile-controls";
  const char *tmp = "controls-fault-test.ini.controls-tmp";
  const char *original = "[Controller]\nSourceP1 = 1\n\n[Controller.abc]\nButtonX = 33\n";
  FzeroControlsPlan plan;
  FzeroControlsPlanLines(kProfile, kProfileLines, &plan);
  CHECK(plan.ok);
  remove(backup);
  for (int pass = 0; pass < 2; ++pass) {   /* without a backup yet (the backup step fails), then with one */
    if (pass) write_file(backup, original);
    for (int fault = 1; fault <= 2; ++fault) {
      write_file(path, original);
      FzeroControlsTestFault(fault);
      CHECK(FzeroControlsWrite(path, "abc", &plan) == 0);
      CHECK(FzeroControlsIniSet(path, "Controller", "GuidP1", "abc") == 0);
      FzeroControlsTestFault(0);
      char *now = slurp(path);
      CHECK(now && !strcmp(now, original));
      free(now);
      CHECK(!exists(tmp));
      CHECK(exists(backup) == pass);
    }
  }
  CHECK(FzeroControlsWrite(path, "abc", &plan) == 1);
  char *applied = slurp(path);
  int v = 0;
  CHECK(FzeroIniReadInt(path, "Controller.abc", "ButtonStart", &v) && v == 35);
  CHECK(FzeroIniReadInt(path, "Controller.abc", "ButtonX", &v) && v == 33);
  char value[64];
  CHECK(FzeroIniReadString(path, "ControlsApplied", "Revision", value, sizeof(value)) && !strcmp(value, "rev-1"));
  CHECK(FzeroControlsWrite(path, "abc", &plan) == 1); /* the same plan again changes nothing */
  char *again = slurp(path);
  CHECK(applied && again && !strcmp(applied, again));
  free(applied);
  free(again);
  CHECK(!exists(tmp));
  remove(path);
  remove(backup);
}

#if SNESRECOMP_SDL3
static void startup_tests(void) {
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
  /* Only the virtual wheel below: a real wheel with the same vendor/product (the rig's R12) must not be seen. */
  SDL_SetHint(SDL_HINT_JOYSTICK_DIRECTINPUT, "0");
  SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "0");
  SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "0");
  SDL_SetHint(SDL_HINT_JOYSTICK_GAMEINPUT, "0");
  SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
  SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "0");
  CHECK(SDL_Init(SDL_INIT_GAMEPAD));
  SDL_VirtualJoystickDesc desc;
  SDL_INIT_INTERFACE(&desc);
  desc.type = SDL_JOYSTICK_TYPE_WHEEL;
  desc.vendor_id = 0x346E;
  desc.product_id = 0x0006;
  desc.naxes = 8;
  desc.nbuttons = 128;
  desc.nhats = 1;
  desc.name = "MOZA R12 Base";
  SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
  CHECK(id != 0);
  SDL_Joystick *wheel = SDL_OpenJoystick(id);
  CHECK(wheel);
  char guid[40];
  SDL_GUIDToString(SDL_GetJoystickGUID(wheel), guid, sizeof(guid));

  const char *cfg = "controls-startup-test.ini";
  char text[4096];
  int n = snprintf(text, sizeof(text), "[Controller]\nSourceP1 = 1\nDeadzoneP1 = 25\n\n[Controller.%s]\n"
                   "AnalogSteering = 1\nDeadzone = 3\nButtonX = 33\nButtonStart = 36\nPedalThreshold = 0\n\n[Controls]\n",
                   guid);
  for (int i = 0; i < kProfileLines; ++i) n += snprintf(text + n, sizeof(text) - (size_t)n, "%s\n", kProfile[i]);
  write_file(cfg, text);
  remove("controls-startup-test.ini.before-profile-controls");

  CHECK(FzeroControlsApplyAtStartup(cfg) == 1);
  char value[80];
  CHECK(FzeroIniReadString(cfg, "Controller", "GuidP1", value, sizeof(value)) && !strcmp(value, guid));
  CHECK(FzeroIniReadString(cfg, "ControlsApplied", "Revision", value, sizeof(value)) && !strcmp(value, "rev-1"));
  char *backup = slurp("controls-startup-test.ini.before-profile-controls");
  CHECK(backup && !strcmp(backup, text)); /* the pre-apply file, byte for byte */
  free(backup);
  char *applied = slurp(cfg);
  CHECK(FzeroControlsApplyAtStartup(cfg) == 0); /* same revision: nothing rewritten */
  char *again = slurp(cfg);
  CHECK(applied && again && !strcmp(applied, again));
  free(applied);
  free(again);

  /* The game's own reader, on the virtual wheel, through the applied keys. */
  FzeroGamepadConfigure(cfg, guid, 25);
  SDL_GameController *pad = NULL;
  FzeroGamepadRefresh(&pad);
  CHECK(!pad);
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 2, -32768) && SDL_SetJoystickVirtualAxis(wheel, 5, -32768));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == 0);
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 0, 32767));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == 0x0080u); /* full right lock */
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 0, -32768));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == 0x0040u);
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 0, 0) && SDL_SetJoystickVirtualAxis(wheel, 2, 32767));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == 0x0001u); /* accelerator -> SNES B */
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 2, -32768) && SDL_SetJoystickVirtualAxis(wheel, 5, 32767));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == 0x0002u); /* brake -> SNES Y */
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 5, -32768));
  static const struct { int button; uint32_t bits; } kButtons[] = {
      {31, 0x0100u}, {18, 0x0001u}, {35, 0x0008u}, {22, 0x0004u}, {33, 0x0200u}, {36, 0}, {32, 0}, {6, 0}};
  for (size_t i = 0; i < sizeof(kButtons) / sizeof(kButtons[0]); ++i) {
    CHECK(SDL_SetJoystickVirtualButton(wheel, kButtons[i].button, true));
    SDL_UpdateJoysticks();
    CHECK(FzeroGamepadReadOverlay(pad) == kButtons[i].bits);
    CHECK(SDL_SetJoystickVirtualButton(wheel, kButtons[i].button, false));
  }
  static const struct { Uint8 hat; uint32_t bits; } kHats[] = {
      {SDL_HAT_UP, 0x0010u}, {SDL_HAT_DOWN, 0x0020u}, {SDL_HAT_LEFT, 0x0040u}, {SDL_HAT_RIGHT, 0x0080u}};
  for (size_t i = 0; i < sizeof(kHats) / sizeof(kHats[0]); ++i) {
    CHECK(SDL_SetJoystickVirtualHat(wheel, 0, kHats[i].hat));
    SDL_UpdateJoysticks();
    CHECK(FzeroGamepadReadOverlay(pad) == kHats[i].bits);
  }
  CHECK(SDL_SetJoystickVirtualHat(wheel, 0, SDL_HAT_CENTERED));

  /* Test injection (fzero_inject.h) through the same reader: raw and action samples on the profile's wheel replace
   * SDL's values before the applied keys turn them into SNES bits; they end on time; another device is refused. */
  SDL_UpdateJoysticks();
  CHECK(!FzeroInjectArmed());
  CHECK(FzeroGamepadRead(pad) == 0);
  CHECK(FzeroInjectTestArm("not-this-wheel", kProfile, kProfileLines));   /* armed for another stick: */
  FzeroInjectTestClock(1000);
  CHECK(FzeroInjectTestCommand("inject raw button 31 dev={11111111-2222-3333-4444-555555555555} value=1 ms=500", NULL, 0));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadReadOverlay(pad) == 0);                      /* ...the opened wheel's GUID differs: untouched */
  CHECK(FzeroInjectTestArm(guid, kProfile, kProfileLines) && FzeroInjectArmed());
  char why[160];
  CHECK(!FzeroInjectTestCommand("inject raw button 1 dev={66666666-7777-8888-9999-aaaaaaaaaaaa} value=1 ms=500", why, sizeof why) &&
        strstr(why, "one controller"));                          /* the shifter: F-Zero reads the wheel only */
  CHECK(FzeroInjectTestCommand("inject raw axis 0 dev={11111111-2222-3333-4444-555555555555} value=65535 ms=500", why, sizeof why));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == 0x0080u);                        /* injected full right lock, physical axis centred */
  CHECK(FzeroInjectTestCommand("inject action throttle 1 ms=500", why, sizeof why));
  CHECK(FzeroGamepadRead(pad) == 0x0081u);                        /* plus the accelerator, through [Controls] */
  CHECK(FzeroInjectTestCommand("inject action confirm 1 ms=500", why, sizeof why));
  CHECK((FzeroGamepadReadOverlay(pad) & 0x0100u) == 0x0100u);     /* confirm -> SNES A */
  CHECK(FzeroInjectTestCommand("inject action navUp 1 ms=500", why, sizeof why));
  CHECK((FzeroGamepadReadOverlay(pad) & 0x0010u) == 0x0010u);     /* hat up -> SNES up */
  FzeroInjectTestClock(1600);                                     /* every sample has ended */
  CHECK(FzeroGamepadRead(pad) == 0 && FzeroGamepadReadOverlay(pad) == 0);
  CHECK(FzeroInjectTestCommand("inject action brake 1 ms=200", why, sizeof why));
  CHECK(FzeroGamepadRead(pad) == 0x0002u);                        /* brake -> SNES Y, the captured rest honoured */
  FzeroInjectTestClock(2000);
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 5, 32767));
  SDL_UpdateJoysticks();
  CHECK(FzeroGamepadRead(pad) == 0x0002u);                        /* the physical brake still works when armed */
  CHECK(SDL_SetJoystickVirtualAxis(wheel, 5, -32768));
  SDL_UpdateJoysticks();
  FzeroGamepadShutdown(&pad);

  /* A new revision is applied again; launcher edits in between are overwritten only for the profile's keys. */
  CHECK(FzeroControlsIniSet(cfg, "Controls", "Revision", "rev-2"));
  CHECK(FzeroControlsIniSet(cfg, "Controls", "start", "button 40 " WHEEL));
  char section[64];
  snprintf(section, sizeof(section), "Controller.%s", guid);
  CHECK(FzeroControlsIniSet(cfg, section, "ButtonY", "7"));
  CHECK(FzeroControlsApplyAtStartup(cfg) == 1);
  int start = 0, y = 0;
  CHECK(FzeroIniReadInt(cfg, section, "ButtonStart", &start) && start == 40);
  CHECK(FzeroIniReadInt(cfg, section, "ButtonY", &y) && y == 7);

  /* Two attached wheels with the profile's vendor/product: SDL cannot tell them apart, so nothing is written. */
  desc.name = "second MOZA R12 Base";
  SDL_JoystickID twin = SDL_AttachVirtualJoystick(&desc);
  CHECK(twin != 0);
  CHECK(FzeroControlsIniSet(cfg, "Controls", "Revision", "rev-twin"));
  char *pre = slurp(cfg);
  CHECK(FzeroControlsApplyAtStartup(cfg) == 0);
  char *post = slurp(cfg);
  CHECK(pre && post && !strcmp(pre, post));
  free(pre);
  free(post);
  CHECK(SDL_DetachVirtualJoystick(twin));

  /* No matching device attached: nothing is written and the revision stays pending. */
  SDL_CloseJoystick(wheel);
  CHECK(SDL_DetachVirtualJoystick(id));
  CHECK(FzeroControlsIniSet(cfg, "Controls", "Revision", "rev-3"));
  char *before = slurp(cfg);
  CHECK(FzeroControlsApplyAtStartup(cfg) == 0);
  char *after = slurp(cfg);
  CHECK(before && after && !strcmp(before, after));
  free(before);
  free(after);
  SDL_Quit();
  remove(cfg);
  remove("controls-startup-test.ini.before-profile-controls");
}
#endif

int main(void) {
  plan_tests();
  ini_tests();
  fault_tests();
#if SNESRECOMP_SDL3
  startup_tests();
#endif
  printf("F-Zero profile controls: %d checks passed (translation, notes, ini edits, startup apply, game reader)\n",
         s_checks);
  return 0;
}
