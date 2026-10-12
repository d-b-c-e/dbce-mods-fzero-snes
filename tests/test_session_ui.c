#include "fzero_session_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
static void parse(bool expected, bool env, int argc, char **argv, enum FzeroSessionMode mode) {
  FzeroSessionOptions o;
  const char *error = NULL;
  CHECK(FzeroSessionParse(argc, argv, env, &o, &error) == expected);
  CHECK(expected ? error == NULL : error && *error);
  if (expected) CHECK(o.mode == mode);
}
int main(void) {
  char *normal[] = {"game"};
  char *launcher[] = {"game", "--launcher", "rom.sfc"};
  char *record[] = {"game", "--record", "new folder/drive.fzpt", "rom.sfc"};
  char *replay[] = {"game", "rom.sfc", "--replay", "drive.fzpt"};
  char *preview[] = {"game", "--shared-profile-preview", "E:/fz-preview", "rom.sfc"};
  parse(true, false, 1, normal, FZERO_SESSION_NONE);
  parse(true, true, 3, launcher, FZERO_SESSION_NONE); /* old environment requests unchanged */
  parse(true, false, 4, record, FZERO_SESSION_RECORD);
  parse(true, false, 4, replay, FZERO_SESSION_REPLAY);
  parse(true, false, 4, preview, FZERO_SESSION_PREVIEW);
  parse(false, true, 4, record, FZERO_SESSION_NONE);
  parse(false, true, 4, replay, FZERO_SESSION_NONE);
  parse(false, true, 4, preview, FZERO_SESSION_NONE); /* cannot become injected preview */
  parse(false, false, 2, record, FZERO_SESSION_NONE); /* no path */
  parse(false, false, 3, record, FZERO_SESSION_NONE); /* no ROM */
  char *double_mode[] = {"game", "--record", "a.fzpt", "--replay", "b.fzpt", "rom.sfc"};
  char *duplicate[] = {"game", "--record", "a.fzpt", "--record", "b.fzpt", "rom.sfc"};
  char *launch_record[] = {"game", "--launcher", "--record", "a.fzpt", "rom.sfc"};
  char *bad_path[] = {"game", "--record", "--replay", "rom.sfc"};
  char *empty[] = {"game", "--record", "", "rom.sfc"};
  char *unknown[] = {"game", "--recording", "rom.sfc"};
  char *two_roms[] = {"game", "first.sfc", "second.sfc"};
  char *escaped[] = {"game", "--", "--launcher"};
  parse(false, false, 6, double_mode, FZERO_SESSION_NONE);
  parse(false, false, 6, duplicate, FZERO_SESSION_NONE);
  parse(false, false, 5, launch_record, FZERO_SESSION_NONE);
  parse(false, false, 4, bad_path, FZERO_SESSION_NONE);
  parse(false, false, 4, empty, FZERO_SESSION_NONE);
  parse(false, false, 3, unknown, FZERO_SESSION_NONE);
  parse(false, false, 3, two_roms, FZERO_SESSION_NONE);
  parse(true, false, 3, escaped, FZERO_SESSION_NONE);
  FzeroSessionOptions o; const char *error;
  CHECK(FzeroSessionParse(4, record, false, &o, &error));
  CHECK(!strcmp(o.path, "new folder/drive.fzpt") && !strcmp(o.rom, "rom.sfc"));
  CHECK(!FzeroSessionStopKey(1, true, false, false, false)); /* plain F11 stays save slot */
  CHECK(!FzeroSessionStopKey(0, true, true, false, false));
  CHECK(!FzeroSessionStopKey(2, true, true, false, false));
  CHECK(!FzeroSessionStopKey(1, false, true, false, false));
  CHECK(!FzeroSessionStopKey(1, true, true, true, false));
  CHECK(!FzeroSessionStopKey(1, true, true, false, true));
  CHECK(FzeroSessionStopKey(1, true, true, false, false));

  const char *path = "fzero-session-ui-test.fzpt";
  uint8_t sha[32] = {1}, ram[32] = {0};
  FzeroPlaythrough p = {0}, reader = {0}; FzeroSessionUi ui;
  char top[80], bottom[96];
  FzeroSessionBegin(&ui, 0, false);
  CHECK(!FzeroSessionCaption(&ui, 0, 0, 0, top, sizeof(top), bottom, sizeof(bottom)));
  CHECK(FzeroPlaythroughRecordOpen(&p, path, sha));
  FzeroSessionBegin(&ui, 1, true);
  CHECK(FzeroSessionCaption(&ui, 60, 0, 0, top, sizeof(top), bottom, sizeof(bottom)));
  CHECK(strstr(top, "MUTED PREVIEW") && strstr(bottom, "save and close"));
  CHECK(FzeroPlaythroughRecordFrame(&p, 0x40000100, ram, sizeof(ram)));
  CHECK(ui.state == FZERO_SESSION_REC); /* writing a frame alone is not Saved */
  CHECK(FzeroSessionFinish(&ui, &p, true, 100));
  CHECK(ui.state == FZERO_SESSION_SAVED && !ui.failed && p.mode == 0);
  CHECK(FzeroSessionCaption(&ui, p.frames, 0, 5099, top, sizeof(top), bottom, sizeof(bottom)));
  CHECK(!FzeroSessionCaption(&ui, p.frames, 0, 5100, top, sizeof(top), bottom, sizeof(bottom)));
  CHECK(FzeroPlaythroughPlaybackOpen(&reader, path, sha));
  FzeroSessionBegin(&ui, 2, false);
  CHECK(FzeroSessionCaption(&ui, 0, 1, 0, top, sizeof(top), bottom, sizeof(bottom)));
  CHECK(strstr(top, "REPLAY") && strstr(bottom, "forces off"));
  uint32_t word;
  CHECK(FzeroPlaythroughNextInput(&reader, &word) && word == 0x40000100);
  CHECK(FzeroPlaythroughVerifyFrame(&reader, ram, sizeof(ram)));
  CHECK(FzeroSessionFinish(&ui, &reader, true, 0));
  CHECK(ui.state == FZERO_SESSION_VERIFIED);
  CHECK(FzeroPlaythroughPlaybackOpen(&reader, path, sha));
  FzeroSessionBegin(&ui, 2, false);
  CHECK(!FzeroSessionFinish(&ui, &reader, true, 0)); /* early close cannot show verified */
  CHECK(ui.state == FZERO_SESSION_ERROR && ui.failed);
  CHECK(remove(path) == 0);

  /* A shared capture failing after actual guest frames must not seal .fzpt. */
  CHECK(FzeroPlaythroughRecordOpen(&p, path, sha));
  CHECK(FzeroPlaythroughRecordFrame(&p, 0x40000001, ram, sizeof(ram)));
  FzeroSessionBegin(&ui, 1, true);
  CHECK(!FzeroSessionFinish(&ui, &p, false, 0));
  CHECK(ui.failed && ui.state == FZERO_SESSION_ERROR);
  CHECK(!FzeroPlaythroughPlaybackOpen(&reader, path, sha));
  CHECK(remove(path) == 0);

  CHECK(FzeroPlaythroughRecordOpen(&p, path, sha));
  FzeroSessionBegin(&ui, 1, false);
  CHECK(!FzeroSessionFinish(&ui, &p, true, 0)); /* empty close */
  CHECK(ui.failed && !FzeroPlaythroughPlaybackOpen(&reader, path, sha));
  CHECK(remove(path) == 0);

  CHECK(FzeroPlaythroughRecordOpen(&p, path, sha));
  CHECK(FzeroPlaythroughRecordFrame(&p, 0x40000000, ram, sizeof(ram)));
  FzeroSessionBegin(&ui, 1, false);
  FzeroPlaythroughAbort(&p); FzeroSessionFail(&ui, "loaded save state");
  CHECK(!FzeroSessionFinish(&ui, &p, true, 100)); /* sticky even though writer now closed */
  CHECK(FzeroSessionCaption(&ui, 1, 0, 999999, top, sizeof(top), bottom, sizeof(bottom)));
  CHECK(strstr(top, "ERROR") && !strcmp(bottom, "loaded save state"));
  CHECK(!FzeroPlaythroughPlaybackOpen(&reader, path, sha));
  CHECK(remove(path) == 0);

  /* Real stdio failure while sealing (read-only stream) must never say Saved. */
  CHECK(FzeroPlaythroughRecordOpen(&p, path, sha));
  FzeroPlaythroughAbort(&p);
  p.stream = fopen(path, "rb"); p.mode = 1; p.frames = 1;
  CHECK(p.stream != NULL);
  FzeroSessionBegin(&ui, 1, false);
  CHECK(!FzeroSessionFinish(&ui, &p, true, 0));
  CHECK(ui.failed && ui.state == FZERO_SESSION_ERROR);
  CHECK(!FzeroPlaythroughPlaybackOpen(&reader, path, sha));
  CHECK(remove(path) == 0);
  printf("F-Zero session CLI/recorder UI: %u checks\n", checks);
  return 0;
}
