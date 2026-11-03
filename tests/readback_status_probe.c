/* Actual production status writer, without SDL, ROM, devices or a game. */
#include "fzero_session_control.h"
#include <windows.h>
#include <stdio.h>
int main(int argc, char **argv) {
  if (argc != 4) return 2;
  FzeroSessionControl c;
  const char *id = "0123456789abcdef0123456789abcdef";
  if (!FzeroSessionControlOpen(&c, argv[1], id, FZERO_SESSION_REPLAY, false) ||
      !CopyFileA(argv[1], argv[2], TRUE)) return 3;
  const uint8_t sha[32] = {
    0xbf,0x16,0xc3,0xc8,0x67,0xc5,0x8e,0x2a,0xb0,0x61,0xc7,0x0d,0xe9,0x29,0x5b,0x69,
    0x30,0xd6,0x3f,0x29,0xf8,0x1c,0xc9,0x86,0xf5,0xec,0xae,0x03,0xe0,0xad,0x18,0xd2};
  /* Identity comes from the writer; this SHA is test data, not a ROM read. */
  FzeroSessionControlRom(&c, sha);
  FzeroSessionUi ui = {0}; ui.state = FZERO_SESSION_PLAYING;
  FzeroPlaythrough p = {0}; p.mode = 2; p.frames = 60; p.total = 3600;
  if (!FzeroSessionControlPublish(&c, &ui, &p) || !CopyFileA(argv[1], argv[3], TRUE)) return 4;
  return 0;
}
