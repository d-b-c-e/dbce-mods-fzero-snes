#pragma once

/* Test injection for the raw wheel (dbce-wheel-mod-toolkit STD-033 section 6), development only.
 *
 * F-Zero reads the wheel through SDL, which the DirectInput proxies' force fence cannot reach, so the samples are
 * substituted in our own reader (fzero_gamepad.c), before the [Controller.<guid>] bindings turn them into SNES buttons.
 * It arms at start only when all of these hold, and stays off otherwise:
 *   - %LOCALAPPDATA%\dbce\fzero\inject.on exists;
 *   - config.ini [ForceFeedback] Enabled is 0, so no force runs in this process at all;
 *   - the [Controls] profile is applied: [ControlsApplied] Revision is the profile's, its Device the wheel's SDL GUID.
 * Commands come from %LOCALAPPDATA%\dbce\fzero\inject.txt (re-read when it changes), one per line, the toolkit grammar:
 *   inject raw <axis|button|hat> <index> [angle] dev=<instance> value=<raw> ms=<50-15000>
 *   inject action <id> <n> ms=<50-15000>      (resolved through the applied [Controls])
 * dev= is the profile's DirectInput instance (its steering binding's dev): the one controller F-Zero reads. Axis values
 * are in the binding's range (default 0..65535); SDL's axis is that minus 32768, the R12's numbering being qualified as
 * equal in both (fzero_controls.cpp). */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void FzeroInjectInit(const char *config_path);
int FzeroInjectArmed(void);

/* Reads new commands when the file changed (at most every 100 ms). */
void FzeroInjectPoll(void);

/* One raw-wheel read of the joystick with this SDL GUID text: axes in SDL units, buttons 0/1, hats SDL bits, each as
 * many as the stick has. Running samples replace their objects. Returns how many objects carried one. */
int FzeroInjectRawRead(const char *sdl_guid, int16_t *axes, int axis_count, unsigned char *buttons, int button_count,
                       uint8_t *hats, int hat_count);

/* Tests only: arm without files (device = the wheel's SDL GUID, lines = the [Controls] body), feed one command line, and
 * run with an explicit clock. */
int FzeroInjectTestArm(const char *device, const char *const *controls_lines, int count);
int FzeroInjectTestCommand(const char *line, char *why, int why_size);
void FzeroInjectTestClock(uint64_t now_ms);

#ifdef __cplusplus
}
#endif
