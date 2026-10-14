#include "fzero_session_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static bool live_input(void *context, uint32_t *word) {
  ++*(unsigned *)context;
  *word = 0xc0000804u; /* physical/host flags and a Select+R gesture */
  return true;
}
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
  char *controlled[] = {"game", "--record", "a.fzpt", "--session-id",
      "0123456789abcdef0123456789abcdef", "--session-status", "fresh/status.json", "rom.sfc", "--mute-outputs"};
  CHECK(FzeroSessionParse(9, controlled, false, &o, &error));
  CHECK(o.mute_outputs && !strcmp(o.status_path, "fresh/status.json"));
  CHECK(!strcmp(o.request_id, controlled[4]));
  parse(false, true, 9, controlled, FZERO_SESSION_NONE);
  controlled[1] = "--replay";
  parse(false, false, 9, controlled, FZERO_SESSION_NONE); /* replay already mutes */
  parse(true, false, 8, controlled, FZERO_SESSION_REPLAY);
  controlled[1] = "--shared-profile-preview";
  parse(false, false, 8, controlled, FZERO_SESSION_NONE); /* independent transport */
  controlled[1] = "--record"; controlled[4] = "0123456789abcdef0123456789abcdeF";
  parse(false, false, 8, controlled, FZERO_SESSION_NONE);
  controlled[4] = "0123456789abcdef0123456789abcdefEXTRA";
  parse(false, false, 8, controlled, FZERO_SESSION_NONE);
  char *isolated[] = {"game", "--record", "a.fzpt", "--session-id",
      "0123456789abcdef0123456789abcdef", "--session-status", "fresh/status.json", "rom.sfc",
      "--isolate-owner-state", "C:/private-save", "--mute-outputs"};
  CHECK(FzeroSessionParse(11, isolated, false, &o, &error));
  CHECK(!strcmp(o.isolated_save_root, "C:/private-save") && o.mute_outputs);
  char *witness[] = {"game", "--record", "a.fzpt", "--session-id",
      "0123456789abcdef0123456789abcdef", "--session-status", "fresh/status.json", "rom.sfc",
      "--isolate-owner-state", "C:/private-save", "--mute-outputs", "--route-witness"};
  CHECK(FzeroSessionParse(12, witness, false, &o, &error) && o.route_witness);
  parse(false, false, 13, (char *[]){"game", "--record", "a.fzpt", "--session-id",
      "0123456789abcdef0123456789abcdef", "--session-status", "fresh/status.json", "rom.sfc",
      "--isolate-owner-state", "C:/private-save", "--mute-outputs", "--route-witness", "--route-witness"},
      FZERO_SESSION_NONE);
  witness[1] = "--replay";
  parse(false, false, 12, witness, FZERO_SESSION_NONE);
  witness[1] = "--record"; witness[10] = "--route-witness";
  parse(false, false, 11, witness, FZERO_SESSION_NONE); /* mute remains mandatory */
  char *not_isolated[] = {"game", "--record", "a.fzpt", "--session-id",
      "0123456789abcdef0123456789abcdef", "--session-status", "fresh/status.json", "rom.sfc",
      "--mute-outputs", "--route-witness"};
  parse(false, false, 10, not_isolated, FZERO_SESSION_NONE);
  parse(false, false, 10, isolated, FZERO_SESSION_NONE); /* isolation does not imply mute */
  parse(false, false, 9, isolated, FZERO_SESSION_NONE); /* missing root */
  parse(false, true, 11, isolated, FZERO_SESSION_NONE); /* no diagnostic/script transport */
  isolated[1] = "--replay";
  parse(false, false, 11, isolated, FZERO_SESSION_NONE);
  isolated[1] = "--record"; isolated[10] = "--isolate-owner-state";
  parse(false, false, 11, isolated, FZERO_SESSION_NONE); /* duplicate */
  CHECK(FzeroSessionParse(4, record, false, &o, &error));
  CHECK(!o.isolated_save_root && !o.mute_outputs); /* Human ordinary unchanged */
  char *no_record[] = {"game", "rom.sfc", "--session-id", "0123456789abcdef0123456789abcdef", "--session-status", "status.json"};
  parse(false, false, 6, no_record, FZERO_SESSION_NONE);
  parse(false, false, 4, no_record, FZERO_SESSION_NONE);
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
  FzeroSessionFail(&ui, "later game fault after Stop");
  CHECK(FzeroSessionFinish(&ui, &p, false, 101));
  CHECK(ui.state == FZERO_SESSION_SAVED && !ui.failed); /* game exit is separate */
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
  /* Production dispatch must never call the physical reader/host-action path
   * while a tape owns input, even on EOF. Gesture-shaped recorded words remain
   * guest input: the host must not consume them as a live menu command. */
  unsigned live_calls = 0;
  CHECK(FzeroSessionReadInput(&p, live_input, &live_calls, &word));
  CHECK(live_calls == 1 && word == 0xc0000804u);
  CHECK(FzeroPlaythroughRecordOpen(&p, path, sha));
  CHECK(FzeroPlaythroughRecordFrame(&p, 0x40000804u, ram, sizeof(ram)));
  CHECK(FzeroPlaythroughClose(&p));
  CHECK(FzeroPlaythroughPlaybackOpen(&reader, path, sha));
  CHECK(FzeroSessionReadInput(&reader, live_input, &live_calls, &word));
  CHECK(live_calls == 1 && word == 0x40000804u);
  CHECK(FzeroPlaythroughVerifyFrame(&reader, ram, sizeof(ram)));
  CHECK(!FzeroSessionReadInput(&reader, live_input, &live_calls, &word));
  CHECK(live_calls == 1);
  CHECK(FzeroPlaythroughClose(&reader));
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
