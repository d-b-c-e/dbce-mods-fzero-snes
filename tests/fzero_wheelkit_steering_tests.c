/* The wheel-to-pad mapping (src/fzero_wheelkit_steering.h) against the old fork's (its main branch,
 * src/fzero_analog.c). Each row was computed with that code, compiled unchanged: the runtime's steer value as the old
 * signed axis (lround(steer x 32767, or 32768 to the left)), the old dead zone ((percent x 32767 + 50) / 100), the old
 * SteeringRangePercent as 100 / sensitivity, SteeringResponsePercent 50; the duty, the presses in 600 frames from
 * rest, and the first 24 frames (bit n: pressed in frame n). */
#include "../src/fzero_wheelkit_steering.h"

#include <math.h>
#include <stdio.h>

static const struct { float steer, sensitivity, dead_zone; double duty; unsigned presses, pattern; } kOld[] = {
  /* The owner's tune: Deadzone 3, SteeringRangePercent 100. */
  {0.000f, 1.00f, 0.03f, 0.000000, 0, 0x000000u},
  {0.020f, 1.00f, 0.03f, 0.000000, 0, 0x000000u},
  {0.030f, 1.00f, 0.03f, 0.000000, 0, 0x000000u},
  {0.031f, 1.00f, 0.03f, 0.032222, 19, 0x000000u},
  {0.050f, 1.00f, 0.03f, 0.143554, 86, 0x102040u},
  {-0.050f, 1.00f, 0.03f, 0.143552, 86, 0x102040u},
  {0.100f, 1.00f, 0.03f, 0.268653, 161, 0x444888u},
  {-0.100f, 1.00f, 0.03f, 0.268649, 161, 0x444888u},
  {0.250f, 1.00f, 0.03f, 0.476248, 285, 0x955554u},
  {-0.250f, 1.00f, 0.03f, 0.476241, 285, 0x955554u},
  {0.500f, 1.00f, 0.03f, 0.696098, 417, 0x76ddb6u},
  {-0.500f, 1.00f, 0.03f, 0.696087, 417, 0x76ddb6u},
  {0.750f, 1.00f, 0.03f, 0.861545, 516, 0xdfbf7eu},
  {0.900f, 1.00f, 0.03f, 0.947047, 568, 0xfbfffeu},
  {-0.990f, 1.00f, 0.03f, 0.994827, 596, 0xfffffeu},
  {1.000f, 1.00f, 0.03f, 1.000000, 600, 0xffffffu},
  {-1.000f, 1.00f, 0.03f, 1.000000, 600, 0xffffffu},
  /* No dead zone. */
  {0.000f, 1.00f, 0.00f, 0.000000, 0, 0x000000u},
  {0.020f, 1.00f, 0.00f, 0.141385, 84, 0x204080u},
  {0.050f, 1.00f, 0.00f, 0.223583, 134, 0x422110u},
  {-0.100f, 1.00f, 0.00f, 0.316237, 189, 0x449248u},
  {0.250f, 1.00f, 0.00f, 0.500008, 300, 0xaaaaaau},
  {-0.500f, 1.00f, 0.00f, 0.707107, 424, 0x6ddbb6u},
  {0.900f, 1.00f, 0.00f, 0.948678, 569, 0xf7fffeu},
  /* Sensitivity 125% (range 80) and 200% (range 50). */
  {0.031f, 1.25f, 0.03f, 0.036025, 21, 0x000000u},
  {0.100f, 1.25f, 0.03f, 0.300365, 180, 0x892248u},
  {-0.250f, 1.25f, 0.03f, 0.532453, 319, 0x55aaaau},
  {0.500f, 1.25f, 0.03f, 0.778264, 466, 0xbbddeeu},
  {0.750f, 1.25f, 0.03f, 0.963241, 577, 0xfffffeu},
  {0.900f, 1.25f, 0.03f, 1.000000, 600, 0xffffffu},
  {0.050f, 2.00f, 0.03f, 0.203017, 121, 0x084210u},
  {-0.100f, 2.00f, 0.03f, 0.379933, 227, 0xa4a4a4u},
  {0.250f, 2.00f, 0.03f, 0.673516, 404, 0xdb6db6u},
  {-0.500f, 2.00f, 0.03f, 0.984431, 590, 0xfffffeu},
  {0.750f, 2.00f, 0.03f, 1.000000, 600, 0xffffffu},
  /* Dead zone 10%. */
  {0.100f, 1.00f, 0.10f, 0.000000, 0, 0x000000u},
  {0.250f, 1.00f, 0.10f, 0.408248, 244, 0x4a5294u},
  {0.500f, 1.00f, 0.10f, 0.666675, 400, 0xdb6db6u},
  {0.750f, 1.00f, 0.10f, 0.849830, 509, 0xf7dfbeu},
  {-1.000f, 1.00f, 0.10f, 1.000000, 600, 0xffffffu},
};

