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
static void lacks(const char *path, const char *text) {
  char bytes[2048] = {0}; FILE *f = fopen(path, "rb"); CHECK(f != NULL);
  size_t n = fread(bytes, 1, sizeof(bytes) - 1, f); CHECK(n > 0 && feof(f)); CHECK(fclose(f) == 0);
  CHECK(strstr(bytes, text) == NULL);
}
int main(void) {
  char dir[MAX_PATH], path[MAX_PATH], tape[MAX_PATH];
  CHECK(GetTempPathA(sizeof(dir), dir) != 0);
  char private_root[80], bindings[MAX_PATH], private_bindings[96];
  snprintf(private_root, sizeof(private_root), "%sfza-%lu", dir, GetCurrentProcessId());
  snprintf(bindings, sizeof(bindings), "%sfza-%lu.ini", dir, GetCurrentProcessId());
  snprintf(private_bindings, sizeof(private_bindings), "%s/keybinds.ini", private_root);
  CHECK(!FzeroSessionCreatePrivateRoot("relative", bindings));
  CHECK(!FzeroSessionCreatePrivateRoot(dir, bindings)); /* existing owner location */
  write_file(bindings, "[Player1]\nB = Z\n");
  CHECK(FzeroSessionCreatePrivateRoot(private_root, bindings));
  contains(private_bindings, "B = Z"); contains(bindings, "B = Z");
  CHECK(!FzeroSessionCreatePrivateRoot(private_root, bindings)); /* no reuse or overwrite */
  contains(private_bindings, "B = Z"); contains(bindings, "B = Z");
  CHECK(remove(private_bindings) == 0); CHECK(RemoveDirectoryA(private_root));
  CHECK(remove(bindings) == 0);
  CHECK(FzeroSessionCreatePrivateRoot(private_root, bindings)); /* missing keybind owner stays absent */
  CHECK(GetFileAttributesA(bindings) == INVALID_FILE_ATTRIBUTES);
  CHECK(GetFileAttributesA(private_bindings) == INVALID_FILE_ATTRIBUTES);
  CHECK(RemoveDirectoryA(private_root));
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
  CHECK(!FzeroSessionControlWitnessEnable(&c)); /* unmuted sessions cannot opt in */
  CHECK(!FzeroSessionControlOpen(&other, path, id, FZERO_SESSION_RECORD, false));
  contains(path, "\"state\":\"starting\"");
  lacks(path, "routeWitness"); /* default-off status stays byte-shape compatible */
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
  CHECK(FzeroSessionControlOpen(&c, path, id, FZERO_SESSION_RECORD, true));
  CHECK(FzeroSessionControlWitnessEnable(&c));
  CHECK(!FzeroSessionControlWitnessEnable(&c));
  CHECK(FzeroPlaythroughRecordOpen(&p, tape, sha)); FzeroSessionBegin(&ui, 1, false);
  FzeroSessionControlRom(&c, sha);
  CHECK(FzeroSessionControlPublish(&c, &ui, &p));
  contains(path, "\"routeWitness\":{\"valid\":false,\"limited\":false}");
  uint8_t state[3] = {0, 1, 0};
  uint32_t menu[12] = {0}; menu[1] = 1;
  CHECK(!FzeroSessionControlWitnessFrame(&c, &ui, &p, 0, 0, state, menu, 0, 0));
  CHECK(FzeroPlaythroughRecordFrame(&p, 0x40000100, ram, sizeof(ram)));
  CHECK(FzeroSessionControlWitnessFrame(&c, &ui, &p, 0, 0x40000100, state, menu, 0, 0));
  contains(path, "\"requestId\":\"0123456789abcdef0123456789abcdef\"");
  contains(path, "\"processCreationFileTime\":");
  contains(path, "\"frames\":1"); contains(path, "\"simulationFrame\":0");
  contains(path, "\"inputWord\":1073742080"); contains(path, "\"training\":1");
  CHECK(!FzeroSessionControlWitnessFrame(&c, &ui, &p, 0, 0, state, menu, 0, 0));
  state[0] = 2; state[1] = 2; state[2] = 2; menu[11] = 0;
  CHECK(FzeroPlaythroughRecordFrame(&p, 0x40000081, ram, sizeof(ram)));
  CHECK(FzeroSessionControlWitnessFrame(&c, &ui, &p, 1, 0x40000081, state, menu, 0, 0));
  contains(path, "\"simulationFrame\":1"); contains(path, "\"state54\":2,\"state55\":2,\"state56\":2");
  state[1] = 3; state[2] = 0;
  CHECK(FzeroPlaythroughRecordFrame(&p, 0x40000081, ram, sizeof(ram)));
  CHECK(FzeroSessionControlWitnessFrame(&c, &ui, &p, 2, 0x40000081, state, menu, 2.5, 1.25));
  contains(path, "\"simulationFrame\":2"); contains(path, "\"speedMps\":2.500000");
  contains(path, "\"training\":1"); contains(path, "\"state5b\":0");
  CHECK(!FzeroSessionControlWitnessFrame(&c, &ui, &p, 2, 0, state, menu, -1, 0));
  FzeroSessionUi terminal_view = ui; terminal_view.state = FZERO_SESSION_SAVED;
  CHECK(FzeroSessionControlPublish(&c, &terminal_view, &p));
  contains(path, "\"routeWitness\":{\"valid\":false,\"limited\":false}");
  p.frames = 6001; /* synthetic cap boundary; no extra tape bytes are written */
  CHECK(FzeroSessionControlWitnessFrame(&c, &ui, &p, 6000, 0, state, menu, 0, 0));
  contains(path, "\"routeWitness\":{\"valid\":false,\"limited\":true}");
  p.frames = 3;
  CHECK(FzeroSessionFinish(&ui, &p, true, 100));
  CHECK(FzeroSessionControlPublish(&c, &ui, &p));
  contains(path, "\"state\":\"saved\"");
  contains(path, "\"routeWitness\":{\"valid\":false,\"limited\":true}");
  CHECK(remove(path) == 0); CHECK(remove(tape) == 0);
  printf("ordinary session control: %u checks\n", checks);
  return 0;
}
