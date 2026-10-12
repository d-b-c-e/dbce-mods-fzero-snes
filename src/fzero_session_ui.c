#include "fzero_session_ui.h"
#include <stdio.h>
#include <string.h>

bool FzeroSessionParse(int argc, char *const argv[], bool conflicting_environment,
                      FzeroSessionOptions *out, const char **error) {
  if (!out || !error) return false;
  memset(out, 0, sizeof(*out));
  *error = "Invalid command line";
  bool positional = false;
  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];
    if (!arg || !*arg) { *error = "Empty command-line argument"; return false; }
    if (!positional && !strcmp(arg, "--")) { positional = true; continue; }
    if (!positional && !strcmp(arg, "--launcher")) {
      if (out->launcher) { *error = "Duplicate --launcher"; return false; }
      out->launcher = true; continue;
    }
    enum FzeroSessionMode mode = FZERO_SESSION_NONE;
    if (!positional) {
      if (!strcmp(arg, "--record")) mode = FZERO_SESSION_RECORD;
      else if (!strcmp(arg, "--replay")) mode = FZERO_SESSION_REPLAY;
      else if (!strcmp(arg, "--shared-profile-preview")) mode = FZERO_SESSION_PREVIEW;
    }
    if (mode) {
      if (out->mode) { *error = "Choose only one recording, replay or preview"; return false; }
      if (++i >= argc || !argv[i] || !*argv[i] || argv[i][0] == '-') {
        *error = "Session option requires a path"; return false;
      }
      out->mode = mode; out->path = argv[i]; continue;
    }
    if (out->rom || (!positional && arg[0] == '-')) {
      *error = "Unknown option or more than one ROM"; return false;
    }
    out->rom = arg;
  }
  if (out->mode && (!out->rom || out->launcher)) {
    *error = "Recording, replay and preview require a ROM path without --launcher";
    return false;
  }
  if (out->mode && conflicting_environment) {
    *error = "Explicit session conflicts with existing recording/replay/test environment";
    return false;
  }
  *error = NULL;
  return true;
}

bool FzeroSessionReadInput(FzeroPlaythrough *p, FzeroSessionLiveInput live,
                           void *context, uint32_t *input) {
  if (!p || !input) return false;
  if (p->mode == 2) return FzeroPlaythroughNextInput(p, input);
  return live && live(context, input);
}

void FzeroSessionBegin(FzeroSessionUi *ui, int mode, bool preview) {
  memset(ui, 0, sizeof(*ui));
  ui->preview = preview;
  ui->state = mode == 1 ? FZERO_SESSION_REC : mode == 2 ? FZERO_SESSION_PLAYING : FZERO_SESSION_IDLE;
}
void FzeroSessionFail(FzeroSessionUi *ui, const char *reason) {
  ui->state = FZERO_SESSION_ERROR; ui->failed = true;
  snprintf(ui->error, sizeof(ui->error), "%s", reason ? reason : "Recording or replay failed");
}
bool FzeroSessionFinish(FzeroSessionUi *ui, FzeroPlaythrough *p,
                       bool upstream_ok, uint64_t now_ms) {
  if (!upstream_ok || ui->failed) {
    FzeroPlaythroughAbort(p);
    if (!ui->failed) FzeroSessionFail(ui, "Session did not finish cleanly");
    return false;
  }
  int mode = p->mode;
  if (!mode) return true;
  if (!FzeroPlaythroughClose(p)) {
    FzeroSessionFail(ui, mode == 1 ? "Recording incomplete - not saved" : "Replay incomplete or diverged");
    return false;
  }
  ui->state = mode == 1 ? FZERO_SESSION_SAVED : FZERO_SESSION_VERIFIED;
  ui->feedback_until = now_ms + 5000;
  return true;
}
bool FzeroSessionStopKey(int mode, bool f11, bool ctrl, bool other, bool repeat) {
  return mode == 1 && f11 && ctrl && !other && !repeat;
}
bool FzeroSessionCaption(const FzeroSessionUi *ui, uint64_t frames,
                          uint64_t total, uint64_t now_ms,
                          char *top, size_t top_size, char *bottom, size_t bottom_size) {
  if (!top || !bottom || !top_size || !bottom_size) return false;
  top[0] = bottom[0] = 0;
  switch (ui->state) {
  case FZERO_SESSION_REC:
    snprintf(top, top_size, "%sREC  %.1f s", ui->preview ? "MUTED PREVIEW - " : "", frames / 60.0988);
    snprintf(bottom, bottom_size, "%s", ui->preview ? "Ctrl+F11: save and close preview" : "Ctrl+F11: stop recording");
    return true;
  case FZERO_SESSION_PLAYING:
    snprintf(top, top_size, "REPLAY  %llu / %llu", (unsigned long long)frames, (unsigned long long)total);
    snprintf(bottom, bottom_size, "Verifying input and state - forces off"); return true;
  case FZERO_SESSION_ERROR:
    snprintf(top, top_size, "ERROR - recording / replay");
    snprintf(bottom, bottom_size, "%s", ui->error); return true;
  case FZERO_SESSION_SAVED:
  case FZERO_SESSION_VERIFIED:
    if (now_ms >= ui->feedback_until) return false;
    snprintf(top, top_size, "%s", ui->state == FZERO_SESSION_SAVED ? "Saved" : "Replay verified");
    snprintf(bottom, bottom_size, "%llu frames", (unsigned long long)frames); return true;
  default: return false;
  }
}
