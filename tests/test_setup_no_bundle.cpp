// Compile/run against CMake's actual optional no-bundle identity header.
#include "fzero_setup.h"
#include "fzero_setup_identity.h"
#include <cstdio>
static_assert(FZERO_SETUP_QUALIFIED == 0, "This target must use the real no-bundle header");
static_assert(FZERO_SETUP_REPORT_QUALIFIED == 0, "No helper cannot advertise reports");
int main() {
  char why[128] = {};
  if (FzeroSetupAvailable() || FzeroReportAvailable() ||
      FzeroReportLaunch("fixture", "fixture", "fzero-video.ini", why, sizeof(why)) || !why[0]) return 1;
  if (FzeroSetupLaunch(why, sizeof(why)) || !why[0]) return 2;
  std::puts("No-bundle generated header: controls and report unavailable; no child launched");
  return 0;
}