static int failures, checks;
static void check(int ok, const char *what, float steer, float sensitivity, float dead_zone) {
  ++checks;
  if (ok) return;
  ++failures;
  printf("FAIL %s at steer %.3f, sensitivity %.2f, dead zone %.2f\n", what, steer, sensitivity, dead_zone);
}

int main(void) {
  for (size_t i = 0; i < sizeof kOld / sizeof kOld[0]; ++i) {
    const float s = kOld[i].steer, k = kOld[i].sensitivity, z = kOld[i].dead_zone;
    /* The old code worked in whole axis units, so the duty may differ in the fifth decimal. */
    check(fabs(FzeroSteerDuty(s, k, z) - kOld[i].duty) < 2e-4, "duty as the old code", s, k, z);
    FzeroSteer st = {0, 0};
    unsigned presses = 0, pattern = 0, wrong = 0;
    for (int f = 0; f < 600; ++f) {
      uint32_t b = FzeroSteerPulse(&st, s, k, z);
      if (b) ++presses;
      if (b && f < 24) pattern |= 1u << f;
      if (b && b != (s < 0 ? 0x40u : 0x80u)) ++wrong;
    }
    check(presses + 1 >= kOld[i].presses && presses <= kOld[i].presses + 1, "presses in 600 frames as the old code (+-1)", s, k, z);
    check(pattern == kOld[i].pattern, "the first 24 frames as the old code", s, k, z);
    check(!wrong, "only the side the wheel is turned to", s, k, z);
  }
  /* Back at the centre and on a change of side the pending share is dropped, as the old code did. */
  FzeroSteer st = {0, 0};
  FzeroSteerPulse(&st, 0.5f, 1.0f, 0.03f);
  check(st.phase != 0 && st.direction == 1, "a share pending after one frame at half lock", 0.5f, 1.0f, 0.03f);
  check(FzeroSteerPulse(&st, 0.01f, 1.0f, 0.03f) == 0 && st.phase == 0 && st.direction == 0, "inside the dead zone: nothing, cleared", 0.01f, 1.0f, 0.03f);
  FzeroSteerPulse(&st, 0.5f, 1.0f, 0.03f);
  check(FzeroSteerPulse(&st, -0.2f, 1.0f, 0.03f) == 0 && st.direction == -1, "a change of side starts from nothing", -0.2f, 1.0f, 0.03f);
  /* Out of range and not a number. */
  check(FzeroSteerDuty(NAN, 1.0f, 0.03f) == 0.0, "not a number steers nowhere", 0, 1.0f, 0.03f);
  check(fabs(FzeroSteerDuty(0.5f, NAN, NAN) - FzeroSteerDuty(0.5f, 1.0f, 0.03f)) < 1e-12, "not a number takes the defaults", 0.5f, 0, 0);
  check(FzeroSteerDuty(0.2f, 9.0f, 0.03f) == FzeroSteerDuty(0.2f, 3.0f, 0.03f), "sensitivity at most 300%", 0.2f, 9.0f, 0.03f);
  check(FzeroSteerDuty(0.2f, 0.1f, 0.03f) == FzeroSteerDuty(0.2f, 0.25f, 0.03f), "sensitivity at least 25%", 0.2f, 0.1f, 0.03f);
  check(FzeroSteerDuty(0.19f, 1.0f, 0.5f) == 0.0 && FzeroSteerDuty(0.21f, 1.0f, 0.5f) > 0.0, "dead zone at most 20%", 0.21f, 1.0f, 0.5f);
  check(fabs(FzeroSteerDuty(0.5f, 0.5f, 0.0f) - sqrt(0.25)) < 1e-9, "sensitivity 50%: half lock is a quarter of the travel", 0.5f, 0.5f, 0.0f);
  printf("fzero wheelkit steering: %d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
