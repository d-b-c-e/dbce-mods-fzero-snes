#include "fzero_surface_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(e) do { if (!(e)) { fprintf(stderr, "%d: %s\n", __LINE__, #e); exit(1); } } while (0)

int main(void) {
  static uint8_t ram[0x20000];
  char line[160];
  ram[0x0b70] = 0x34; ram[0x0b71] = 0xf2;  /* x keeps 13 bits: 0x1234 */
  ram[0x0b90] = 0x78; ram[0x0b91] = 0xf6;  /* y keeps 12 bits: 0x678 */
  ram[0xc7] = 0x10; ram[0xc9] = 0xd0; ram[0xca] = 0x07;
  /* Each traced byte has a distinct value, so a swapped field fails. */
  ram[0x0d40] = 0xff; ram[0x0d41] = 0x02;  /* player slot only, not car 1 */
  ram[0xc0] = 0x01; ram[0xc1] = 0x23; ram[0xc2] = 0x45;
  /* The lap count and all 15 record bytes, each distinct; $0E8F and $0E9F are outside. */
  ram[0x0f53] = 0x02;
  ram[0x0e8f] = 0xee; ram[0x0e9f] = 0xdd;
  for (int i = 0; i < 15; ++i) ram[0x0e90 + i] = (uint8_t)(0xa0 + i);
  CHECK(FzeroSurfaceTraceLine(line, sizeof(line), 4321, ram, 0x81u) > 0);
  CHECK(strcmp(line, "[fzero-surface] frame=4321 x=4660 y=1656 c7=10 energy=2000 "
                     "input=00000081 d40=ff c0=01 c1=23 c2=45 "
                     "f53=02 e90=a0a1a2a3a4a5a6a7a8a9aaabacadae\n") == 0);
  ram[0x0d40] = 0x00;
  CHECK(FzeroSurfaceTraceLine(line, sizeof(line), 4322, ram, 0u) > 0);
  CHECK(strstr(line, " d40=00 c0=01 c1=23 c2=45 f53=02 e90=") != NULL);
  /* The widest values still fit the 160-byte line the headless runner uses. */
  memset(ram, 0xff, sizeof(ram));
  CHECK(FzeroSurfaceTraceLine(line, sizeof(line), 2147483647L, ram, 0xffffffffu) < (int)sizeof(line));
  puts("F-Zero surface trace line tests passed");
  return 0;
}
