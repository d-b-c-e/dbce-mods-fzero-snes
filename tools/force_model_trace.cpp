// Synthetic component response only. Compiles out native output/discovery.
// No ROM, game window, controller, or physical speed claim.
#include "fzero_ffb.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void put16(uint8_t *p, unsigned v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

int main(int argc, char **argv) {
  if (argc != 3 && argc != 4) { std::fprintf(stderr, "Usage: fzero_force_trace new.csv legacy-strength-0..100 [steering-strength-0..100]\n"); return 2; }
  char *end = nullptr;
  long strength = std::strtol(argv[2], &end, 10);
  if (!end || end == argv[2] || *end || strength < 0 || strength > 100) return 2;
  long steering = strength;
  if (argc == 4) {
    steering = std::strtol(argv[3], &end, 10);
    if (!end || end == argv[3] || *end || steering < 0 || steering > 100) return 2;
  }
  FILE *file = std::fopen(argv[1], "wx"); // exclusive; never replace an original/candidate
  if (!file) { std::perror("new trace"); return 2; }
  std::fputs("time_s,epoch,valid,spring,damper,road,fallback_constant,collision_edge\n", file);
  uint8_t ram[0x20000]{};
  FzeroFfbState state{};
  FzeroFfbOutput output{};
  unsigned x = 100;
  for (int frame = 0; frame <= 600; ++frame) {
    bool racing = frame >= 60 && frame < 570;
    ram[0x54] = racing ? 2 : 0; ram[0x55] = racing ? 3 : 0;
    if (frame >= 120 && frame < 540) x = (x + 2) & 8191;
    put16(ram + 0xb70, x); put16(ram + 0xb90, 100);
    put16(ram + 0xc9, frame < 510 ? 2000 : 1900);
    ram[0xc7] = frame >= 480 && frame < 540 ? 1 : 0;
    unsigned input = frame >= 240 && frame < 360 ? 0x40 : frame >= 360 && frame < 480 ? 0x80 : 0;
    FzeroFfbComputeSteering(&state, ram, sizeof(ram), input, (int)strength, (int)steering, &output);
    std::fprintf(file, "%.9f,0,1,%.8f,%.8f,%.8f,%.8f,%d\n", frame / 60.0,
      output.spring_coefficient / 10000.0, output.damper_coefficient / 10000.0,
      output.road_magnitude / 10000.0, output.constant_force / 10000.0, output.collision_pulse);
  }
  bool ok = !std::ferror(file);
  ok = std::fclose(file) == 0 && ok;
  if (!ok) return 1;
  std::puts("601 synthetic model rows; no device backend compiled. Fallback constant is not simultaneous with spring. Collision is an event edge, not a force amplitude.");
  return 0;
}
