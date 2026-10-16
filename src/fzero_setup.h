#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Exact exe-anchored controls-setup bundle only. No device or Core operation.
 * Runtime checks just the qualified manifest and GUI apphost identities. */
int FzeroSetupAvailable(void);
/* Optional read-only report window in a separately qualified helper. Linux and
 * old setup bundles return unavailable. Native controls ownership is unchanged. */
int FzeroReportAvailable(void);
int FzeroReportLaunch(const char *version, const char *revision, const char *video_config,
                      char *why, size_t capacity);
/* Call only after launcher teardown. Starts the GUI with an exact self-process
 * wait identity, then caller must return from main before opening game input. */
int FzeroSetupLaunch(char *why, size_t capacity);
#ifdef __cplusplus
}
#endif
