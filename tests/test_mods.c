#include "fzero_mods.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(e) do { if (!(e)) { fprintf(stderr, "%d: %s\n", __LINE__, #e); exit(1); } } while (0)
static int wrote_wheel_range, wrote_rewind, wrote_ffb, wrote_steering, wrote_impact, wrote_brake_invert, wrote_steer_invert;
static char wrote_device[256], wrote_impact_type[32];
static int live_axis_value;
static int read_live_axis(const char *guid, int axis, int *value) {
  CHECK(!strcmp(guid, "030000004c0500006802000000000000"));
  if (axis != 0) return 0;
  *value = live_axis_value;
  return 1;
}
static int list_ffb(char names[][256], int max_devices) {
  CHECK(max_devices >= 2);
  strcpy(names[0], "MOZA R12 Base");
  strcpy(names[1], "Other FFB wheel");
  return 2;
}
static void unexpected_write(const char *path, const char *section, const char *key, const char *value) {
  (void)path; (void)section; (void)key; (void)value;
  CHECK(0); /* Shared must not persist native wheel or force options. */
}
static int unexpected_list(char names[][256], int max_devices) {
  (void)names; (void)max_devices; CHECK(0); return 0;
}
static int unexpected_axis(const char *guid, int axis, int *value) {
  (void)guid; (void)axis; (void)value; CHECK(0); return 0;
}
static int report_ready, report_calls;
static int fake_report_available(void) { return report_ready; }
static int fake_report_open(char *why, size_t capacity) {
  ++report_calls; snprintf(why, capacity, "fixture report refusal"); return 0;
}
static void test_report_dispatch(void) {
  FzeroVideoSettings video; FzeroVideoDefaults(&video);
  const RecompLauncherCModProvider *p = FzeroModsProvider(&video, "not-written.ini");
  CHECK(p->feature_report_available && p->feature_report_open);
  CHECK(!p->feature_report_available(NULL,"fzero-diagnostics","diagnostics"));
  FzeroModsSetReport(fake_report_available, fake_report_open); report_ready = 1;
  CHECK(p->feature_report_available(NULL,"fzero-diagnostics","diagnostics"));
  CHECK(!p->feature_report_available(NULL,"fzero-wheel","wheel"));
  CHECK(!p->feature_report_open(NULL,"fzero-wheel","wheel") && report_calls == 0);
  int before = video.diagnostics;
  CHECK(!p->feature_report_open(NULL,"fzero-diagnostics","diagnostics") && report_calls == 1);
  CHECK(strstr(p->last_error(NULL),"fixture report refusal") && video.diagnostics == before);
  report_ready = 0;
  CHECK(!p->feature_report_open(NULL,"fzero-diagnostics","diagnostics") && report_calls == 1);
  FzeroModsProvider(&video, "not-written.ini");
  CHECK(!p->feature_report_available(NULL,"fzero-diagnostics","diagnostics"));
}
static void test_shared_launcher_selection(void) {
  FzeroVideoSettings video; FzeroVideoDefaults(&video);
  RecompLauncherCGameInfo game = {0};
  CHECK(!game.disable_native_controllers);
  FzeroModsConfigureLauncherInput(&game, 1, &video, "shared-launcher-video.ini",
      "not-opened-controls.ini", "030000004c0500006802000000000000",
      unexpected_write, unexpected_list, unexpected_axis);
  CHECK(game.disable_native_controllers && game.mods);
  CHECK(game.mods->feature_count(NULL) == 7 && !game.mods->select_controller);
  CHECK(!FzeroModsWheelGuid()[0]);
  for (int i = 0; i < game.mods->feature_count(NULL); ++i) {
    RecompLauncherCModFeature f;
    CHECK(game.mods->feature_get(NULL, i, &f));
    CHECK(strcmp(f.id, "wheel") && strcmp(f.id, "force-feedback"));
  }
  CHECK(!game.mods->feature_set_option(NULL, "fzero-wheel", "wheel", "SteeringAxis", "0"));
  CHECK(!game.mods->feature_enable(NULL, "fzero-ffb", "force-feedback", 1));
  CHECK(game.mods->commit(NULL, NULL)); // video settings only; callbacks above must remain untouched
  CHECK(remove("shared-launcher-video.ini") == 0);
  // Native explicitly clears the opt-in and retains its existing device/preview provider.
  FzeroModsConfigureLauncherInput(&game, 0, &video, "native-launcher-video.ini",
      "native-launcher-controls.ini", "030000004c0500006802000000000000",
      NULL, list_ffb, read_live_axis);
  CHECK(!game.disable_native_controllers && game.mods->feature_count(NULL) == 9);
  CHECK(game.mods->select_controller);
  RecompLauncherCModFeature wheel;
  CHECK(game.mods->feature_get(NULL, 5, &wheel) && !strcmp(wheel.id, "wheel"));
  CHECK(strstr(wheel.status, "Steering preview:"));
}
static void note_ini(const char *path, const char *section,
                     const char *key, const char *value) {
  CHECK(!strcmp(path, "wheel-options.ini"));
  if (!strcmp(section, "Controller.030000004c0500006802000000000000") &&
      !strcmp(key, "SteeringRangePercent")) wrote_wheel_range = atoi(value);
  if (!strcmp(section, "Controller.030000004c0500006802000000000000") &&
      !strcmp(key, "ButtonRewind")) wrote_rewind = atoi(value);
  if (!strcmp(section, "ForceFeedback") &&
      !strcmp(key, "Strength")) wrote_ffb = atoi(value);
  if (!strcmp(section, "ForceFeedback") &&
      !strcmp(key, "SteeringStrength")) wrote_steering = atoi(value);
  if (!strcmp(section, "ForceFeedback") &&
      !strcmp(key, "ImpactStrength")) wrote_impact = atoi(value);
  if (!strcmp(section, "ForceFeedback") &&
      !strcmp(key, "ImpactType")) strcpy(wrote_impact_type, value);
  if (!strcmp(section, "ForceFeedback") &&
      !strcmp(key, "Device")) strcpy(wrote_device, value);
  if (!strcmp(section, "Controller.030000004c0500006802000000000000") &&
      !strcmp(key, "BrakeInvert")) wrote_brake_invert = atoi(value);
  if (!strcmp(section, "Controller.030000004c0500006802000000000000") &&
      !strcmp(key, "SteeringInvert")) wrote_steer_invert = atoi(value);
}
static const char *guid_a = "030000004c0500006802000000000000";
static const char *guid_b = "030000004c0500006802000000000001";
static char profile_keys[2][32][40], profile_values[2][32][32];
static int profile_counts[2], profile_writes, axis_reads;
static char observed_guid[40];
static void write_profiles(void) {
  FILE *f = fopen("selection.ini", "w"); CHECK(f);
  for (int p = 0; p < 2; ++p) {
    fprintf(f, "[Controller.%s]\n", p ? guid_b : guid_a);
    for (int i = 0; i < profile_counts[p]; ++i)
      fprintf(f, "%s=%s\n", profile_keys[p][i], profile_values[p][i]);
  }
  fputs("[ForceFeedback]\nEnabled=0\nStrength=12\nImpactStrength=50\n", f);
  fclose(f);
}
static void selection_write(const char *path, const char *section,
                            const char *key, const char *value) {
  CHECK(!strcmp(path, "selection.ini"));
  if (!strcmp(section, "ForceFeedback")) return;
  CHECK(!strncmp(section, "Controller.", 11));
  int p = !strcmp(section + 11, guid_b);
  CHECK(!strcmp(section + 11, p ? guid_b : guid_a));
  int i = 0;
  while (i < profile_counts[p] && strcmp(profile_keys[p][i], key)) ++i;
  CHECK(i < 32);
  if (i == profile_counts[p]) ++profile_counts[p];
  snprintf(profile_keys[p][i], sizeof(profile_keys[p][i]), "%s", key);
  snprintf(profile_values[p][i], sizeof(profile_values[p][i]), "%s", value);
  ++profile_writes;
  write_profiles();
}
static int selection_axis(const char *guid, int axis, int *value) {
  CHECK(axis == (!strcmp(guid, guid_a) ? 2 : 3));
  snprintf(observed_guid, sizeof(observed_guid), "%s", guid);
  *value = 0; ++axis_reads; return 1;
}
static void test_selection_rebind(void) {
  FzeroVideoSettings video; FzeroVideoStock(&video);
  for (int p = 0; p < 2; ++p) {
    strcpy(profile_keys[p][0], "SteeringRangePercent");
    strcpy(profile_values[p][0], p ? "80" : "40");
    strcpy(profile_keys[p][1], "SteeringAxis");
    strcpy(profile_values[p][1], p ? "3" : "2");
    profile_counts[p] = 2;
  }
  write_profiles();
  const RecompLauncherCModProvider *provider = FzeroModsProviderWheel(
      &video, "selection-video.ini", "selection.ini", "", selection_write,
      NULL, selection_axis);
  RecompLauncherCModFeature wheel, ffb;
  RecompLauncherCModOption option;
  CHECK(provider->select_controller && provider->feature_count(NULL) == 9);
  CHECK(provider->feature_get(NULL, 5, &wheel) && !wheel.option_count);
  CHECK(!FzeroModsWheelGuid()[0] && axis_reads == 0 && profile_writes == 0);
  CHECK(!provider->feature_set_option(NULL, wheel.package_id, wheel.id,
                                      "SteeringRangePercent", "50"));
  /* Cancelling initial selection makes no callback or profile change. */
  CHECK(provider->feature_get(NULL, 5, &wheel) && !FzeroModsWheelGuid()[0]);
  CHECK(provider->select_controller(NULL, 0, 2, guid_a));
  CHECK(provider->feature_option_get(NULL, wheel.package_id, wheel.id, 1, &option));
  CHECK(!strcmp(option.value, "40"));
  CHECK(provider->feature_option_get(NULL, wheel.package_id, wheel.id, 3, &option));
  CHECK(!strcmp(option.device_guid, guid_a) && !strcmp(option.value, "2"));
  CHECK(provider->feature_get(NULL, 5, &wheel) && !strcmp(observed_guid, guid_a));
  CHECK(provider->feature_set_option(NULL, wheel.package_id, wheel.id,
                                     "SteeringRangePercent", "70"));
  CHECK(provider->feature_set_option(NULL, wheel.package_id, wheel.id, "SteeringInvert", "true"));
  CHECK(provider->select_controller(NULL, 0, 2, guid_a) && !profile_writes);
  const char *invalid[] = {NULL, "", "invalid", "00000000000000000000000000000000",
                          "030000004c0500006802000000000000-extra"};
  for (unsigned i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
    CHECK(!provider->select_controller(NULL, 0, 2, invalid[i]));
    CHECK(!strcmp(FzeroModsWheelGuid(), guid_a) && !profile_writes);
  }
  CHECK(provider->select_controller(NULL, 0, 2, guid_b) && profile_writes == 25);
  CHECK(provider->feature_option_get(NULL, wheel.package_id, wheel.id, 23, &option) && !strcmp(option.value, "false"));
  CHECK(provider->feature_option_get(NULL, wheel.package_id, wheel.id, 1, &option));
  CHECK(!strcmp(option.value, "80"));
  CHECK(provider->feature_option_get(NULL, wheel.package_id, wheel.id, 3, &option));
  CHECK(!strcmp(option.device_guid, guid_b) && !strcmp(option.value, "3"));
  CHECK(provider->feature_get(NULL, 5, &wheel) && !strcmp(observed_guid, guid_b));
  CHECK(provider->select_controller(NULL, 0, 1, "") &&
        !strcmp(FzeroModsWheelGuid(), guid_b)); /* Keyboard keeps raw wheel. */
  CHECK(provider->select_controller(NULL, 1, 2, "invalid") &&
        !strcmp(FzeroModsWheelGuid(), guid_b)); /* Other player is unrelated. */
  CHECK(provider->select_controller(NULL, 0, 2, guid_a));
  CHECK(provider->feature_option_get(NULL, wheel.package_id, wheel.id, 1, &option));
  CHECK(!strcmp(option.value, "70")); /* Prior edits saved to their own profile. */
  CHECK(provider->feature_option_get(NULL, wheel.package_id, wheel.id, 23, &option) && !strcmp(option.value, "true"));
  CHECK(provider->feature_get(NULL, 6, &ffb) && !ffb.enabled);
  CHECK(provider->feature_option_get(NULL, ffb.package_id, ffb.id, 0, &option));
  CHECK(!strcmp(option.value, "12")); /* Rebind doesn't reload/enable force state. */
  remove("selection.ini"); remove("selection-video.ini");
}
int main(void) {
  test_report_dispatch();
  FzeroVideoSettings s, loaded; FzeroVideoStock(&s); /* start from nothing enabled to test each toggle */
  const RecompLauncherCModProvider *p = FzeroModsProvider(&s, "test-mods.ini");
  RecompLauncherCModFeature w, f;
  RecompLauncherCModOption option;
  CHECK(p->package_count(NULL) == 7 && p->feature_count(NULL) == 7);
  RecompLauncherCModFeature triple;
  CHECK(p->feature_get(NULL, 5, &triple) && !triple.enabled && triple.option_count == 8); /* 7 rig values incl. curve radius + display mode */
  CHECK(p->feature_enable(NULL, triple.package_id, triple.id, 1) && s.triple_screen);
  CHECK(p->feature_option_get(NULL, triple.package_id, triple.id, 0, &option));
  CHECK(!strcmp(option.id, "TriplePanelWidthMm") && !strcmp(option.value, "708") &&
        option.min_value == 200 && option.max_value == 3000);
  CHECK(p->feature_option_get(NULL, triple.package_id, triple.id, 6, &option));
  CHECK(!strcmp(option.id, "TripleCurveRadiusMm") && !strcmp(option.value, "0") && option.max_value == 20000);
  CHECK(p->feature_option_get(NULL, triple.package_id, triple.id, 7, &option));
  CHECK(!strcmp(option.id, "TripleOutputMode") && !strcmp(option.value, "Span") &&
        option.type == RECOMP_MOD_OPTION_CHOICE && option.choice_count == 2);
  RecompLauncherCModChoice triple_choice;
  CHECK(p->feature_choice_get(NULL, triple.package_id, triple.id,
                              "TripleOutputMode", 1, &triple_choice));
  CHECK(!strcmp(triple_choice.value, "Separate"));
  CHECK(p->feature_set_option(NULL, triple.package_id, triple.id,
                              "TripleOutputMode", "Separate"));
  CHECK(!p->feature_set_option(NULL, triple.package_id, triple.id,
                               "TripleOutputMode", "invalid"));
  CHECK(p->feature_set_option(NULL, triple.package_id, triple.id, "TriplePanelWidthMm", "620"));
  CHECK(p->feature_set_option(NULL, triple.package_id, triple.id, "TripleEyeDistanceMm", "780"));
  CHECK(p->feature_set_option(NULL, triple.package_id, triple.id, "TripleLeftAngleDeg", "55"));
  CHECK(p->feature_set_option(NULL, triple.package_id, triple.id, "TripleRightAngleDeg", "65"));
  CHECK(p->feature_set_option(NULL, triple.package_id, triple.id, "TripleBezelGapMm", "12"));
  CHECK(p->feature_set_option(NULL, triple.package_id, triple.id, "TripleEyeHeightMm", "45"));
  CHECK(!p->feature_set_option(NULL, triple.package_id, triple.id, "TriplePanelWidthMm", "0"));
  CHECK(!p->feature_set_option(NULL, triple.package_id, triple.id, "TripleRightAngleDeg", "90"));
  CHECK(!p->feature_set_option(NULL, triple.package_id, triple.id, "TripleEyeDistanceMm", "660junk"));
  RecompLauncherCModFeature flash;
  CHECK(p->feature_get(NULL, 6, &flash) && !flash.enabled && flash.option_count == 0);
  CHECK(p->feature_enable(NULL, flash.package_id, flash.id, 1) && s.reduce_crash_flash);
  RecompLauncherCModFeature diag;
  CHECK(p->feature_get(NULL, 4, &diag) && !diag.enabled && diag.option_count == 0);
  CHECK(!p->feature_option_get(NULL, diag.package_id, diag.id, 0, &option));
  CHECK(p->feature_enable(NULL, diag.package_id, diag.id, 1) && s.diagnostics);
  CHECK(!s.enhanced && !s.hd_mode7 && !s.fps_enabled);
  RecompLauncherCModFeature hd;
  CHECK(p->feature_get(NULL, 3, &hd) && !hd.enabled && hd.option_count == 1);
  CHECK(p->feature_enable(NULL, hd.package_id, hd.id, 1));
  CHECK(s.hd_mode7 && !s.enhanced && !s.fps_enabled);
  CHECK(p->feature_set_option(NULL, hd.package_id, hd.id, "scale", "4"));
  CHECK(s.hd_scale == 4);
  CHECK(p->feature_get(NULL, 3, &hd) && !strstr(hd.description, "Warning:"));
  for (unsigned scale = 5; scale <= 10; ++scale) {
    char text[8]; snprintf(text, sizeof(text), "%u", scale);
    CHECK(p->feature_set_option(NULL, hd.package_id, hd.id, "scale", text));
    CHECK(s.hd_scale == scale);
    CHECK(p->feature_get(NULL, 3, &hd));
    CHECK(strstr(hd.description, "Warning:") && strstr(hd.description, "own risk"));
    CHECK(!hd.has_error); /* high-cost values remain usable */
  }
  CHECK(!p->feature_set_option(NULL, hd.package_id, hd.id, "scale", "11"));
  CHECK(!p->feature_set_option(NULL, hd.package_id, hd.id, "scale", "6.5"));
  CHECK(s.hd_scale == 10 && strstr(p->last_error(NULL), "2 to 10"));
  CHECK(p->feature_option_get(NULL, hd.package_id, hd.id, 0, &option));
  CHECK(!strcmp(option.value, "10") && !strcmp(option.default_value, "2"));
  CHECK(option.type == RECOMP_MOD_OPTION_INTEGER && option.min_value == 2 && option.max_value == 10);
  RecompLauncherCModFeature deluxe;
  CHECK(p->feature_get(NULL, 2, &deluxe) && deluxe.option_count == 0);
#ifdef FZERO_HAS_DELUXE
  CHECK(p->feature_enable(NULL, deluxe.package_id, deluxe.id, 1));
  CHECK(s.bs_deluxe && !s.fps_enabled && !s.enhanced);
#else
  CHECK(!deluxe.enabled && strstr(deluxe.status, "Unavailable"));
  CHECK(!p->feature_enable(NULL, deluxe.package_id, deluxe.id, 1));
  CHECK(!s.bs_deluxe && strstr(p->last_error(NULL), "unavailable"));
#endif
  CHECK(!p->feature_option_get(NULL, deluxe.package_id, deluxe.id, 0, &option));
  CHECK(p->feature_get(NULL, 0, &w) && p->feature_get(NULL, 1, &f));
  CHECK(strcmp(w.package_id, f.package_id) && w.option_count == 1 && f.option_count == 2);
  CHECK(p->feature_option_get(NULL, f.package_id, f.id, 1, &option));
  CHECK(!strcmp(option.id, "ShowFPS") && option.type == RECOMP_MOD_OPTION_BOOLEAN &&
        !strcmp(option.value, "false") && !strcmp(option.default_value, "false"));
  CHECK(!p->feature_set_option(NULL, f.package_id, f.id, "ShowFPS", "yes"));
  CHECK(!p->feature_set_option(NULL, w.package_id, w.id, "ShowFPS", "true"));
  CHECK(p->feature_set_option(NULL, f.package_id, f.id, "ShowFPS", "true") && s.show_fps);
  CHECK(p->feature_enable(NULL, f.package_id, f.id, 1));
  CHECK(s.fps_enabled && !s.enhanced);
  CHECK(p->feature_set_option(NULL, f.package_id, f.id, "fps", "144"));
  CHECK(!p->feature_set_option(NULL, w.package_id, w.id, "fps", "60"));
  CHECK(p->feature_enable(NULL, w.package_id, w.id, 1));
  CHECK(p->feature_enable(NULL, f.package_id, f.id, 0));
  CHECK(s.enhanced && !s.fps_enabled && s.fps == 144);
  CHECK(p->feature_option_get(NULL, w.package_id, w.id, 0, &option) && !strcmp(option.id, "aspect"));
  CHECK(p->feature_option_get(NULL, f.package_id, f.id, 0, &option) && !strcmp(option.id, "fps"));
  CHECK(p->commit(NULL, NULL) && FzeroVideoLoad(&loaded, "test-mods.ini"));
  CHECK(loaded.enhanced && !loaded.fps_enabled && loaded.fps == 144 && loaded.show_fps);
#ifdef FZERO_HAS_DELUXE
  CHECK(loaded.bs_deluxe);
#else
  CHECK(!loaded.bs_deluxe);
#endif
  CHECK(loaded.hd_mode7 && loaded.hd_scale == 10 && loaded.diagnostics &&
        loaded.triple_screen && loaded.reduce_crash_flash);
  CHECK(loaded.triple_panel_width_mm == 620 && loaded.triple_eye_distance_mm == 780 &&
        loaded.triple_left_yaw_deg == 55 && loaded.triple_right_yaw_deg == 65 &&
        loaded.triple_bezel_gap_mm == 12 && loaded.triple_eye_height_mm == 45 &&
        loaded.triple_output_mode == FZERO_TRIPLE_OUTPUT_SEPARATE);
  CHECK(p->feature_enable(NULL, diag.package_id, diag.id, 0) && !s.diagnostics);
  CHECK(p->commit(NULL, NULL) && FzeroVideoLoad(&loaded, "test-mods.ini") && !loaded.diagnostics);
  CHECK(p->feature_enable(NULL, hd.package_id, hd.id, 0));
  CHECK(!s.hd_mode7 && s.hd_scale == 10 && s.enhanced);
  CHECK(p->feature_enable(NULL, deluxe.package_id, deluxe.id, 0));
  CHECK(!s.bs_deluxe && s.enhanced && !s.fps_enabled);
  FILE *legacy_config = fopen("wheel-options.ini", "w"); CHECK(legacy_config);
  fputs("[ForceFeedback]\nStrength=12\n", legacy_config); fclose(legacy_config);
  p = FzeroModsProviderWheel(&s, "test-mods.ini", "wheel-options.ini",
                             "030000004c0500006802000000000000", note_ini, list_ffb, read_live_axis);
  CHECK(p->feature_count(NULL) == 9);
  RecompLauncherCModFeature wheel, ffb, triple_with_wheel;
  CHECK(p->feature_get(NULL, 5, &wheel) && wheel.option_count == 24);
  live_axis_value = 8192;
  CHECK(p->feature_get(NULL, 5, &wheel));
  CHECK(strstr(wheel.status, "RIGHT 50% of SNES frames") &&
        strstr(wheel.status, "axis 0 +25%"));
  CHECK(p->feature_set_option(NULL, wheel.package_id, wheel.id,
                              "SteeringRangePercent", "50"));
  CHECK(p->feature_get(NULL, 5, &wheel) &&
        strstr(wheel.status, "RIGHT 71% of SNES frames"));
  live_axis_value = -8192;
  CHECK(p->feature_get(NULL, 5, &wheel) &&
        strstr(wheel.status, "LEFT 71% of SNES frames"));
  live_axis_value = 0;
  CHECK(p->feature_get(NULL, 5, &wheel) &&
        strstr(wheel.status, "CENTER 0% of SNES frames"));
  CHECK(p->feature_option_get(NULL, wheel.package_id, wheel.id, 23, &option));
  CHECK(!strcmp(option.id, "SteeringInvert") && option.type == RECOMP_MOD_OPTION_BOOLEAN &&
        !strcmp(option.value, "false"));
  CHECK(!p->feature_set_option(NULL, wheel.package_id, wheel.id, "SteeringInvert", "1"));
  CHECK(!p->feature_set_option(NULL, wheel.package_id, wheel.id, "SteeringInvert", "yes"));
  CHECK(p->feature_set_option(NULL, wheel.package_id, wheel.id, "SteeringInvert", "true"));
  live_axis_value = 8192;
  CHECK(p->feature_get(NULL, 5, &wheel) && strstr(wheel.status, "LEFT 71% of SNES frames"));
  live_axis_value = -32768;
  CHECK(p->feature_get(NULL, 5, &wheel) && strstr(wheel.status, "RIGHT 100% of SNES frames"));
  live_axis_value = 0;
  CHECK(p->feature_get(NULL, 5, &wheel) && strstr(wheel.status, "CENTER 0% of SNES frames"));
  CHECK(p->feature_set_option(NULL, wheel.package_id, wheel.id, "SteeringInvert", "false"));
  CHECK(p->feature_get(NULL, 6, &ffb) && ffb.option_count == 4);
  CHECK(p->feature_get(NULL, 7, &triple_with_wheel) &&
        !strcmp(triple_with_wheel.package_id, triple.package_id) &&
        triple_with_wheel.enabled && triple_with_wheel.option_count == 8);
  RecompLauncherCModFeature flash_with_wheel;
  CHECK(p->feature_get(NULL, 8, &flash_with_wheel) &&
        !strcmp(flash_with_wheel.package_id, flash.package_id) &&
        flash_with_wheel.enabled && flash_with_wheel.option_count == 0);
  CHECK(p->feature_option_get(NULL, wheel.package_id, wheel.id, 22, &option));
  CHECK(option.type == RECOMP_MOD_OPTION_RAW_BUTTON);
  CHECK(!strcmp(option.device_guid, "030000004c0500006802000000000000"));
  CHECK(p->feature_option_get(NULL, wheel.package_id, wheel.id, 3, &option));
  CHECK(option.type == RECOMP_MOD_OPTION_RAW_AXIS);
  CHECK(!strcmp(option.device_guid, "030000004c0500006802000000000000"));
  CHECK(p->feature_option_get(NULL, wheel.package_id, wheel.id, 8, &option));
  CHECK(option.type == RECOMP_MOD_OPTION_BOOLEAN && !strcmp(option.value, "false"));
  CHECK(p->feature_option_get(NULL, ffb.package_id, ffb.id, 1, &option));
  CHECK(option.type == RECOMP_MOD_OPTION_CHOICE && option.choice_count == 3);
  CHECK(p->feature_option_get(NULL, ffb.package_id, ffb.id, 0, &option));
  CHECK(!strcmp(option.id, "SteeringStrength") && !strcmp(option.value, "12"));
  CHECK(strstr(option.label, "Steering strength") && strstr(option.description, "Damping"));
  CHECK(p->feature_option_get(NULL, ffb.package_id, ffb.id, 2, &option));
  CHECK(!strcmp(option.id, "ImpactStrength") && !strcmp(option.value, "20"));
  CHECK(p->feature_option_get(NULL, ffb.package_id, ffb.id, 3, &option));
  CHECK(!strcmp(option.id, "ImpactType") && !strcmp(option.value, "Constant") &&
        option.type == RECOMP_MOD_OPTION_CHOICE && option.choice_count == 2);
  RecompLauncherCModChoice choice;
  CHECK(p->feature_choice_get(NULL, ffb.package_id, ffb.id, "Device", 1, &choice));
  CHECK(!strcmp(choice.value, "MOZA R12 Base"));
  CHECK(p->feature_choice_get(NULL, ffb.package_id, ffb.id, "ImpactType", 1, &choice));
  CHECK(!strcmp(choice.value, "Sine"));
  CHECK(p->feature_set_option(NULL, wheel.package_id, wheel.id,
                              "SteeringRangePercent", "45"));
  CHECK(p->feature_set_option(NULL, wheel.package_id, wheel.id,
                              "ButtonRewind", "37"));
  CHECK(p->feature_set_option(NULL, wheel.package_id, wheel.id,
                              "ButtonUp", "128"));
  CHECK(p->feature_option_get(NULL, wheel.package_id, wheel.id, 17, &option));
  CHECK(!strcmp(option.value, "128"));
  CHECK(!p->feature_set_option(NULL, wheel.package_id, wheel.id,
                               "ButtonUp", "192"));
  CHECK(!p->feature_set_option(NULL, wheel.package_id, wheel.id,
                               "ButtonRewind", "128"));
  CHECK(p->feature_set_option(NULL, ffb.package_id, ffb.id,
                              "SteeringStrength", "40"));
  CHECK(!p->feature_set_option(NULL, ffb.package_id, ffb.id, "SteeringStrength", "101"));
  CHECK(!p->feature_set_option(NULL, ffb.package_id, ffb.id, "Strength", "80"));
  CHECK(p->feature_set_option(NULL, ffb.package_id, ffb.id,
                              "ImpactStrength", "25"));
  CHECK(p->feature_set_option(NULL, ffb.package_id, ffb.id,
                              "ImpactType", "Sine"));
  CHECK(!p->feature_set_option(NULL, ffb.package_id, ffb.id,
                               "ImpactStrength", "101"));
  CHECK(!p->feature_set_option(NULL, ffb.package_id, ffb.id,
                               "ImpactType", "Square"));
  CHECK(p->feature_set_option(NULL, ffb.package_id, ffb.id,
                              "Device", "MOZA R12 Base"));
  CHECK(!p->feature_set_option(NULL, ffb.package_id, ffb.id,
                               "Device", "unknown"));
  CHECK(p->feature_set_option(NULL, wheel.package_id, wheel.id,
                              "BrakeInvert", "true"));
  CHECK(p->feature_set_option(NULL, wheel.package_id, wheel.id, "SteeringInvert", "true"));
  CHECK(p->commit(NULL, NULL));
  CHECK(wrote_wheel_range == 45 && wrote_rewind == 37 && wrote_ffb == 12 && wrote_steering == 40 &&
        wrote_impact == 25 && !strcmp(wrote_impact_type, "Sine"));
  CHECK(wrote_brake_invert == 1 && !strcmp(wrote_device, "MOZA R12 Base"));
  CHECK(wrote_steer_invert == 1);
  CHECK(p->feature_set_option(NULL, wheel.package_id, wheel.id, "SteeringInvert", "false"));
  CHECK(p->commit(NULL, NULL) && wrote_steer_invert == 0);
  remove("test-mods.ini");
  remove("wheel-options.ini");
  test_selection_rebind();
  test_shared_launcher_selection();
  puts("Independent widescreen and presentation FPS plugins passed");
  return 0;
}
