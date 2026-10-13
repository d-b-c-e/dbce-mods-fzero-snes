#include "fzero_session_control.h"
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
static void write_file(const char *path, const char *text) {
  FILE *f = fopen(path, "wbx"); CHECK(f != NULL);
  CHECK(fwrite(text, 1, strlen(text), f) == strlen(text)); CHECK(fclose(f) == 0);
}
static void contains(const char *path, const char *text) {
  char bytes[2048] = {0}; FILE *f = fopen(path, "rb"); CHECK(f != NULL);
  size_t n = fread(bytes, 1, sizeof(bytes) - 1, f); CHECK(n > 0 && feof(f)); CHECK(fclose(f) == 0);
  CHECK(strstr(bytes, text) != NULL);
}
int main(void) {
  char dir[MAX_PATH], path[MAX_PATH], tape[MAX_PATH];
  CHECK(GetTempPathA(sizeof(dir), dir) != 0);
  snprintf(path, sizeof(path), "%sfzero-control-%lu.json", dir, GetCurrentProcessId());
  snprintf(tape, sizeof(tape), "%sfzero-control-%lu.fzpt", dir, GetCurrentProcessId());
  const char *id = "0123456789abcdef0123456789abcdef";
  FzeroSessionControl c, other; FzeroSessionUi ui; FzeroPlaythrough p = {0}, reader = {0};
  uint8_t sha[32] = {1}, ram[32] = {0};
  CHECK(!FzeroSessionControlOpen(&c, "relative.json", id, FZERO_SESSION_RECORD, false));
  CHECK(!FzeroSessionControlOpen(&c, path, "wrong", FZERO_SESSION_RECORD, false));
  CHECK(!FzeroSessionControlOpen(&c, path, id, FZERO_SESSION_PREVIEW, true));
  CHECK(FzeroSessionControlOpen(&c, path, id, FZERO_SESSION_RECORD, false));
  CHECK(c.process_id == GetCurrentProcessId() && c.creation_file_time > 0 && !c.muted);
  CHECK(!FzeroSessionControlOpen(&other, path, id, FZERO_SESSION_RECORD, false));
  contains(path, "\"state\":\"starting\"");
  CHECK(FzeroPlaythroughRecordOpen(&p, tape, sha));
  FzeroSessionBegin(&ui, 1, false); FzeroSessionControlRom(&c, sha);
  CHECK(FzeroSessionControlPublish(&c, &ui, &p));
  contains(path, "\"state\":\"recording\"");
  contains(path, "\"effectiveRomSha256\":\"0100000000000000000000000000000000000000000000000000000000000000\"");
  CHECK(FzeroSessionControlStop(&c, &ui) == 0);
  write_file(c.stop_path, "fzero.session-stop@1\nffffffffffffffffffffffffffffffff\n");
  CHECK(FzeroSessionControlStop(&c, &ui) == -1); /* another launch cannot stop this one */
  CHECK(remove(c.stop_path) == 0);
  write_file(c.stop_path, "fzero.session-stop@1\n0123456789abcdef0123456789abcdef\nextra");
  CHECK(FzeroSessionControlStop(&c, &ui) == -1);
  CHECK(remove(c.stop_path) == 0);
  write_file(c.stop_path, "fzero.session-stop@1\n0123456789abcdef0123456789abcdef\n");
  CHECK(FzeroPlaythroughRecordFrame(&p, 0x40000100, ram, sizeof(ram)));
  CHECK(FzeroSessionControlStop(&c, &ui) == 1);
  CHECK(FzeroSessionFinish(&ui, &p, true, 100));
  CHECK(FzeroSessionControlPublish(&c, &ui, &p));
  contains(path, "\"state\":\"saved\""); contains(path, "\"frames\":1");
  CHECK(FzeroSessionControlStop(&c, &ui) == 0);
  CHECK(FzeroPlaythroughPlaybackOpen(&reader, tape, sha));
  uint32_t word; CHECK(FzeroPlaythroughNextInput(&reader, &word) && word == 0x40000100);
  CHECK(FzeroPlaythroughVerifyFrame(&reader, ram, sizeof(ram)));
  CHECK(FzeroPlaythroughClose(&reader)); /* same tape, not a second replay engine */
  CHECK(remove(path) == 0);
  CHECK(!FzeroSessionControlOpen(&other, path, id, FZERO_SESSION_RECORD, false)); /* stale Stop */
  CHECK(remove(c.stop_path) == 0);
  CHECK(FzeroSessionControlOpen(&c, path, id, FZERO_SESSION_REPLAY, false));
  CHECK(c.muted);
  CHECK(FzeroPlaythroughPlaybackOpen(&p, tape, sha)); FzeroSessionBegin(&ui, 2, false);
  CHECK(!FzeroSessionFinish(&ui, &p, true, 100)); /* early close remains failed */
  CHECK(FzeroSessionControlPublish(&c, &ui, &p)); contains(path, "\"state\":\"failed\"");
  CHECK(remove(path) == 0); CHECK(remove(tape) == 0);
  write_file(c.temporary, "retained failed publication");
  CHECK(!FzeroSessionControlOpen(&other, path, id, FZERO_SESSION_RECORD, false));
  CHECK(remove(c.temporary) == 0);
  /* A normal status reader without FILE_SHARE_DELETE must not corrupt the
   * recorder. Continue recording, stop normally, then recover publication. */
  CHECK(FzeroSessionControlOpen(&c, path, id, FZERO_SESSION_RECORD, false));
  CHECK(FzeroPlaythroughRecordOpen(&p, tape, sha)); FzeroSessionBegin(&ui, 1, false);
  FzeroSessionControlRom(&c, sha); CHECK(FzeroSessionControlPublish(&c, &ui, &p));
  HANDLE lock = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  CHECK(lock != INVALID_HANDLE_VALUE);
  for (unsigned i = 0; i < 60; ++i) CHECK(FzeroPlaythroughRecordFrame(&p, 0x40000100, ram, sizeof(ram)));
  CHECK(!FzeroSessionControlPublish(&c, &ui, &p));
  CHECK(c.owns_temporary && p.mode == 1 && ui.state == FZERO_SESSION_REC);
  CHECK(FzeroPlaythroughRecordFrame(&p, 0x40000100, ram, sizeof(ram)));
  write_file(c.stop_path, "fzero.session-stop@1\n0123456789abcdef0123456789abcdef\n");
  CHECK(FzeroSessionControlStop(&c, &ui) == 1);
  CHECK(FzeroSessionFinish(&ui, &p, true, 100));
  CHECK(!FzeroSessionControlPublish(&c, &ui, &p));
  CHECK(CloseHandle(lock));
  CHECK(FzeroSessionControlPublish(&c, &ui, &p));
  CHECK(!c.owns_temporary); contains(path, "\"state\":\"saved\""); contains(path, "\"frames\":61");
  CHECK(FzeroPlaythroughPlaybackOpen(&reader, tape, sha));
  for (unsigned i = 0; i < 61; ++i) {
    CHECK(FzeroPlaythroughNextInput(&reader, &word) && word == 0x40000100);
    CHECK(FzeroPlaythroughVerifyFrame(&reader, ram, sizeof(ram)));
  }
  CHECK(FzeroPlaythroughClose(&reader));
  CHECK(remove(c.stop_path) == 0); CHECK(remove(path) == 0); CHECK(remove(tape) == 0);
  printf("ordinary session control: %u checks\n", checks);
  return 0;
}
