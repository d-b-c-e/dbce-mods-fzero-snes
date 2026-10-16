#pragma once
#include "fzero_video.h"
#include "recomp_launcher.h"
const RecompLauncherCModProvider *FzeroModsProvider(FzeroVideoSettings *settings,
                                                   const char *path);
const RecompLauncherCModProvider *FzeroModsProviderWheel(
    FzeroVideoSettings *settings, const char *video_path,
    const char *config_path, const char *wheel_guid,
    void (*write_ini)(const char *, const char *, const char *, const char *),
    int (*list_ffb_devices)(char names[][256], int max_devices),
    int (*read_wheel_axis)(const char *guid, int axis, int *value));
/* Optional shared report host. Set after configuring the provider; NULL hides
 * the action. The callbacks receive no device or settings mutation request. */
void FzeroModsSetReport(int (*available)(void), int (*open)(char *why, size_t capacity));
/* Requires the statically paired UI select_controller notification; the
 * provider copies each accepted GUID and never acquires a force device. */
const char *FzeroModsWheelGuid(void);
/* Select before launcher platform initialization. Shared exposes presentation
 * settings only and never calls native wheel/device callbacks. Requires the
 * paired recomp-ui disable_native_controllers field; it does not admit Shared
 * launcher execution by itself. */
void FzeroModsConfigureLauncherInput(
    RecompLauncherCGameInfo *game, int shared_input,
    FzeroVideoSettings *settings, const char *video_path,
    const char *config_path, const char *wheel_guid,
    void (*write_ini)(const char *, const char *, const char *, const char *),
    int (*list_ffb_devices)(char names[][256], int max_devices),
    int (*read_wheel_axis)(const char *guid, int axis, int *value));
