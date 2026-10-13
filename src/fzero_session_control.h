#pragma once
#include "fzero_session_ui.h"

/* Ordinary Windows companion control, independent of diagnostic injection.
 * No commands can supply input, change configuration or enable output. */
typedef struct {
  char path[2048], stop_path[2056], temporary[2056], request_id[33], rom_sha256[65];
  uint32_t process_id;
  uint64_t creation_file_time, published_frames;
  enum FzeroSessionMode action;
  enum FzeroSessionState published_state;
  bool enabled, muted, published, stopped, owns_temporary, pending_valid;
} FzeroSessionControl;

/* Claims a fresh status path and refuses stale status/stop/temporary files.
 * Called before opening any input/output devices. Windows process identity is
 * the actual PID + creation FILETIME, not an approximate launch timestamp. */
bool FzeroSessionControlOpen(FzeroSessionControl *control, const char *path,
    const char *request_id, enum FzeroSessionMode action, bool muted);
void FzeroSessionControlRom(FzeroSessionControl *control, const uint8_t sha256[32]);
/* 1 = exact immutable Stop, 0 = no Stop, -1 = unreadable/foreign/malformed Stop.
 * The caller alone closes the existing recorder. This never edits a tape. */
int FzeroSessionControlStop(FzeroSessionControl *control, const FzeroSessionUi *ui);
/* Atomic status replacement, at transitions and every 60 guest frames. Saved
 * and Verified are the existing recorder's states, never inferred from exit. */
bool FzeroSessionControlPublish(FzeroSessionControl *control, const FzeroSessionUi *ui,
    const FzeroPlaythrough *playthrough);
