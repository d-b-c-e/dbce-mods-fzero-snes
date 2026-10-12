#include "fzero_shim_map.h"
#include <math.h>
#include <string.h>

const char *const kFzeroShimActions[DBCE_SHIM_ACTIONS] = {
  "steer", "throttle", "brake", "clutch", "handbrake", "shiftUp", "shiftDown",
  "neutral", "reverse", "confirm", "back", "start", "select", "navUp", "navDown",
  "navLeft", "navRight", "camera", "lookBack", "reset", "horn", "modMenu", "muteFfb",
  "gear1", "gear2", "gear3", "gear4", "gear5", "gear6", "gear7", "gear8"
};
const char kFzeroShimUnsupported[] =
  "clutch=F-Zero has no clutch\nhandbrake=F-Zero has no handbrake\n"
  "shiftUp=F-Zero has no gearbox\nshiftDown=F-Zero has no gearbox\n"
  "neutral=F-Zero has no gearbox\nreverse=F-Zero has no reverse gear\n"
  "camera=no generic F-Zero camera action\nlookBack=no F-Zero look-back action\n"
  "reset=guest reset is excluded from this diagnostic\nhorn=F-Zero has no horn\n"
  "modMenu=the F-Zero launcher is not an in-game mod menu\n"
  "muteFfb=physical force is latched off in this diagnostic\n"
  "gear1=F-Zero has no gearbox\ngear2=F-Zero has no gearbox\n"
  "gear3=F-Zero has no gearbox\ngear4=F-Zero has no gearbox\n"
  "gear5=F-Zero has no gearbox\ngear6=F-Zero has no gearbox\n"
  "gear7=F-Zero has no gearbox\ngear8=F-Zero has no gearbox\n";

void FzeroShimMapInit(FzeroShimMap *s, int range, int response, float threshold) {
  memset(s, 0, sizeof(*s));
  s->range_percent = range;
  s->response_percent = response;
  s->pedal_threshold = isfinite(threshold) && threshold >= 0 && threshold <= 1 ? threshold : .5f;
}
void FzeroShimMapNeutral(FzeroShimMap *s) { FzeroAnalogSteeringReset(&s->steering); }
static float applied(const dbce_shim_action *a, int signed_axis) {
  /* Status::Live is 12 in the pinned ABI; every other status is neutral. */
  if (a->status != 12 || !isfinite(a->value)) return 0;
  float low = signed_axis ? -1.f : 0.f;
  return a->value < low ? low : a->value > 1.f ? 1.f : a->value;
}
int FzeroShimMapFrame(FzeroShimMap *s, const dbce_shim_frame *f, uint32_t *word) {
  if (word) *word = 0;
  if (!s || !f || !word) return 0;
  if (f->struct_size != sizeof(*f) || f->abi != DBCE_SHIM_ABI ||
      (s->have_seq && f->seq <= s->last_seq)) {
    FzeroShimMapNeutral(s);
    return 0;
  }
  s->last_seq = f->seq; s->have_seq = 1;
  float steer = applied(&f->action[0], 1);
  int axis = (int)roundf(steer * (steer < 0 ? 32768.f : 32767.f));
  /* Core already applied the profile's calibrated deadzone. Preserve the
   * game's independent range/response curve and PDM, without a second zone. */
  *word = FzeroAnalogSteeringReadTuned(&s->steering, axis, 0,
                                      s->range_percent, s->response_percent);
  if (applied(&f->action[1], 0) > s->pedal_threshold) *word |= 1u;  /* SNES B / gas */
  if (applied(&f->action[2], 0) > s->pedal_threshold) *word |= 2u;  /* SNES Y / brake */
  static const unsigned bits[] = {8, 0, 3, 2, 4, 5, 6, 7};
  for (int i = 0; i < 8; ++i)
    if (applied(&f->action[9+i], 0) > .5f) *word |= 1u << bits[i];
  return 1;
}
void FzeroShimConsumedValues(uint64_t frame, uint32_t input, double out[2]) {
  out[0] = (double)frame;
  out[1] = (double)input;
}
int FzeroControlsStartupMayWrite(int script, int replay, int shim) {
  return !script && !replay && !shim;
}
