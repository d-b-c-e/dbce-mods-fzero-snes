#include "fzero_output_latch.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static volatile LONG s_state;   /* 0 undecided, 1 no force (test injection), 2 force output */
static int take(LONG want) {
  const LONG prev = InterlockedCompareExchange(&s_state, want, 0);
  return prev == 0 || prev == want;
}
int FzeroNoForceLatched(void) { return InterlockedCompareExchange(&s_state, 0, 0) == 1; }
#else
static int s_state;
static int take(int want) {
  if (!s_state) s_state = want;
  return s_state == want;
}
int FzeroNoForceLatched(void) { return s_state == 1; }
#endif

int FzeroLatchNoForce(void) { return take(1); }
int FzeroLatchForceOutput(void) { return take(2); }
