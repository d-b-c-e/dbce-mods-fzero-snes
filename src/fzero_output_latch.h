#pragma once

/* One process-wide fact shared by the force path (fzero_ffb.cpp) and the dev-only test injection (fzero_inject.h):
 * whichever comes first wins and it never clears. A test injection requested at start (inject.on present, armed or
 * not) means no force in this process; force output started means no test injection. */

#ifdef __cplusplus
extern "C" {
#endif

/* 1: this process now runs no force (or already did). 0: force output already started; refuse the test request. */
int FzeroLatchNoForce(void);

/* 1: force output may start (and from now on no test injection arms). 0: a test injection was requested. */
int FzeroLatchForceOutput(void);

int FzeroNoForceLatched(void);

#ifdef __cplusplus
}
#endif
