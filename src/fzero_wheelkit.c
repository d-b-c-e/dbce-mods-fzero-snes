/* The Wheelkit runtime in F-Zero (fzero_wheelkit.h): the loader and what the
 * call sites in sdl_main.c do. Everything not specific to F-Zero (devices, the
 * profile, the panel, the telemetry packet) is in wheelkit.dll. */
#include "fzero_wheelkit.h"
#include "fzero_renderer.h"
#include "fzero_wheelkit_steering.h"
#include "wheelkit.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* This build's own version, by the wheelkit repository's scheme (docs/versioning.md
 * there); its games/f-zero/wheelkit-mod.json says the same. The program's own
 * version stays upstream's. */
#define FZERO_WHEELKIT_VERSION "0.9.2-alpha"

static struct {
#define X(ret, name, args) ret (WK_CALL *name) args;
  WK_FUNCTIONS(X)
#undef X
} wk;

static bool s_on, s_panel, s_attached, s_driven;
static float s_sensitivity = 1.0f, s_dead_zone = FZERO_STEER_DEAD_ZONE;
static FzeroSteer s_steer;
static bool s_triples; /* three screens from the rig profile (at the end of this file) */
static void triples_start(void);
static struct { uint16_t x, y; bool have, jumped; float speed, distance; } s_car;

static const char kPages[] =
  "{\"settings\":{\"product\":\"F-Zero\",\"version\":\"%s\",\"keys\":\"F6 settings\",\"pages\":{"
  "\"controls\":["
  "{\"id\":\"controls.profile\",\"kind\":\"status\",\"label\":\"Wheel profile\",\"text\":\"No profile from Wheelkit\"},"
  "{\"id\":\"controls.sensitivity\",\"kind\":\"percent\",\"label\":\"Steering sensitivity\",\"min\":25,\"max\":300,"
  "\"default\":100,\"help\":\"How far the machine steers for a turn of the wheel.\"},"
  "{\"id\":\"controls.deadzone\",\"kind\":\"percent\",\"label\":\"Steering dead zone\",\"min\":0,\"max\":20,"
  "\"default\":3,\"help\":\"How far the wheel turns before the machine steers.\"}],"
  "\"ffb\":[{\"id\":\"ffb.status\",\"kind\":\"status\",\"text\":\"Off\"},"
  "{\"id\":\"ffb.enabled\",\"kind\":\"toggle\",\"label\":\"Force feedback\",\"default\":true,"
  "\"help\":\"The wheel centers more firmly with speed. F8 turns it off.\"},"
  "{\"id\":\"ffb.steering\",\"kind\":\"percent\",\"label\":\"Steering strength\",\"default\":50,"
  "\"help\":\"50%% is the tuned strength; 100%% doubles it.\"}],"
  "\"display\":[{\"id\":\"display.status\",\"kind\":\"status\",\"label\":\"Screens\",\"text\":\"One screen\"}],"
  "\"telemetry\":[{\"id\":\"telemetry.enabled\",\"kind\":\"toggle\",\"label\":\"Telemetry\",\"default\":true,"
  "\"help\":\"Sends race data to SimHub and dashboards on this PC.\"}],"
  "\"help\":[{\"id\":\"help.runtime\",\"kind\":\"status\",\"label\":\"Wheelkit runtime\",\"text\":\"not loaded\"}]}}}";

static void WK_CALL log_line(const char *line, void *context) {
  (void)context;
  fprintf(stderr, "%s\n", line);
}

