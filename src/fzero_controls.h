#pragma once

/* Rig-profile controls (dbce-wheel-mod-toolkit STD-033, docs/controls-contract.md).
 *
 * Wheelkit writes the profile's bindings as a [Controls] section in config.ini. At startup they are translated into
 * this game's own raw-wheel keys under [Controller.<SDL GUID>] (the keys the launcher's Racing wheel controls page
 * shows and edits), then [ControlsApplied] records the revision so later launcher edits stand until the profile
 * changes. Keys the profile has no action for (X, Y, L, R, save-state menu, rewind, deadzone, steering range and
 * response, pedal threshold) are left as they are. */

#ifdef __cplusplus
extern "C" {
#endif

enum { FZERO_CONTROLS_MAX_KEYS = 16, FZERO_CONTROLS_MAX_NOTES = 40 };

typedef struct {
  char key[32];
  int value;
} FzeroControlsKey;

typedef struct {
  int ok;                 /* a schema-1 section with at least one key to write */
  char error[128];        /* why nothing applies (ok == 0) */
  char revision[72];      /* [Controls] Revision, or a hash of the section when it has none */
  char profile[72];
  unsigned vendor, product; /* the steering device, from its product GUID (USB VID/PID) */
  char device[64];          /* its name, for the log */
  int nkeys;
  FzeroControlsKey keys[FZERO_CONTROLS_MAX_KEYS];
  int nnotes;               /* "action: reason" for each profile entry not applied */
  char notes[FZERO_CONTROLS_MAX_NOTES][64];
} FzeroControlsPlan;

/* Pure: the body lines of a [Controls] section -> the keys to write. */
void FzeroControlsPlanLines(const char *const *lines, int count, FzeroControlsPlan *plan);

/* Reads [Controls] from config_path into plan. Returns 0 when the file has no such section. */
int FzeroControlsPlanFile(const char *config_path, FzeroControlsPlan *plan);

/* 1 when [ControlsApplied] does not yet record plan->revision. */
int FzeroControlsPending(const char *config_path, const FzeroControlsPlan *plan);

/* Sets "key = value" inside [section], preserving every other line; creates the file, section or key when absent.
 * Returns 1 on success. */
int FzeroControlsIniSet(const char *path, const char *section, const char *key, const char *value);

/* Writes the plan for the joystick with this SDL GUID: backs config_path up once (<config>.before-profile-controls),
 * sets the [Controller.<guid>] keys and [Controller] GuidP1, then records [ControlsApplied]. Returns 1 on success. */
int FzeroControlsWrite(const char *config_path, const char *guid, const FzeroControlsPlan *plan);

/* Startup (before the launcher): applies a pending [Controls] revision to the attached joystick whose vendor/product
 * matches the profile's steering device. Opens SDL's joystick subsystem only for the scan. Returns 1 when keys were
 * written, 0 when nothing was pending, no section exists or no matching device is attached (retried next start). */
int FzeroControlsApplyAtStartup(const char *config_path);

#ifdef __cplusplus
}
#endif
