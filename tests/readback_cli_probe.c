/* Console-only ingress probe: invokes the real production parser, opens no game. */
#include "fzero_session_ui.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
  FzeroSessionOptions options;
  const char *error = NULL;
  const char *replay = getenv("FZERO_REPLAY_PLAYTHROUGH");
  if (!FzeroSessionParse(argc, argv, replay && *replay, &options, &error)) {
    fprintf(stderr, "%s\n", error);
    return 2;
  }
  printf("mode=%d env=%d\n", options.mode, replay && *replay ? 1 : 0);
  return 0;
}
