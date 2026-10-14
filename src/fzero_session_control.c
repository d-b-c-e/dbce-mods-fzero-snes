#include "fzero_session_control.h"
#include <errno.h>
#include <math.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

static const char *state_name(enum FzeroSessionState state) {
  switch (state) {
  case FZERO_SESSION_REC: return "recording";
  case FZERO_SESSION_PLAYING: return "playing";
  case FZERO_SESSION_SAVED: return "saved";
  case FZERO_SESSION_VERIFIED: return "verified";
  case FZERO_SESSION_ERROR: return "failed";
  default: return "starting";
  }
}
static bool absent(const char *path) {
  FILE *file = fopen(path, "rb");
  if (file) { fclose(file); return false; }
  return errno == ENOENT;
}
bool FzeroSessionCreatePrivateRoot(const char *root, const char *keybinds) {
#ifdef _WIN32
  /* The engine still has 128-byte save suffix buffers. Keep the same short
   * root limit as the existing companion replay path, and refuse aliases. */
  if (!root || strlen(root) < 4 || strlen(root) > 70 || root[1] != ':' ||
      (root[2] != '\\' && root[2] != '/') || !keybinds) return false;
  char normalized[80], full[80];
  snprintf(normalized, sizeof(normalized), "%s", root);
  for (char *at = normalized; *at; ++at) if (*at == '/') *at = '\\';
  DWORD length = GetFullPathNameA(normalized, sizeof(full), full, NULL);
  if (!length || length >= sizeof(full) || _stricmp(full, normalized) ||
      full[length - 1] == '\\' || GetFileAttributesA(full) != INVALID_FILE_ATTRIBUTES ||
      GetLastError() != ERROR_FILE_NOT_FOUND) return false;
  char parent[80]; snprintf(parent, sizeof(parent), "%s", full);
  while (strlen(parent) > 3) {
    char *slash = strrchr(parent, '\\'); if (!slash) return false;
    if (slash == parent + 2) slash[1] = 0; else *slash = 0;
    DWORD attributes = GetFileAttributesA(parent);
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY) ||
        (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
  }
  if (!CreateDirectoryA(full, NULL)) return false;
  char destination[96]; snprintf(destination, sizeof(destination), "%s\\keybinds.ini", full);
  if (CopyFileA(keybinds, destination, TRUE)) return true;
  return GetLastError() == ERROR_FILE_NOT_FOUND; /* private defaults may be generated */
#else
  (void)root; (void)keybinds; return false;
#endif
}
static bool write_status(FILE *file, const FzeroSessionControl *c,
                         const FzeroSessionUi *ui, const FzeroPlaythrough *p) {
  /* All strings are fixed tokens or validated hex, so no lossy JSON escaping.
   * The existing game log/UI retains the descriptive error. */
  if (fprintf(file, "{\"schema\":\"fzero.session-status@1\",\"requestId\":\"%s\","
      "\"action\":\"%s\",\"state\":\"%s\",\"processId\":%u,"
      "\"processCreationFileTime\":%llu,\"frames\":%llu,\"total\":%llu,"
      "\"effectiveRomSha256\":\"%s\",\"outputMuted\":%s",
      c->request_id, c->action == FZERO_SESSION_RECORD ? "record" : "replay", state_name(ui->state),
      (unsigned)c->process_id, (unsigned long long)c->creation_file_time,
      (unsigned long long)p->frames, (unsigned long long)p->total, c->rom_sha256,
      c->muted ? "true" : "false") < 0) return false;
  if (c->witness_enabled) {
    bool witness_live = c->witness_valid && ui->state == FZERO_SESSION_REC;
    if (fprintf(file, ",\"routeWitness\":{\"valid\":%s,\"limited\":%s",
        witness_live ? "true" : "false", c->witness_limit ? "true" : "false") < 0) return false;
    if (witness_live && fprintf(file,
        ",\"simulationFrame\":%llu,\"inputWord\":%u,\"state54\":%u,\"state55\":%u,\"state56\":%u,"
        "\"selection\":%u,\"training\":%u,\"transition\":%u,\"car\":%u,\"course\":%u,\"league\":%u,"
        "\"deluxeCar\":%u,\"deluxeRival\":%u,\"deluxePanel\":%u,\"deluxeAccept\":%u,\"deluxeTrack\":%u,\"state5b\":%u,"
        "\"speedMps\":%.6f,\"distanceM\":%.6f",
        (unsigned long long)c->witness_frame, (unsigned)c->witness_input,
        (unsigned)c->witness_state[0], (unsigned)c->witness_state[1], (unsigned)c->witness_state[2],
        (unsigned)c->witness_menu[0], (unsigned)c->witness_menu[1], (unsigned)c->witness_menu[2],
        (unsigned)c->witness_menu[3], (unsigned)c->witness_menu[4], (unsigned)c->witness_menu[5],
        (unsigned)c->witness_menu[6], (unsigned)c->witness_menu[7], (unsigned)c->witness_menu[8],
        (unsigned)c->witness_menu[9], (unsigned)c->witness_menu[10], (unsigned)c->witness_menu[11],
        c->witness_speed_mps, c->witness_distance_m) < 0) return false;
    if (fputc('}', file) == EOF) return false;
  }
  return fputs("}\n", file) >= 0 && fflush(file) == 0;
}
bool FzeroSessionControlOpen(FzeroSessionControl *c, const char *path,
    const char *id, enum FzeroSessionMode action, bool muted) {
  if (!c) return false;
  memset(c, 0, sizeof(*c));
  if (!path || !*path || !id || strlen(id) != 32 || strspn(id, "0123456789abcdef") != 32 ||
      (action != FZERO_SESSION_RECORD && action != FZERO_SESSION_REPLAY) ||
      strlen(path) >= sizeof(c->path)) return false;
#ifdef _WIN32
  /* The CLI resolves against its caller before changing cwd. Refuse relative
   * paths here too; a companion must name one concrete result location. */
  if (!((strlen(path) > 2 && path[1] == ':' && (path[2] == '/' || path[2] == '\\')) ||
        (path[0] == '\\' && path[1] == '\\'))) return false;
  FILETIME created, exited, kernel, user;
  if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return false;
  c->process_id = GetCurrentProcessId();
  c->creation_file_time = ((uint64_t)created.dwHighDateTime << 32) | created.dwLowDateTime;
#else
  return false; /* Ordinary CLI recording/replay remains portable, control is Windows-only. */
#endif
  snprintf(c->path, sizeof(c->path), "%s", path);
  snprintf(c->temporary, sizeof(c->temporary), "%s.tmp", path);
  snprintf(c->stop_path, sizeof(c->stop_path), "%s.stop", path);
  snprintf(c->request_id, sizeof(c->request_id), "%s", id);
  c->action = action; c->muted = muted || action == FZERO_SESSION_REPLAY;
  if (!absent(c->path) || !absent(c->temporary) || !absent(c->stop_path)) return false;
  FILE *file = fopen(c->temporary, "wbx");
  if (!file) return false;
  FzeroSessionUi ui = {0}; FzeroPlaythrough p = {0};
  bool ok = write_status(file, c, &ui, &p);
  if (fclose(file)) ok = false;
#ifdef _WIN32
  if (ok) ok = MoveFileExA(c->temporary, c->path, MOVEFILE_WRITE_THROUGH) != 0;
#endif
  c->enabled = ok;
  return ok;
}
void FzeroSessionControlRom(FzeroSessionControl *c, const uint8_t sha[32]) {
  if (!c || !sha) return;
  for (unsigned i = 0; i < 32; ++i) snprintf(c->rom_sha256 + 2 * i, 3, "%02x", sha[i]);
}
int FzeroSessionControlStop(FzeroSessionControl *c, const FzeroSessionUi *ui) {
  if (!c || !c->enabled || c->stopped || c->action != FZERO_SESSION_RECORD ||
      !ui || ui->state != FZERO_SESSION_REC) return 0;
  FILE *file = fopen(c->stop_path, "rb");
  if (!file) return errno == ENOENT ? 0 : -1;
  char bytes[96], expected[96];
  size_t count = fread(bytes, 1, sizeof(bytes), file);
  bool valid_read = !ferror(file) && feof(file);
  if (fclose(file)) valid_read = false;
  int expected_size = snprintf(expected, sizeof(expected), "fzero.session-stop@1\n%s\n", c->request_id);
  if (!valid_read || count != (size_t)expected_size || memcmp(bytes, expected, count)) return -1;
  c->stopped = true;
  return 1;
}
bool FzeroSessionControlPublish(FzeroSessionControl *c, const FzeroSessionUi *ui,
                               const FzeroPlaythrough *p) {
  if (!c || !c->enabled) return true;
  if (!ui || !p) return false;
  if (c->owns_temporary) {
    /* A reader may temporarily deny replacement. Retry only the file created
     * by this control instance; never adopt a pre-existing .tmp. Publish the
     * current state again immediately after moving an older pending update. */
#ifdef _WIN32
    bool recovered = c->pending_valid
        ? MoveFileExA(c->temporary, c->path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0
        : remove(c->temporary) == 0;
#else
    bool recovered = c->pending_valid ? rename(c->temporary, c->path) == 0 : remove(c->temporary) == 0;
#endif
    if (!recovered) return false;
    c->owns_temporary = c->pending_valid = false;
    c->published = false;
  }
  if (c->published && c->published_state == ui->state &&
      p->frames >= c->published_frames && p->frames - c->published_frames <
          (c->witness_enabled && !c->witness_limit ? 1u : 60u)) return true;
  FILE *file = fopen(c->temporary, "wbx");
  if (!file) return false;
  c->owns_temporary = true;
  bool ok = write_status(file, c, ui, p);
  if (fclose(file)) ok = false;
  c->pending_valid = ok;
#ifdef _WIN32
  if (ok) ok = MoveFileExA(c->temporary, c->path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
  if (ok) ok = rename(c->temporary, c->path) == 0;
#endif
  if (ok) { c->owns_temporary = c->pending_valid = false; c->published = true; c->published_state = ui->state; c->published_frames = p->frames; }
  return ok;
}
bool FzeroSessionControlWitnessEnable(FzeroSessionControl *c) {
  if (!c || !c->enabled || !c->muted || c->action != FZERO_SESSION_RECORD || c->witness_enabled)
    return false;
  c->witness_enabled = true;
  return true;
}
bool FzeroSessionControlWitnessFrame(FzeroSessionControl *c, const FzeroSessionUi *ui,
    const FzeroPlaythrough *p, uint64_t simulation_frame, uint32_t input,
    const uint8_t state[3], const uint32_t menu[12], double speed_mps, double distance_m) {
  if (!c || !c->witness_enabled || !ui || ui->state != FZERO_SESSION_REC || !p || p->mode != 1 ||
      !state || !menu || p->frames != simulation_frame + 1 ||
      !isfinite(speed_mps) || !isfinite(distance_m) || speed_mps < 0 || distance_m < 0)
    return false;
  if (c->witness_limit) return true;
  if (c->witness_valid && simulation_frame <= c->witness_frame) return false;
  if (p->frames > 6000) {
    c->witness_limit = true;
    c->witness_valid = false;
    c->published = false;
    (void)FzeroSessionControlPublish(c, ui, p);
    return true;
  }
  c->witness_frame = simulation_frame;
  c->witness_input = input;
  memcpy(c->witness_state, state, sizeof(c->witness_state));
  memcpy(c->witness_menu, menu, sizeof(c->witness_menu));
  c->witness_speed_mps = speed_mps;
  c->witness_distance_m = distance_m;
  c->witness_valid = true;
  /* Publication may be temporarily blocked by a reader. The recorder keeps
   * running; the route driver must refuse stale status frames. */
  (void)FzeroSessionControlPublish(c, ui, p);
  return true;
}
