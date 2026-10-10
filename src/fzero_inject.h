#pragma once

/* Test injection for the raw wheel (dbce-wheel-mod-toolkit STD-033 section 6), development only.
 *
 * F-Zero reads the wheel through SDL, which the DirectInput proxies' force fence cannot reach, so the samples are
 * substituted in our own reader (fzero_gamepad.c), before the [Controller.<guid>] bindings turn them into SNES buttons.
 * It arms at start only when all of these hold, and stays off otherwise:
 *   - %LOCALAPPDATA%\dbce\fzero\inject.on exists and names a session: "nonce=<8-64 letters/digits>" and
 *     "expires=<unix seconds, UTC>" no more than an hour ahead;
 *   - config.ini [ForceFeedback] Enabled is 0;
 *   - the [Controls] profile is applied: [ControlsApplied] Revision is the profile's, its Device the wheel's SDL GUID.
 * inject.on present at start latches "no force" for the whole process (fzero_output_latch.h), whether or not the
 * session then arms: a later FzeroFfbInit refuses, whatever config.ini says by then. Force already started refuses
 * the request.
 * Commands come from %LOCALAPPDATA%\dbce\fzero\inject.txt (read once each time it changes, at most 4096 bytes). Its
 * first line is "nonce=<the session's>"; then at most 32 commands, the toolkit grammar:
 *   inject raw <axis|button|hat> <index> [angle] dev=<instance> value=<raw> ms=<50-15000>
 *   inject action <id> <n> ms=<50-15000>      (resolved through the applied [Controls])
 * A session takes at most 2000 commands and ends at its expiry: running samples are dropped then, as they are when the
 * wheel closes or disconnects. dev= is the profile's DirectInput instance (its steering binding's dev): the one
 * controller F-Zero reads. Axis values are in the binding's range (default 0..65535); SDL's axis is that minus 32768,
 * the R12's numbering being qualified as equal in both (fzero_controls.cpp). */

#include <stddef.h>
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

/* The raw wheel was closed (detached, replaced or shut down): its running samples never reach a reopened stick. */
void FzeroInjectDeviceClosed(void);

/* Tests only: arm without files (device = the wheel's SDL GUID, lines = the [Controls] body; session nonce "test",
 * no expiry), feed one command line or a whole command file, end the session at a time, read an inject.on text, and
 * run with an explicit clock. */
int FzeroInjectTestArm(const char *device, const char *const *controls_lines, int count);
int FzeroInjectTestCommand(const char *line, char *why, int why_size);
int FzeroInjectTestFile(const char *text, size_t size, char *why, int why_size);
void FzeroInjectTestExpire(uint64_t expires_ms);
int FzeroInjectTestSession(const char *text, uint64_t now_unix, char *why, int why_size);
void FzeroInjectTestClock(uint64_t now_ms);

#ifdef __cplusplus
}
#endif
