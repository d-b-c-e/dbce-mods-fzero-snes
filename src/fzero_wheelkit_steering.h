#pragma once

/* The wheel's angle as the SNES pad's Left and Right (fzero_wheelkit.c). A SNES pad has no steering axis, so the
 * direction is held for the share of frames that matches the angle, spread evenly over the frames (pulse density);
 * full lock holds it every frame. This is the old fork's mapping (its main branch, src/fzero_analog.c) at the tune the
 * owner drove (config.ini [Controller.<wheel>] Deadzone = 3, SteeringRangePercent = 100,
 * SteeringResponsePercent = 50): a dead zone at the centre, the rest of the travel to full lock scaled by the
 * sensitivity (the old range: sensitivity 1 is 100%, 2 reaches full lock at half the travel), then a square-root
 * curve, so small corrections count for more and the slope tapers toward full lock. steer is the runtime's -1..1,
 * which already spans the rig profile's measured travel. Pure, so tests/fzero_wheelkit_steering_tests.c checks it
 * against values computed with the old code. */

#include <math.h>
#include <stdint.h>

#define FZERO_STEER_DEAD_ZONE 0.03f   /* the old Deadzone = 3 (percent of half the travel) */
#define FZERO_STEER_CURVE 0.5         /* the old SteeringResponsePercent = 50: the duty is the square root */

typedef struct FzeroSteer {
  uint32_t phase;   /* 16.16: one frame's press is 65536 */
  int direction;    /* -1 left, 1 right, 0 centred */
} FzeroSteer;

/* v within lo..hi; not a number gives fallback. */
static inline float FzeroSteerClamp(float v, float lo, float hi, float fallback) {
  return v < lo ? lo : v > hi ? hi : v == v ? v : fallback;
}

/* The share of frames (0..1) the direction is held at this angle. Sensitivity 0.25..3 (F6's 25..300%), dead zone
 * 0..0.2 (F6's 0..20%). */
static inline double FzeroSteerDuty(float steer, float sensitivity, float dead_zone) {
  sensitivity = FzeroSteerClamp(sensitivity, 0.25f, 3.0f, 1.0f);
  dead_zone = FzeroSteerClamp(dead_zone, 0.0f, 0.2f, FZERO_STEER_DEAD_ZONE);
  double magnitude = fabs((double)steer);
  if (!(magnitude > dead_zone)) return 0.0;  /* also not finite */
  double travel = (magnitude - dead_zone) / (1.0 - dead_zone) * sensitivity;
  if (travel > 1.0) travel = 1.0;
  return pow(travel, FZERO_STEER_CURVE);
}

/* One simulated frame: 0x40 (Left), 0x80 (Right) or 0. Back at the centre or on a change of side the pending share
 * is dropped, as the old code did. */
static inline uint32_t FzeroSteerPulse(FzeroSteer *s, float steer, float sensitivity, float dead_zone) {
  double duty = FzeroSteerDuty(steer, sensitivity, dead_zone);
  int direction = duty > 0.0 ? (steer < 0.0f ? -1 : 1) : 0;
  if (!direction) { s->phase = 0; s->direction = 0; return 0; }
  if (direction != s->direction) { s->phase = 0; s->direction = direction; }
  const uint32_t button = direction < 0 ? 0x40u : 0x80u;
  uint32_t level = (uint32_t)(duty * 65536.0 + 0.5);
  if (level >= 65536u) { s->phase = 0; return button; }
  s->phase += level;
  if (s->phase < 65536u) return 0;
  s->phase -= 65536u;
  return button;
}