void FzeroWheelkitStart(const char *config_path, const char *version) {
  wchar_t path[MAX_PATH];
  DWORD n = GetModuleFileNameW(NULL, path, MAX_PATH);
  while (n && path[n - 1] != L'\\' && path[n - 1] != L'/') --n;
  if (n + 13 > MAX_PATH) return;
  wcscpy(path + n, L"wheelkit.dll");
  HMODULE m = LoadLibraryW(path);
  bool complete = m != NULL;
#define X(ret, name, args) \
  wk.name = m ? (ret (WK_CALL *) args)(void *)GetProcAddress(m, #name) : NULL; \
  complete = complete && wk.name;
  WK_FUNCTIONS(X)
#undef X
  if (!complete || wk.wk_abi_version() != WK_ABI_VERSION) {
    fprintf(stderr, "[wheelkit] %s; keyboard and gamepad only\n",
            m ? "wheelkit.dll is another version" : "wheelkit.dll not found beside the game");
    return;
  }
  char settings[MAX_PATH];
  if (!_fullpath(settings, config_path, sizeof(settings))) return;
  wk_identity id = {sizeof(id)};
  id.game_id = "f-zero";
  id.game_name = "F-Zero";
  (void)version;  /* upstream's, for the program; the runtime is told this build's */
  id.game_version = FZERO_WHEELKIT_VERSION;
  id.settings_file = settings;
  id.log = log_line;
  if (!wk.wk_start(&id)) {
    fprintf(stderr, "[wheelkit] not started: %s\n", wk.wk_last_error());
    return;
  }
  s_on = true;
  triples_start(); /* before the window is created */
  char pages[sizeof(kPages) + 64], value[32];
  if (wk.wk_settings_get("Wheel", "SteeringSensitivity", value, sizeof(value))) {
    float saved = (float)atof(value);
    if (saved >= 0.25f && saved <= 3.0f) s_sensitivity = saved;
  }
  if (wk.wk_settings_get("Wheel", "SteeringDeadZone", value, sizeof(value))) {
    float saved = (float)atof(value);
    if (saved >= 0.0f && saved <= 0.2f) s_dead_zone = saved;
  }
  char line[200];
  snprintf(line, sizeof(line), "f-zero: steering: dead zone %.0f%%, sensitivity %.0f%%, square-root curve, "
           "Left and Right held for that share of frames in a race (the old build's mapping)",
           s_dead_zone * 100.0, s_sensitivity * 100.0);
  wk.wk_log(line);
  int length = snprintf(pages, sizeof(pages), kPages, version);
  if (!wk.wk_panel_configure(pages, (size_t)length)) {
    snprintf(line, sizeof(line), "f-zero: F6 pages refused: %s", wk.wk_last_error());
    wk.wk_log(line); /* the launch check forbids this line */
    return;
  }
  s_panel = true;
  wk.wk_panel_set_number("controls.sensitivity", s_sensitivity * 100.0);
  wk.wk_panel_set_number("controls.deadzone", s_dead_zone * 100.0);
  const char *profile = wk.wk_input_profile();
  wk.wk_panel_set_text("controls.profile", *profile ? profile : "No profile from Wheelkit: keyboard and gamepad work",
                       *profile ? WK_TONE_GOOD : WK_TONE_WARNING);
  wk.wk_panel_set_text("help.runtime", wk.wk_runtime_version(), WK_TONE_NORMAL);
  wk.wk_panel_set_text("display.status", s_triples ? "Three screens, from your rig profile; the sides in a race" :
                       "One screen", WK_TONE_NORMAL);
}

bool FzeroWheelkitWantsGl(void) { return s_panel; }

static void panel_events(void) {
  wk_panel_event e = {sizeof(e)};
  while (wk.wk_panel_poll_event(&e)) {
    if (e.type == WK_PANEL_CHANGED && !strcmp(e.id, "controls.sensitivity")) {
      char value[32];
      s_sensitivity = (float)(e.number / 100.0);
      snprintf(value, sizeof(value), "%g", s_sensitivity);
      if (!wk.wk_settings_set("Wheel", "SteeringSensitivity", value))
        wk.wk_panel_toast("Not saved", wk.wk_last_error(), 2500);
    } else if (e.type == WK_PANEL_CHANGED && !strcmp(e.id, "controls.deadzone")) {
      char value[32];
      s_dead_zone = FzeroSteerClamp((float)(e.number / 100.0), 0.0f, 0.2f, FZERO_STEER_DEAD_ZONE);
      snprintf(value, sizeof(value), "%g", s_dead_zone);
      if (!wk.wk_settings_set("Wheel", "SteeringDeadZone", value))
        wk.wk_panel_toast("Not saved", wk.wk_last_error(), 2500);
    }
    memset(&e, 0, sizeof(e));
    e.size = sizeof(e);
  }
}


static uint16_t read16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static int wrapped(int delta, int modulus) {
  return delta > modulus / 2 ? delta - modulus : delta < -modulus / 2 ? delta + modulus : delta;
}
/* The race clock and lap records are BCD minutes, seconds, hundredths. */
static float bcd_seconds(const uint8_t *t) {
  for (int i = 0; i < 3; ++i)
    if ((t[i] >> 4) > 9 || (t[i] & 15) > 9) return 0.0f;
  return (float)((t[0] >> 4) * 10 + (t[0] & 15)) * 60.0f + (float)((t[1] >> 4) * 10 + (t[1] & 15)) +
         (float)((t[2] >> 4) * 10 + (t[2] & 15)) * 0.01f;
}

/* WRAM: $54/$55 scene (2 and 3 or more: racing), $0B70/$0B90 the machine's
 * position (8192 x 4096 units, a quarter metre each), $0BD1 the way the machine
 * faces (192 steps a turn; upstream's fzero_actor.c: held Right adds to it, Left
 * subtracts, so it grows clockwise; $0BE1 is the way it travels), $C0 the race
 * clock, $0F53 laps timed, $0DC8 rank. */
static void telemetry(const uint8_t *ram, uint32_t input) {
  wk_car car = {sizeof(car)};
  bool racing = ram[0x54] == 2 && ram[0x55] >= 3;
  uint16_t x = read16(ram + 0x0b70) & 0x1fff, y = read16(ram + 0x0b90) & 0x0fff;
  s_car.jumped = false;
  if (racing && s_car.have) {
    int dx = wrapped(x - s_car.x, 0x2000), dy = wrapped(y - s_car.y, 0x1000);
    s_car.jumped = dx * dx + dy * dy > 32 * 32;
    if (!s_car.jumped) { /* further in one frame is a state load or rewind */
      float step = sqrtf((float)(dx * dx + dy * dy)) * 0.25f;
      s_car.speed += (step * 60.098811862f - s_car.speed) * 0.25f;
      s_car.distance += step;
    }
  } else {
    s_car.speed = 0.0f;
    if (!racing) s_car.distance = 0.0f;
  }
  s_car.x = x; s_car.y = y; s_car.have = racing;
  car.race_on = racing;
  car.speed = car.velocity[2] = s_car.speed;
  car.rpm_max = 12000.0f;
  car.rpm_idle = 1200.0f;
  car.rpm = racing ? 1200.0f + fminf(s_car.speed / 200.0f, 1.0f) * 10800.0f : 0.0f;
  car.gear = 1;
  car.throttle = (input & 1u) ? 1.0f : 0.0f;
  car.brake = (input & 2u) ? 1.0f : 0.0f;
  /* The game's own steering, as it applies it: a SNES pad steers by the share of frames Left or Right is held, so
   * this is that share, signed, over about the last quarter second (the proof shows it beside the wheel's angle). */
  static float pad_share;
  pad_share += (((input & 0x40u) ? -1.0f : (input & 0x80u) ? 1.0f : 0.0f) - pad_share) * (1.0f / 15.0f);
  if (!racing) pad_share = 0.0f;
  car.steer = pad_share;
  car.yaw = (float)ram[0x0bd1] * (6.28318530718f / 192.0f);
  car.position[0] = (float)x * 0.25f;
  car.position[2] = (float)y * 0.25f;
  car.distance = s_car.distance;
  car.race_time = bcd_seconds(ram + 0xc0);
  if (racing && ram[0x0f53] <= 5) car.lap = ram[0x0f53];
  if (racing && ram[0x0dc8] >= 1 && ram[0x0dc8] <= 20) car.race_position = ram[0x0dc8];
  wk.wk_telemetry_frame(&car);
}

/* The old fork's force model (its main branch, src/fzero_ffb.cpp) at the levels the
 * owner drove on the R12 (his config.ini [ForceFeedback]: SteeringStrength 50,
 * Strength 12, ImpactStrength 50), as Steering strength 50: a spring of 0.5
 * (SteeringStrength x 100 of 10000) and a damper of 0.048 (Strength x 40) the wheel
 * runs itself and a road texture of 0.048 (0.084 on rough ground, $C7; Strength x 40
 * and x 70) at 18 to 42 Hz, each growing with speed to full at 2 units a frame
 * (30 m/s); a 120 ms knock of 0.5 (ImpactStrength x 100) when the machine loses 16 or
 * more energy ($C9) in a frame without boosting (A). */
static void force(const uint8_t *ram, uint32_t input) {
  static uint16_t energy;
  static bool had;
  static int cooldown;
  bool racing = ram[0x54] == 2 && ram[0x55] >= 3;
  float scale = racing ? fminf(s_car.speed / 30.05f, 1.0f) : 0.0f;
  uint16_t now = read16(ram + 0xc9);
  if (cooldown > 0) --cooldown;
  if (racing && had && !s_car.jumped && energy >= now + 16 && !(input & 0x100u) && !cooldown) {
    wk.wk_ffb_cue(WK_CUE_IMPACT, 0.5f, 120);
    cooldown = 8;
  }
  energy = now;
  had = racing;
  wk.wk_ffb_set_spring(0.5f * scale);
  wk.wk_ffb_set_damper(0.048f * scale);
  wk.wk_ffb_set_texture((ram[0xc7] ? 0.084f : 0.048f) * scale, 18.0f + 24.0f * scale);
}

/* Under test input only (inject.on): every half second of a race with steady test input, what the mapping did and
 * what the game did with it ($0BD1, the way the machine faces: Right turns it up, Left down, 192 steps a turn), the
 * pedals and the speed, so a check can see the machine answer the wheel and the pedals. */
static void steering_report(const uint8_t *ram, float steer, uint32_t input) {
  static unsigned frames, held, gas, brake;
  static double sum;
  static int heading, turned;
  static float speed;
  const bool racing = ram[0x54] == 2 && ram[0x55] >= 3;
  static float first;
  static uint32_t pedals;
  if (!racing) { frames = 0; return; }
  /* One report per steady wheel position and pedal state. */
  if (frames && (fabsf(steer - first) > 0.02f || (input & 3u) != pedals)) frames = 0;
  const int now = ram[0x0bd1];
  if (frames == 0) {
    held = gas = brake = 0; sum = 0.0; turned = 0; heading = now; speed = s_car.speed; first = steer; pedals = input & 3u;
  }
  int step = now - heading;
  if (step > 96) step -= 192;
  if (step < -96) step += 192;
  turned += step;
  heading = now;
  sum += steer;
  if (input & (sum < 0 ? 0x40u : 0x80u)) ++held;
  if (input & 1u) ++gas;
  if (input & 2u) ++brake;
  if (++frames < 30) return; /* half a second of a steady input */
  const double mean = sum / frames;
  char line[260];
  snprintf(line, sizeof(line), "f-zero: steering in a race (test input): wheel %+.3f, %s held in %u of %u frames "
           "(the mapping's %.3f), facing moved %+d of 192; B %u Y %u frames; speed %.0f to %.0f km/h", mean,
           mean < 0 ? "Left" : "Right", held, frames, FzeroSteerDuty((float)mean, s_sensitivity, s_dead_zone), turned,
           gas, brake, speed * 3.6, s_car.speed * 3.6);
  wk.wk_log(line);
  frames = 0;
}

uint32_t FzeroWheelkitFrame(uint32_t input, const uint8_t *ram) {
  if (!s_on) return input;
  if (s_panel) panel_events();
  const uint32_t keyboard = input;
  wk_input in = {sizeof(in)};
  if (wk.wk_input_poll(&in)) {
    if (in.flags & WK_INPUT_PANEL) {
      input &= ~0xfffu; /* the panel has the input */
    } else if (in.flags & (WK_INPUT_PROFILE | WK_INPUT_INJECTED)) {
      /* B Y Select Start Up Down Left Right A X L R are bits 0 to 11. */
      static const struct { int action; uint32_t button; } kButtons[] = {
        {WK_ACTION_CONFIRM, 0x100u}, {WK_ACTION_BACK, 0x1u}, {WK_ACTION_START, 0x8u}, {WK_ACTION_SELECT, 0x4u},
        {WK_ACTION_NAV_UP, 0x10u}, {WK_ACTION_NAV_DOWN, 0x20u}, {WK_ACTION_NAV_LEFT, 0x40u}, {WK_ACTION_NAV_RIGHT, 0x80u},
        {WK_ACTION_SHIFT_DOWN, 0x400u}, {WK_ACTION_SHIFT_UP, 0x800u}};
      for (size_t i = 0; i < sizeof(kButtons) / sizeof(kButtons[0]); ++i)
        if (in.held[kButtons[i].action]) input |= kButtons[i].button;
      if (in.throttle > 0.5f) input |= 0x1u; /* B: accelerate */
      if (in.brake > 0.5f) input |= 0x2u;    /* Y: brake */
      /* A SNES pad has no steering axis: in a race the wheel's angle is the share of frames Left or Right is held
       * (fzero_wheelkit_steering.h); in the menus the wheel does not steer, as in the old build. */
      if (ram[0x54] == 2 && ram[0x55] >= 3) input |= FzeroSteerPulse(&s_steer, in.steer, s_sensitivity, s_dead_zone);
      else s_steer.phase = 0, s_steer.direction = 0;
      if (!s_driven && input != keyboard) { s_driven = true; wk.wk_log("f-zero: the profile's input reached player 1's controller"); }
      if (in.flags & WK_INPUT_INJECTED) steering_report(ram, in.steer, input);
    }
  }
  static bool was_racing;
  const bool racing = ram[0x54] == 2 && ram[0x55] >= 3;
  if (racing && !was_racing) wk.wk_log("f-zero: a race started");
  was_racing = racing;
  static int scene = -1;
  if ((wk.wk_switches() & WK_SWITCH_INJECT) && scene != (ram[0x54] << 8 | ram[0x55])) {
    char line[64]; /* under test input, the game's scene ($54, $55), for writing a route */
    scene = ram[0x54] << 8 | ram[0x55];
    snprintf(line, sizeof(line), "f-zero: scene %u %u", ram[0x54], ram[0x55]);
    wk.wk_log(line);
  }
  telemetry(ram, input);
  force(ram, input);
  static unsigned frames;
  wk_ffb_status st = {sizeof(st)};
  if (s_panel && ++frames % 30 == 0 && wk.wk_ffb_get_status(&st))
    wk.wk_panel_set_text("ffb.status", st.text, st.state == WK_FFB_ON ? WK_TONE_GOOD : WK_TONE_NORMAL);
  return input;
}

void FzeroWheelkitPresent(int width, int height) {
  if (!s_panel) return;
  if (!s_attached) {
    HDC dc = wglGetCurrentDC();
    HWND window = dc ? WindowFromDC(dc) : NULL;
    if (!window) return;
    s_attached = true;
    if (!wk.wk_panel_gl_attach(window)) { s_panel = false; return; }
  }
  wk.wk_panel_gl_frame(width, height);
}

void FzeroWheelkitStop(void) {
  if (!s_on) return;
  wk.wk_panel_gl_detach();
  wk.wk_stop();
  s_on = s_panel = false;
}

/* ---- three screens -------------------------------------------------------------------------------------------- */
/* From the rig profile (wk_triples_get): three screens spanned by one borderless window, the centre one drawn as the
 * game always is (its picture, HUD and menus), and in a race each side screen drawn from the runtime's views at the
 * rig's angles (wk_triples_views at the seat's true field of view): the Mode 7 ground and the skyline the centre's
 * own lines describe (fzero_renderer.c, FzeroRendererDrawSide). No Screens setting: the profile says it. */
static wk_triples s_tr;

static void triples_start(void) {
  s_tr.size = sizeof(s_tr);
  if (!wk.wk_triples_get(&s_tr)) return;
  s_triples = s_tr.span && s_tr.rig && s_tr.realistic_fov_y > 0 && s_tr.center_width > 0 && s_tr.width > s_tr.center_width &&
              (s_tr.screens == WK_SCREENS_SEPARATE || s_tr.screens == WK_SCREENS_SURROUND);
  char line[320];
  if (s_triples)
    snprintf(line, sizeof(line), "f-zero: three screens: one window %dx%d at %d,%d, the centre at %d (%d wide); sides at "
             "%.0f and %.0f degrees, the seat's view %.1f degrees high; sides drawn in a race",
             s_tr.width, s_tr.height, s_tr.x, s_tr.y, s_tr.center_x, s_tr.center_width, s_tr.left_degrees,
             s_tr.right_degrees, s_tr.realistic_fov_y * 57.29577951308232);
  else
    snprintf(line, sizeof(line), "f-zero: one screen (%s)", s_tr.text);
  wk.wk_log(line);
}

bool FzeroWheelkitSpan(int span[4]) {
  if (!s_triples) return false;
  span[0] = s_tr.x; span[1] = s_tr.y; span[2] = s_tr.width; span[3] = s_tr.height;
  return true;
}
int FzeroWheelkitCentreWidth(int drawable_width) {
  return s_triples ? (int)((long long)drawable_width * s_tr.center_width / s_tr.width) : drawable_width;
}
int FzeroWheelkitCentreX(int drawable_width) {
  return s_triples ? (int)((long long)drawable_width * s_tr.center_x / s_tr.width) : 0;
}

/* The side screens' rays in the centre's eye frame: a pixel's direction in a screen's own view, from its off-axis
 * projection (row vectors, row-major, z = -1 ahead), turned back into the centre's view by the transpose of that
 * screen's rotation. */
static void side_rays(const double rotation[16], const double projection[16], int width, int height, FzeroSideView *v) {
  const double origin[3] = {(-1.0 + projection[8]) / projection[0], (1.0 + projection[9]) / projection[5], -1.0};
  const double right[3] = {2.0 / (width * projection[0]), 0.0, 0.0};
  const double down[3] = {0.0, -2.0 / (height * projection[5]), 0.0};
  for (int j = 0; j < 3; ++j) {
    v->origin[j] = v->right[j] = v->down[j] = 0.0;
    for (int i = 0; i < 3; ++i) {
      v->origin[j] += origin[i] * rotation[j * 4 + i];
      v->right[j] += right[i] * rotation[j * 4 + i];
      v->down[j] += down[i] * rotation[j * 4 + i];
    }
  }
}

bool FzeroWheelkitSides(int x, int y, int w, int h, int drawable_width, int drawable_height, int logical_width,
                        const uint32_t *pixels[2], int *width, int *height, int rects[2][4]) {
  (void)x; (void)y;
  if (!s_triples || w < 2 || h < 2 || drawable_height < 2) return false;
  static uint32_t *buffers[2];
  static FzeroSideView views[2];
  static int built_w, built_h, built_dw, built_dh, sw, sh;
  static bool ok, noted;
  const int cx = FzeroWheelkitCentreX(drawable_width), cw = FzeroWheelkitCentreWidth(drawable_width);
  const int lw = cx, rw = drawable_width - cx - cw;
  if (lw < 2 || rw < 2) return false;
  if (built_w != w || built_h != h || built_dw != drawable_width || built_dh != drawable_height) {
    built_w = w; built_h = h; built_dw = drawable_width; built_dh = drawable_height; ok = false;
    /* The side pictures are drawn on the CPU, so at a fixed 288 rows (as the old build's) and scaled up. */
    sh = 288; sw = (int)((long long)sh * (lw > rw ? lw : rw) / drawable_height);
    if (sw < 2 || sw > 4096) return false;
    for (int i = 0; i < 2; ++i) {
      free(buffers[i]);
      buffers[i] = malloc((size_t)sw * sh * sizeof(uint32_t));
      if (!buffers[i]) return false;
    }
    /* The centre's projection at the seat's true view (a centred D3DXMatrixPerspectiveFovRH, row-major). */
    const double n = 0.1, f = 1000.0, ty = tan(s_tr.realistic_fov_y * 0.5), tx = ty * cw / drawable_height;
    const double centre[16] = {1.0 / tx, 0, 0, 0, 0, 1.0 / ty, 0, 0, 0, 0, f / (n - f), -1, 0, 0, n * f / (n - f), 0};
    wk_triple_views tv = {sizeof(tv)};
    if (!wk.wk_triples_views(centre, &tv)) { wk.wk_log("f-zero: three screens: the runtime gave no views; sides black"); return false; }
    for (int i = 0; i < 2; ++i) {
      side_rays(tv.rotation[i ? 2 : 0], tv.projection[i ? 2 : 0], sw, sh, &views[i]);
      /* The game's picture within the centre screen (Fit fills it at the screen's shape). */
      views[i].tan_x = tx * w / cw;
      views[i].tan_y = ty * h / drawable_height;
      views[i].panel_tan_y = ty;
      views[i].logical_width = logical_width;
    }
    /* What is drawn, in the proof line's terms: each view's yaw (its screen's facing, from the rotation) and its
     * horizontal field of view (from its projection's edges), to compare with the rig profile. */
    double yaw[3], hfov[3];
    for (int i = 0; i < 3; ++i) {
      const double *r = tv.rotation[i], *p = tv.projection[i];
      yaw[i] = atan2(-r[2], r[10]) * 57.29577951308232; /* the screen's forward, -(column 2), against -z */
      hfov[i] = (atan((p[8] + 1.0) / p[0]) - atan((p[8] - 1.0) / p[0])) * 57.29577951308232;
    }
    char line[200];
    snprintf(line, sizeof(line), "f-zero: three screens: views=3 angles=%.1f,%.1f,%.1f hfov=%.1f,%.1f,%.1f (the rig profile: "
             "sides %.1f and %.1f degrees)", yaw[0], yaw[1], yaw[2], hfov[0], hfov[1], hfov[2], s_tr.left_degrees,
             s_tr.right_degrees);
    wk.wk_log(line);
    ok = true;
  }
  if (!ok) return false;
  for (int i = 0; i < 2; ++i) views[i].logical_width = logical_width;
  LARGE_INTEGER t0, t1, hz;
  QueryPerformanceCounter(&t0);
  if (!FzeroRendererDrawSides(views, buffers, sw, sh)) return false;
  { /* Asked again while the sides are drawn, so the runtime knows three views are on the screens now (its proof). */
    const double n = 0.1, f = 1000.0, ty = tan(s_tr.realistic_fov_y * 0.5), tx = ty * cw / drawable_height;
    const double centre[16] = {1.0 / tx, 0, 0, 0, 0, 1.0 / ty, 0, 0, 0, 0, f / (n - f), -1, 0, 0, n * f / (n - f), 0};
    wk_triple_views tv = {sizeof(tv)};
    wk.wk_triples_views(centre, &tv);
  }
  QueryPerformanceCounter(&t1);
  static bool rival_noted;
  if (!rival_noted && FzeroRendererSideOpponentPixels()) {
    rival_noted = true;
    wk.wk_log("f-zero: three screens: a rival drawn on a side screen (its own sprite, standing at its place)");
  }
  static double spent;
  static unsigned draws;
  spent += (double)(t1.QuadPart - t0.QuadPart);
  if (++draws == 600) { /* what the side screens cost, for a slow frame rate's report */
    QueryPerformanceFrequency(&hz);
    char cost[120];
    snprintf(cost, sizeof(cost), "f-zero: three screens: the side screens took %.1f ms a presented frame (average of 600)",
             spent * 1000.0 / (double)hz.QuadPart / draws);
    wk.wk_log(cost);
    spent = 0.0; draws = 0;
  }
  if (!noted) {
    noted = true;
    char line[200];
    snprintf(line, sizeof(line), "f-zero: three screens: side screens drawn in a race (the ground and the skyline, %dx%d "
             "each, scaled to %dx%d and %dx%d)", sw, sh, lw, drawable_height, rw, drawable_height);
    wk.wk_log(line);
  }
  pixels[0] = buffers[0]; pixels[1] = buffers[1];
  *width = sw; *height = sh;
  rects[0][0] = 0; rects[0][1] = 0; rects[0][2] = lw; rects[0][3] = drawable_height;
  rects[1][0] = cx + cw; rects[1][1] = 0; rects[1][2] = rw; rects[1][3] = drawable_height;
  return true;
}

/* The old build's Reduce crash flashes (on in the owner's fzero-video.ini): in a race, a frame whose picture is near
 * white on average (the flash after a hard impact) is not presented, so the previous picture stays, for at most 6
 * frames running. The game, the force and telemetry carry on. */
bool FzeroWheelkitHoldFlash(const uint32_t *pixels, int width, int height, const uint8_t *ram) {
  static unsigned held, total;
  if (!s_on || !pixels || width < 64 || height < 32 || !(ram[0x54] == 2 && ram[0x55] >= 3)) { held = 0; return false; }
  unsigned sum = 0, samples = 0;
  for (int y = height / 32; y < height; y += height / 16)
    for (int x = width / 64; x < width; x += width / 32) {
      uint32_t c = pixels[(size_t)y * width + x];
      sum += ((c >> 16) & 255) + ((c >> 8) & 255) + (c & 255);
      ++samples;
    }
  if (!samples || sum / (3 * samples) < 225 || held >= 6) { held = 0; return false; }
  ++held;
  if (++total == 1) wk.wk_log("f-zero: a crash flash held (the previous picture kept, as the old build's Reduce crash flashes)");
  return true;
}