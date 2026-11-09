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
  "reset=guest reset remains a native host hotkey\nhorn=F-Zero has no horn\n"
  "modMenu=the F-Zero launcher is not an in-game mod menu\n"
  "muteFfb=shared input does not own the native force lifecycle\n"
  "gear1=F-Zero has no gearbox\ngear2=F-Zero has no gearbox\n"
  "gear3=F-Zero has no gearbox\ngear4=F-Zero has no gearbox\n"
  "gear5=F-Zero has no gearbox\ngear6=F-Zero has no gearbox\n"
  "gear7=F-Zero has no gearbox\ngear8=F-Zero has no gearbox\n";

const char kFzeroShimExtrasDeclaration[] =
  "game fzero-snes\n"
  "fzero-snes.x digital\n"
  "fzero-snes.y digital\n"
  "fzero-snes.l digital\n"
  "fzero-snes.r digital\n";

const char kFzeroShimHostDeclaration[] =
  "game fzero-snes\n"
  "fzero-snes.x digital\n"
  "fzero-snes.y digital\n"
  "fzero-snes.l digital\n"
  "fzero-snes.r digital\n"
  "fzero-snes.saveMenu digital\n"
  "fzero-snes.rewind digital\n";

int FzeroShimExtrasWord(const dbce_shim_frame *f,
                       const dbce_shim_extras_frame *x, uint32_t *word) {
  return FzeroShimExtrasWordWithHosts(f,x,0,word);
}
int FzeroShimExtrasWordWithHosts(const dbce_shim_frame *f,
                       const dbce_shim_extras_frame *x, int hosts, uint32_t *word) {
  if (word) *word = 0;
  if (!word || !f || !x || f->struct_size != sizeof(*f) || f->abi != DBCE_SHIM_ABI ||
      x->struct_size != sizeof(*x) || x->abi != DBCE_SHIM_EXTRAS_ABI ||
      (hosts != 0 && hosts != 1) || x->seq != f->seq ||
      x->count != (hosts ? 6u : 4u) || x->contexts != 0) return 0;
  static const unsigned bits[] = {9, 1, 10, 11, 24, 25};
  uint32_t result = 0;
  for (unsigned i = 0; i < x->count; ++i) {
    const dbce_shim_action *a = &x->slot[i];
    // Ordinary extras never admit injected provenance; device faults and
    // rearming stay neutral. The six-slot opt-in adds only host bits 24/25.
    if (a->injected || !isfinite(a->value) || a->value < 0 || a->value > 1) return 0;
    if (a->status == 12 && a->value > .5f) result |= 1u << bits[i];
  }
  *word = result;
  return 1;
}

void FzeroShimMapInit(FzeroShimMap *s, int range, int response, float threshold) {
  memset(s, 0, sizeof(*s));
  s->range_percent = range;
  s->response_percent = response;
  s->pedal_threshold = isfinite(threshold) && threshold >= 0 && threshold <= 1 ? threshold : .5f;
}
void FzeroShimMapNeutral(FzeroShimMap *s) { FzeroAnalogSteeringReset(&s->steering); }
int FzeroShimGameplayState(const uint8_t *ram, size_t ram_size) {
  return ram && ram_size > 0x55 && ram[0x54] == 2 && ram[0x55] >= 3;
}
static float applied(const dbce_shim_action *a, int signed_axis) {
  /* Status::Live is 12 in the pinned ABI; every other status is neutral. */
  if (a->status != 12 || !isfinite(a->value)) return 0;
  float low = signed_axis ? -1.f : 0.f;
  return a->value < low ? low : a->value > 1.f ? 1.f : a->value;
}
static int valid_frame(FzeroShimMap *s, const dbce_shim_frame *f, uint32_t *word) {
  if (word) *word = 0;
  if (!s || !f || !word) return 0;
  if (f->struct_size != sizeof(*f) || f->abi != DBCE_SHIM_ABI ||
      (s->have_seq && f->seq <= s->last_seq)) {
    FzeroShimMapNeutral(s);
    return 0;
  }
  s->last_seq = f->seq; s->have_seq = 1;
  return 1;
}
static uint32_t digital_word(const dbce_shim_frame *f) {
  uint32_t word = 0;
  static const unsigned bits[] = {8, 0, 3, 2, 4, 5, 6, 7};
  for (int i = 0; i < 8; ++i)
    if (applied(&f->action[9+i], 0) > .5f) word |= 1u << bits[i];
  return word;
}
int FzeroShimMapFrame(FzeroShimMap *s, const dbce_shim_frame *f, uint32_t *word,
                      int gameplay) {
  if (!valid_frame(s,f,word)) return 0;
  if (gameplay) {
    float steer = applied(&f->action[0], 1);
    int axis = (int)roundf(steer * (steer < 0 ? 32768.f : 32767.f));
    /* Core already applied the profile's calibrated deadzone. Preserve the
     * game's independent range/response curve and PDM, without a second zone. */
    *word = FzeroAnalogSteeringReadTuned(&s->steering, axis, 0,
                                        s->range_percent, s->response_percent);
  } else {
    FzeroShimMapNeutral(s);
  }
  if (applied(&f->action[1], 0) > s->pedal_threshold) *word |= 1u;  /* SNES B / gas */
  if (applied(&f->action[2], 0) > s->pedal_threshold) *word |= 2u;  /* SNES Y / brake */
  *word |= digital_word(f);
  return 1;
}
int FzeroShimMapOverlayFrame(FzeroShimMap *s, const dbce_shim_frame *f, uint32_t *word) {
  if (!valid_frame(s,f,word)) return 0;
  FzeroShimMapNeutral(s);
  *word = digital_word(f);
  return 1;
}
void FzeroShimConsumedValues(uint64_t frame, uint32_t input, double out[2]) {
  out[0] = (double)frame;
  out[1] = (double)input;
}
int FzeroShimMenuValues(const uint8_t *ram, size_t ram_size,
                       double out[FZERO_SHIM_MENU_VALUES]) {
  if (!out) return 0;
  memset(out, 0, sizeof(double) * FZERO_SHIM_MENU_VALUES);
  if (!ram || ram_size < 0x14c9a) return 0;
  /* Native title/course state in bank 7E and the pinned Deluxe selector in
   * bank 7F. No guesses/default filling for captures without full WRAM. */
  static const unsigned bytes[] = {0x5a,0x58,0x60,0x52,0x53,0x90};
  static const unsigned words[] = {0x14c84,0x14c8a,0x14c92,0x14c98};
  for (unsigned i=0;i<6;++i) out[i]=ram[bytes[i]];
  for (unsigned i=0;i<4;++i)
    out[6+i]=(unsigned)ram[words[i]] | ((unsigned)ram[words[i]+1]<<8);
  out[10]=ram[0x14c27]; out[11]=ram[0x5b];
  return 1;
}
uint32_t FzeroShimGuestWord(uint32_t input) {
  input &= ~FZERO_HOST_BUTTON_MASK;
  const uint32_t pairs[] = {0x30u, 0xc0u, 0x30000u, 0xc0000u};
  for (unsigned i = 0; i < 4; ++i)
    if ((input & pairs[i]) == pairs[i]) input ^= pairs[i];
  return input;
}
int FzeroControlsStartupMayWrite(int script, int replay, int shim) {
  return !script && !replay && !shim;
}
