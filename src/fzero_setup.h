#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Exact exe-anchored controls-setup bundle only. No device or Core operation.
 * Runtime checks just the qualified manifest and GUI apphost identities. */
int FzeroSetupAvailable(void);
/* Call only after launcher teardown. Starts the GUI with an exact self-process
 * wait identity, then caller must return from main before opening game input. */
int FzeroSetupLaunch(char *why, size_t capacity);
#ifdef __cplusplus
}
#endif
