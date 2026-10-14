#pragma once
#include "fzero_session_ui.h"

/* Ordinary Windows companion control, independent of diagnostic injection.
 * No commands can supply input, change configuration or enable output. */
typedef struct {
  char path[2048], stop_path[2056], temporary[2056], request_id[33], rom_sha256[65];
  uint32_t process_id;
  uint64_t creation_file_time, published_frames, witness_frame;
  uint32_t witness_input, witness_menu[12];
  uint8_t witness_state[3];
  double witness_speed_mps, witness_distance_m;
  enum FzeroSessionMode action;
  enum FzeroSessionState published_state;
  bool enabled, muted, published, stopped, owns_temporary, pending_valid;
  bool witness_enabled, witness_valid, witness_limit;
} FzeroSessionControl;

/* Claims a fresh status path and refuses stale status/stop/temporary files.
 * Called before opening any input/output devices. Windows process identity is
 * the actual PID + creation FILETIME, not an approximate launch timestamp. */
bool FzeroSessionControlOpen(FzeroSessionControl *control, const char *path,
    const char *request_id, enum FzeroSessionMode action, bool muted);
/* Claims a fresh short local save root without traversing links. Snapshot the
 * existing keybind file there so its missing-file writer cannot touch the game.
 * The caller's explicit isolation policy separately fences all host writes. */
bool FzeroSessionCreatePrivateRoot(const char *root, const char *keybinds);
void FzeroSessionControlRom(FzeroSessionControl *control, const uint8_t sha256[32]);
/* 1 = exact immutable Stop, 0 = no Stop, -1 = unreadable/foreign/malformed Stop.
 * The caller alone closes the existing recorder. This never edits a tape. */
int FzeroSessionControlStop(FzeroSessionControl *control, const FzeroSessionUi *ui);
/* Atomic status replacement, at transitions and every 60 guest frames. Saved
 * and Verified are the existing recorder's states, never inferred from exit. */
bool FzeroSessionControlPublish(FzeroSessionControl *control, const FzeroSessionUi *ui,
    const FzeroPlaythrough *playthrough);
/* Optional, read-only snapshot in the existing atomic status JSON. It is
 * bounded to 6000 recorded frames and never creates a second tape or input API.
 * One successful post-frame tape write must precede each published snapshot. */
bool FzeroSessionControlWitnessEnable(FzeroSessionControl *control);
bool FzeroSessionControlWitnessFrame(FzeroSessionControl *control,
    const FzeroSessionUi *ui, const FzeroPlaythrough *playthrough,
    uint64_t simulation_frame, uint32_t input, const uint8_t state[3],
    const uint32_t menu[12], double speed_mps, double distance_m);
