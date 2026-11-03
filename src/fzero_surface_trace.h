#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* One FZERO_SURFACE_TRACE line for a racing frame. Fields are raw WRAM bytes
 * named by address, uninterpreted. d40 is the player's slot of the per-car
 * $0D40 line counter, c0..c2 the BCD race clock, f53 the count of laps the
 * game has timed and e90 the 15 lap-record bytes $0E90-$0E9E (five clock
 * times; TELEMETRY_SIGNALS.md). New fields go last so existing readers keep
 * their positions. The longest line is 148 bytes (headless_main.c has 160). */
static inline int FzeroSurfaceTraceLine(char *out, size_t size, long frame,
                                        const uint8_t *ram, uint32_t input) {
  char records[31];
  for (int i = 0; i < 15; ++i)
    snprintf(records + 2 * i, 3, "%02x", ram[0x0e90 + i]);
  return snprintf(out, size,
                  "[fzero-surface] frame=%ld x=%u y=%u c7=%02x energy=%u input=%08x "
                  "d40=%02x c0=%02x c1=%02x c2=%02x f53=%02x e90=%s\n",
                  frame,
                  (unsigned)((ram[0x0b70] | ((unsigned)ram[0x0b71] << 8)) & 0x1fff),
                  (unsigned)((ram[0x0b90] | ((unsigned)ram[0x0b91] << 8)) & 0x0fff),
                  ram[0xc7], (unsigned)(ram[0xc9] | ((unsigned)ram[0xca] << 8)),
                  (unsigned)input, ram[0x0d40], ram[0xc0], ram[0xc1], ram[0xc2],
                  ram[0x0f53], records);
}
