#pragma once

/* Rig-profile controls (dbce-wheel-mod-toolkit STD-033, docs/controls-contract.md).
 *
 * Wheelkit writes the profile's bindings as a [Controls] section in config.ini. At startup they are translated into
 * this game's own raw-wheel keys under [Controller.<SDL GUID>] (the keys the launcher's Racing wheel controls page
 * shows and edits), then [ControlsApplied] records the revision so later launcher edits stand until the profile
 * changes. Schema 2 stores canonical axis bindings beside those keys; the Native reader calls the same toolkit
 * normalization once before the existing deadzone/range/response and pedal threshold. Omitted actions and unrelated
 * settings (deadzone, steering range/response, pedal threshold) stay as they are. Native digital requests
 * nativeX/nativeY/nativeL/nativeR/saveStateMenu/rewind edit the corresponding canonical button keys.
 * This Native reader support does not advertise Schema 2 for Shared or change a writer/install capability. */

#ifdef __cplusplus
extern "C" {
#endif

enum { FZERO_CONTROLS_MAX_KEYS = 20, FZERO_CONTROLS_MAX_NOTES = 40 };

typedef struct {
  char key[32];
  int value;
} FzeroControlsKey;

typedef struct {
  int ok;                 /* a supported section with at least one key to write */
  char error[128];        /* why nothing applies (ok == 0) */
  char revision[72];      /* [Controls] Revision, or a hash of the section when it has none */
  char profile[72];
  unsigned vendor, product; /* the steering device, from its product GUID (USB VID/PID) */
  char device[64];          /* its name, for the log */
  int nkeys;
  FzeroControlsKey keys[FZERO_CONTROLS_MAX_KEYS];
  unsigned axis_profiles; /* touched axes: steer/throttle/brake bits 0/1/2; empty text restores legacy mapping */
  char axis_profile[3][512]; /* canonical Schema-2 bindings, persisted with the native controller */
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
 * The file is replaced atomically (temporary file, then one move). Returns 1 on success. */
int FzeroControlsIniSet(const char *path, const char *section, const char *key, const char *value);

/* Writes the plan for the joystick with this SDL GUID in one transaction: backs config_path up once
 * (<config>.before-profile-controls), then the [Controller.<guid>] keys, [Controller] GuidP1 and [ControlsApplied] are
 * set in one document, read back, and moved over config.ini in one step. On any failure config.ini keeps its bytes.
 * Returns 1 on success. */
int FzeroControlsWrite(const char *config_path, const char *guid, const FzeroControlsPlan *plan);

/* Native reader only: cache persisted canonical axis bindings when the raw device is opened.
 * No device access. NULL clears the cache. Normalize returns 0 for legacy mapping,
 * 1 for a canonical value, -1 for invalid metadata (neutral, never legacy fallback).
 * A deliberate native axis change leaves that axis on its legacy mapping; calibration
 * is never borrowed by a different axis. Native inversion remains the editable switch. */
void FzeroControlsNativeLoad(const char *config_path, const char *section);
int FzeroControlsNativeNormalize(int slot, int axis, int inverted, int sdl_raw, double *value);

/* Tests only: 1 makes the next writes fail while writing their temporary file, 2 at the final replace; 0 restores. */
void FzeroControlsTestFault(int fault);

/* Startup (before the launcher): applies a pending [Controls] revision to the attached joystick whose vendor/product
 * matches the profile's steering device. Opens SDL's joystick subsystem only for the scan. Returns 1 when keys were
 * written, 0 when nothing was pending, no section exists or no matching device is attached (retried next start). */
int FzeroControlsApplyAtStartup(const char *config_path);

#ifdef __cplusplus
}
#endif
