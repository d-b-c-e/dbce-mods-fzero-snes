/* The Wheelkit runtime in F-Zero (fzero_wheelkit.h): the loader and what the
 * call sites in sdl_main.c do. Everything not specific to F-Zero (devices, the
 * profile, the panel, the telemetry packet) is in wheelkit.dll. */
#include "fzero_wheelkit.h"
#include "wheelkit.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* This build's own version, by the wheelkit repository's scheme (docs/versioning.md
 * there); its games/f-zero/wheelkit-mod.json says the same. The program's own
 * version stays upstream's. */
#define FZERO_WHEELKIT_VERSION "0.4.0-alpha"

static struct {
#define X(ret, name, args) ret (WK_CALL *name) args;
  WK_FUNCTIONS(X)
#undef X
} wk;

static bool s_on, s_panel, s_attached, s_driven;
static float s_sensitivity = 1.0f;
static uint32_t s_phase;
static int s_direction;
static struct { uint16_t x, y; bool have, jumped; float speed, distance; } s_car;

static const char kPages[] =
  "{\"settings\":{\"product\":\"F-Zero\",\"version\":\"%s\",\"keys\":\"F6 settings\",\"pages\":{"
  "\"controls\":["
  "{\"id\":\"controls.profile\",\"kind\":\"status\",\"label\":\"Wheel profile\",\"text\":\"No profile from Wheelkit\"},"
  "{\"id\":\"controls.sensitivity\",\"kind\":\"percent\",\"label\":\"Steering sensitivity\",\"min\":25,\"max\":300,"
  "\"default\":100,\"help\":\"How far the machine steers for a turn of the wheel.\"}],"
  "\"ffb\":[{\"id\":\"ffb.status\",\"kind\":\"status\",\"text\":\"Off\"},"
  "{\"id\":\"ffb.enabled\",\"kind\":\"toggle\",\"label\":\"Force feedback\",\"default\":false,"
  "\"help\":\"The wheel centers more firmly with speed. F8 turns it off.\"},"
  "{\"id\":\"ffb.steering\",\"kind\":\"percent\",\"label\":\"Steering strength\",\"default\":50,"
  "\"help\":\"50%% is the tuned strength; 100%% doubles it.\"}],"
  "\"display\":[{\"id\":\"display.status\",\"kind\":\"status\",\"label\":\"Screens\",\"text\":\"One screen. Three screens are not available yet\"}],"
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
  char pages[sizeof(kPages) + 64], value[32];
  int length = snprintf(pages, sizeof(pages), kPages, version);
  if (!wk.wk_panel_configure(pages, (size_t)length)) {
    fprintf(stderr, "[wheelkit] F6 pages refused: %s\n", wk.wk_last_error());
    return;
  }
  s_panel = true;
  if (wk.wk_settings_get("Wheel", "SteeringSensitivity", value, sizeof(value))) {
    float saved = (float)atof(value);
    if (saved >= 0.25f && saved <= 3.0f) s_sensitivity = saved;
  }
  wk.wk_panel_set_number("controls.sensitivity", s_sensitivity * 100.0);
  const char *profile = wk.wk_input_profile();
  wk.wk_panel_set_text("controls.profile", *profile ? profile : "No profile from Wheelkit: keyboard and gamepad work",
                       *profile ? WK_TONE_GOOD : WK_TONE_WARNING);
  wk.wk_panel_set_text("help.runtime", wk.wk_runtime_version(), WK_TONE_NORMAL);
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
    }
    memset(&e, 0, sizeof(e));
    e.size = sizeof(e);
  }
}

/* A SNES pad has no steering axis: hold Left or Right for the share of frames
 * that matches the wheel's angle (full lock holds it every frame). */
static uint32_t steering_pulses(float steer) {
  float duty = fabsf(steer) * s_sensitivity;
  int direction = steer < 0 ? -1 : steer > 0 ? 1 : 0;
  uint32_t button = direction < 0 ? 0x40u : 0x80u;
  if (!direction) { s_phase = 0; s_direction = 0; return 0; }
  if (direction != s_direction) { s_phase = 0; s_direction = direction; }
  if (duty >= 1.0f) { s_phase = 0; return button; }
  s_phase += (uint32_t)(duty * 65536.0f + 0.5f);
  if (s_phase < 65536u) return 0;
  s_phase -= 65536u;
  return button;
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
 * position (8192 x 4096 units, a quarter metre each), $0BE1 heading (192
 * steps), $C0 the race clock, $0F53 laps timed, $0DC8 rank. */
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
  car.steer = (input & 0x40u) ? -1.0f : (input & 0x80u) ? 1.0f : 0.0f;
  car.yaw = (float)ram[0x0be1] * (6.28318530718f / 192.0f);
  car.position[0] = (float)x * 0.25f;
  car.position[2] = (float)y * 0.25f;
  car.distance = s_car.distance;
  car.race_time = bcd_seconds(ram + 0xc0);
  if (racing && ram[0x0f53] <= 5) car.lap = ram[0x0f53];
  if (racing && ram[0x0dc8] >= 1 && ram[0x0dc8] <= 20) car.race_position = ram[0x0dc8];
  wk.wk_telemetry_frame(&car);
}

/* The old fork's force model (its main branch, src/fzero_ffb.cpp) at the levels
 * the owner drove (40% steering and effects, 20% impact), as Steering strength 50:
 * a spring of 0.4 and a damper of 0.16 the wheel runs itself and a road texture
 * of 0.16 (0.28 on rough ground, $C7) at 18 to 42 Hz, each growing with speed to
 * full at 2 units a frame (30 m/s); a 120 ms knock of 0.2 when the machine loses
 * 16 or more energy ($C9) in a frame without boosting (A). */
static void force(const uint8_t *ram, uint32_t input) {
  static uint16_t energy;
  static bool had;
  static int cooldown;
  bool racing = ram[0x54] == 2 && ram[0x55] >= 3;
  float scale = racing ? fminf(s_car.speed / 30.05f, 1.0f) : 0.0f;
  uint16_t now = read16(ram + 0xc9);
  if (cooldown > 0) --cooldown;
  if (racing && had && !s_car.jumped && energy >= now + 16 && !(input & 0x100u) && !cooldown) {
    wk.wk_ffb_cue(WK_CUE_IMPACT, 0.2f, 120);
    cooldown = 8;
  }
  energy = now;
  had = racing;
  wk.wk_ffb_set_spring(0.4f * scale);
  wk.wk_ffb_set_damper(0.16f * scale);
  wk.wk_ffb_set_texture((ram[0xc7] ? 0.28f : 0.16f) * scale, 18.0f + 24.0f * scale);
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
      if (ram[0x54] == 2 && ram[0x55] >= 3) input |= steering_pulses(in.steer);
      else { s_phase = 0; s_direction = 0; }
      if (!s_driven && input != keyboard) { s_driven = true; wk.wk_log("f-zero: the profile's input reached player 1's controller"); }
    }
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
