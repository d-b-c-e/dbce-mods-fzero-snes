#pragma once
#include "fzero_playthrough.h"

enum FzeroSessionMode { FZERO_SESSION_NONE, FZERO_SESSION_RECORD,
  FZERO_SESSION_REPLAY, FZERO_SESSION_PREVIEW };
typedef struct {
  enum FzeroSessionMode mode;
  const char *rom, *path;
  bool launcher;
} FzeroSessionOptions;

/* Parse before changing cwd or opening devices. Explicit sessions require a
 * ROM and cannot inherit another recording, injection or replay request. */
bool FzeroSessionParse(int argc, char *const argv[], bool conflicting_environment,
                      FzeroSessionOptions *out, const char **error);

typedef bool (*FzeroSessionLiveInput)(void *context, uint32_t *input);
/* Tape owns the input boundary before any physical read or live host-button
 * side effect. The live callback is never invoked during recorded playback. */
bool FzeroSessionReadInput(FzeroPlaythrough *p, FzeroSessionLiveInput live,
                           void *context, uint32_t *input);

enum FzeroSessionState { FZERO_SESSION_IDLE, FZERO_SESSION_REC,
  FZERO_SESSION_PLAYING, FZERO_SESSION_SAVED, FZERO_SESSION_VERIFIED,
  FZERO_SESSION_ERROR };
typedef struct {
  enum FzeroSessionState state;
  bool preview, failed;
  uint64_t feedback_until;
  char error[96];
} FzeroSessionUi;
void FzeroSessionBegin(FzeroSessionUi *ui, int recorder_mode, bool preview);
void FzeroSessionFail(FzeroSessionUi *ui, const char *reason);
/* Close the actual recorder, not just the UI. An upstream failure invalidates
 * the file; Saved/Verified requires a successful, complete close. */
bool FzeroSessionFinish(FzeroSessionUi *ui, FzeroPlaythrough *p,
                       bool upstream_ok, uint64_t now_ms);
bool FzeroSessionStopKey(int recorder_mode, bool f11, bool ctrl,
                         bool shift_or_alt, bool repeat);
bool FzeroSessionCaption(const FzeroSessionUi *ui, uint64_t frames,
                          uint64_t total, uint64_t now_ms,
                          char *top, size_t top_size, char *bottom, size_t bottom_size);
