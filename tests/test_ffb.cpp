#include "fzero_ffb.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%d: %s\n", __LINE__, #x); std::exit(1); } } while (0)

static void put16(uint8_t *p, uint16_t value) {
  p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
}

int main() {
  // Compare identical trajectories under legacy, equivalent split, and zero
  // steering. Turning steering off must not erase road, damping or impacts.
  {
    uint8_t track[0x20000]{};
    FzeroFfbState legacy{}, equivalent{}, zero{};
    FzeroFfbOutput a{}, b{}, c{};
    track[0x54] = 2; track[0x55] = 3;
    int contacts = 0;
    for (int frame = 0; frame < 600; ++frame) {
      put16(track + 0xb70, (uint16_t)(frame * 2));
      put16(track + 0xc9, frame < 300 ? 2000 : 1900);
      track[0xc7] = frame >= 200 ? 1 : 0;
      if (frame > 570) track[0x55] = 0;
      unsigned input = frame % 60 < 30 ? 0x40 : 0x80;
      FzeroFfbCompute(&legacy, track, sizeof(track), input, 40, &a);
      FzeroFfbComputeSteering(&equivalent, track, sizeof(track), input, 40, 40, &b);
      FzeroFfbComputeSteering(&zero, track, sizeof(track), input, 40, 0, &c);
      CHECK(!std::memcmp(&a, &b, sizeof(a)));
      CHECK(!std::memcmp(&legacy, &equivalent, sizeof(legacy)) && !std::memcmp(&legacy, &zero, sizeof(legacy)));
      CHECK(c.constant_force == 0 && c.spring_coefficient == 0);
      CHECK(a.damper_coefficient == c.damper_coefficient && a.road_magnitude == c.road_magnitude);
      CHECK(a.road_frequency_millihz == c.road_frequency_millihz && a.collision_pulse == c.collision_pulse && a.racing == c.racing);
      contacts += c.collision_pulse;
      if (frame == 250) CHECK(a.spring_coefficient > 0 && c.road_magnitude > 0 && c.damper_coefficient > 0);
    }
    CHECK(contacts == 1);
  }
  uint8_t ram[0x20000]{};
  FzeroFfbState state{};
  FzeroFfbOutput out{};
  ram[0x54] = 2; ram[0x55] = 3;
  put16(ram + 0xc9, 2000);
  put16(ram + 0x0b70, 0x1ffe); put16(ram + 0x0b90, 100);
  FzeroFfbCompute(&state, ram, sizeof(ram), 0, 40, &out);
  CHECK(out.racing && out.constant_force == 0);
  put16(ram + 0x0b70, 2); /* wrapped movement, not a huge speed spike */
  FzeroFfbCompute(&state, ram, sizeof(ram), 0x0040, 40, &out);
  CHECK(out.constant_force > 0 && out.constant_force <= 10000);
  CHECK(out.road_magnitude > 0 && out.road_magnitude <= 10000);
  CHECK(out.spring_coefficient > 0 && out.spring_coefficient <= 10000);
  CHECK(out.damper_coefficient > 0 && out.damper_coefficient <= 10000);
  /* Typical race movement must exceed the toolkit's condition-update step
   * at the default 35% strength, or no centering reaches the wheel. */
  FzeroFfbState race_state{};
  put16(ram + 0x0b70, 2312); put16(ram + 0x0b90, 344);
  FzeroFfbCompute(&race_state, ram, sizeof(ram), 0, 35, &out);
  put16(ram + 0x0b70, 2314);
  FzeroFfbCompute(&race_state, ram, sizeof(ram), 0, 35, &out);
  CHECK(out.spring_coefficient > 500);
  for (int i = 1; i <= 8; ++i) {
    put16(ram + 0x0b70, (uint16_t)(2314 + 2 * i));
    FzeroFfbCompute(&race_state, ram, sizeof(ram), 0, 35, &out);
  }
  CHECK(out.damper_coefficient > 500);
  put16(ram + 0x0b70, 2332);
  FzeroFfbCompute(&race_state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.road_magnitude >= 400 && out.road_magnitude <= 500);
  /* Animation RAM changes ahead of contact are not a collision signal. */
  ram[0xe0] = 1;
  FzeroFfbCompute(&state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.collision_pulse == 0);
  put16(ram + 0xc9, 1995);
  FzeroFfbCompute(&state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.collision_pulse == 0);
  put16(ram + 0xc9, 1899);
  FzeroFfbCompute(&state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.collision_pulse == 1);
  FzeroFfbCompute(&state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.collision_pulse == 0);
  put16(ram + 0xc9, 1803);
  FzeroFfbCompute(&state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.collision_pulse == 0); /* same contact's aftershock */
  for (int i = 0; i < 8; ++i)
    FzeroFfbCompute(&state, ram, sizeof(ram), 0, 12, &out);
  put16(ram + 0xc9, 1707);
  FzeroFfbCompute(&state, ram, sizeof(ram), 0x0100, 12, &out);
  CHECK(out.collision_pulse == 0); /* boost energy spend */
  put16(ram + 0xc9, 1611);
  FzeroFfbCompute(&state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.collision_pulse == 1);
  /* Three isolated 24-unit drops in the recorded drive were below the old
   * 32-unit threshold. Recover them without treating drain or boost as hits. */
  FzeroFfbState light_state{};
  put16(ram + 0xc9, 2000);
  FzeroFfbCompute(&light_state, ram, sizeof(ram), 0, 12, &out);
  put16(ram + 0xc9, 1995);
  FzeroFfbCompute(&light_state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.collision_pulse == 0);
  put16(ram + 0xc9, 1980);
  FzeroFfbCompute(&light_state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.collision_pulse == 0);
  put16(ram + 0xc9, 1956);
  FzeroFfbCompute(&light_state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.collision_pulse == 1);
  for (int i = 0; i < 8; ++i)
    FzeroFfbCompute(&light_state, ram, sizeof(ram), 0, 12, &out);
  put16(ram + 0xc9, 1932);
  FzeroFfbCompute(&light_state, ram, sizeof(ram), 0x0100, 12, &out);
  CHECK(out.collision_pulse == 0);
  put16(ram + 0xc9, 1908);
  FzeroFfbCompute(&light_state, ram, sizeof(ram), 0, 12, &out);
  CHECK(out.collision_pulse == 1);
  ram[0x55] = 0;
  FzeroFfbCompute(&state, ram, sizeof(ram), 0x0040, 100, &out);
  CHECK(!out.racing && out.constant_force == 0 && out.road_magnitude == 0);
  CHECK(out.spring_coefficient == 0 && out.damper_coefficient == 0);
  FzeroFfbCompute(&state, ram, 12, 0, 40, &out);
  CHECK(!out.racing);

#if defined(_WIN32) && !defined(FZERO_FFB_MODEL_ONLY)
  char devices[16][256]{};
  int device_count = FzeroFfbListDevices(devices, 16);
  CHECK(device_count >= 0 && device_count <= 16);
  for (int i = 0; i < device_count; ++i)
    std::printf("FFB device %d: %s\n", i, devices[i]);
  /* The optional runtime must remain harmless when its configured wheel is
   * absent (and also when the DLL was not staged for this test target). */
  const char *config_path = "fzero_ffb_test.ini";
  FILE *config = std::fopen(config_path, "wb");
  CHECK(config != nullptr);
  std::fputs("[ForceFeedback]\nEnabled=1\nDevice=definitely-not-a-wheel\n",
             config);
  std::fclose(config);
  FzeroFfbInit(config_path, nullptr);
  FzeroFfbShutdown();
  FzeroFfbShutdown();
  std::remove(config_path);
#endif
  std::puts("F-Zero force-feedback model tests passed");
  return 0;
}
