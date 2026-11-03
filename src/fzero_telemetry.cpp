#include "fzero_telemetry.h"

extern "C" {
#include "fzero_hotkeys.h"
}
#include "../lib/toolkit/include/forza_packet.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET FzeroSocket;
static const FzeroSocket kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int FzeroSocket;
static const FzeroSocket kInvalidSocket = -1;
#endif

namespace {
FzeroSocket s_socket = kInvalidSocket;
sockaddr_storage s_destination{};
socklen_t s_destination_size = 0;
FzeroTelemetryState s_state{};
#ifdef _WIN32
bool s_winsock_started = false;
#endif

uint16_t read16(const uint8_t *p) {
  return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

/* About four times measured race movement; anything longer in one frame is a jump. */
const int kMaxStepUnits = 32;

int wrapped_delta(uint16_t current, uint16_t previous, int modulus) {
  int delta = (int)current - (int)previous;
  if (delta > modulus / 2) delta -= modulus;
  if (delta < -modulus / 2) delta += modulus;
  return delta;
}

/* $00:8ECE ticks the race clock in decimal mode and the HUD ($00:A880) draws
 * its nibbles: minutes, seconds and hundredths are BCD bytes. The lap records
 * copy it. Seconds of one such time, or -1 when the bytes are not one. */
float bcd_seconds(const uint8_t *time) {
  for (int i = 0; i < 3; ++i)
    if ((time[i] >> 4) > 9 || (time[i] & 15) > 9) return -1.0f;
  if (time[1] >= 0x60) return -1.0f;
  const auto value = [](uint8_t b) { return (float)((b >> 4) * 10 + (b & 15)); };
  return value(time[0]) * 60.0f + value(time[1]) + value(time[2]) * 0.01f;
}

void close_socket(void) {
  if (s_socket == kInvalidSocket) return;
#ifdef _WIN32
  closesocket(s_socket);
#else
  close(s_socket);
#endif
  s_socket = kInvalidSocket;
}
}  // namespace

size_t FzeroTelemetryBuild(FzeroTelemetryState *state, const uint8_t *ram,
                           size_t ram_size, uint32_t input,
                           uint8_t out[324]) {
  if (!state || !ram || ram_size < 0x0be2 || !out) return 0;

  using namespace dbce::forza;
  Sled sled{};
  Dash dash{};

  const bool racing = ram[0x54] == 2 && ram[0x55] >= 3;
  const uint16_t x = read16(ram + 0x0b70) & 0x1fff;
  const uint16_t y = read16(ram + 0x0b90) & 0x0fff;
  if (racing && state->have_position) {
    const int dx = wrapped_delta(x, state->previous_x, 0x2000);
    const int dy = wrapped_delta(y, state->previous_y, 0x1000);
    /* Racing moves about 7 units a frame (TELEMETRY_SIGNALS.md). A save-state
     * load or rewind moves the craft further in one frame: hold speed and
     * distance and count again from the new position. */
    if (dx * dx + dy * dy <= kMaxStepUnits * kMaxStepUnits) {
      /* F-Zero's world is 8192 x 4096 coordinate units.  One quarter metre per
       * unit makes the stock top-speed movement agree with the HUD to within
       * the precision useful to a dashboard; keep the scale isolated here so a
       * measured calibration can replace it without touching the wire format. */
      const float distance = std::sqrt((float)(dx * dx + dy * dy)) * 0.25f;
      const float instantaneous = distance * 60.098811862f;
      state->speed_mps += (instantaneous - state->speed_mps) * 0.25f;
      state->total_distance_m += distance;
    }
  } else {
    state->speed_mps = 0.0f;
    if (!racing) state->total_distance_m = 0.0f;
  }
  state->previous_x = x;
  state->previous_y = y;
  state->have_position = racing ? 1 : 0;
  state->timestamp_ms += 17;

  sled.isRaceOn = racing ? 1 : 0;
  sled.timestampMs = state->timestamp_ms;
  sled.engineMaxRpm = 12000.0f;
  sled.engineIdleRpm = 1200.0f;
  sled.currentEngineRpm = racing ? 1200.0f +
      std::min(state->speed_mps / 200.0f, 1.0f) * 10800.0f : 0.0f;
  sled.yaw = (float)ram[0x0be1] * (6.28318530718f / 192.0f);
  sled.velZ = state->speed_mps;
  sled.carOrdinal = ram[0x52];
  sled.drivetrain = 2;

  dash.posX = (float)x * 0.25f;
  dash.posZ = (float)y * 0.25f;
  dash.speed = state->speed_mps;
  dash.distance = state->total_distance_m;
  const float race = std::max(0.0f, bcd_seconds(ram + 0xc0));
  dash.currentLap = race;
  dash.currentRaceTime = race;
  dash.fuel = std::max(0.0f, std::min((float)(int16_t)read16(ram + 0xc9) /
                                      1024.0f, 1.0f));
  /* $00:A8B1 renders this 1-based rank. Short snapshots and invalid values
   * retain unknown (zero). */
  if (racing && ram_size > 0x0dc8 && ram[0x0dc8] >= 1 && ram[0x0dc8] <= 20)
    dash.racePosition = ram[0x0dc8];
  /* Completed laps are the laps the game has timed (TELEMETRY_SIGNALS.md):
   * race start clears $0F53 and the lap records, and $00:99C0 stores the race
   * clock for lap L at $0E90 + 3(L - 1) and sets $0F53 = L in the same frame.
   * $0F53 never falls within a race; lap 5 is the finish. Records that are not
   * BCD times in order up to the clock leave the lap fields unknown (zero) and
   * currentLap on the race clock, which is lap 1's time. */
  if (racing && ram_size > 0x0f53 && ram[0x0f53] <= 5) {
    const int laps = ram[0x0f53];
    float previous = 0.0f, last = 0.0f, best = 0.0f;
    bool valid = race > 0.0f || laps == 0;
    for (int lap = 1; valid && lap <= laps; ++lap) {
      const float at = bcd_seconds(ram + 0x0e90 + 3 * (lap - 1));
      valid = at >= previous && at <= race;
      last = at - previous;
      best = lap == 1 ? last : std::min(best, last);
      previous = at;
    }
    if (valid) {
      dash.lapNumber = (uint16_t)laps;
      dash.lastLap = last;
      dash.bestLap = best;
      dash.currentLap = race - previous;
    }
  }
  dash.accel = (input & 0x0001u) ? 255 : 0;  /* SNES B */
  dash.brake = (input & 0x0002u) ? 255 : 0;  /* SNES Y */
  dash.gear = 1;
  dash.steer = (input & 0x0040u) ? -127 : (input & 0x0080u) ? 127 : 0;

  return (size_t)build(FORZA_HORIZON_324, sled, dash, out, 324, 'F');
}

static bool s_no_delivery;
int FzeroTelemetryLatchNoDelivery(void) {
  if (s_socket != kInvalidSocket) return 0;
#ifdef _WIN32
  if (s_winsock_started) return 0;
#endif
  s_no_delivery = true;
  return 1;
}
int FzeroTelemetryDeliveryBlocked(void) { return s_no_delivery && s_socket == kInvalidSocket; }
void FzeroTelemetryInit(const char *config_path) {
  if (s_no_delivery) return;
  FzeroTelemetryShutdown();
  int enabled = 0, port = 8000;
  char host[256] = "127.0.0.1";
  if (!FzeroIniReadInt(config_path, "Telemetry", "Enabled", &enabled) ||
      !enabled) return;
  FzeroIniReadInt(config_path, "Telemetry", "Port", &port);
  FzeroIniReadString(config_path, "Telemetry", "Host", host, sizeof(host));
  if (port < 1 || port > 65535) {
    std::fprintf(stderr, "[fzero-telemetry] invalid port %d; disabled\n", port);
    return;
  }

#ifdef _WIN32
  WSADATA data;
  if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return;
  s_winsock_started = true;
#endif
  addrinfo hints{}, *result = nullptr;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  char service[16];
  std::snprintf(service, sizeof(service), "%d", port);
  if (getaddrinfo(host, service, &hints, &result) != 0 || !result) {
    std::fprintf(stderr, "[fzero-telemetry] cannot resolve %s:%d; disabled\n",
                 host, port);
    FzeroTelemetryShutdown();
    return;
  }
  s_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (s_socket != kInvalidSocket &&
      result->ai_addrlen <= sizeof(s_destination)) {
    std::memcpy(&s_destination, result->ai_addr, result->ai_addrlen);
    s_destination_size = (socklen_t)result->ai_addrlen;
  } else {
    close_socket();
  }
  freeaddrinfo(result);
  if (s_socket != kInvalidSocket)
    std::fprintf(stderr, "[fzero-telemetry] Forza Horizon 324 -> %s:%d\n",
                 host, port);
  else
    FzeroTelemetryShutdown();
}

void FzeroTelemetryFrame(const uint8_t *ram, size_t ram_size, uint32_t input) {
  if (s_no_delivery || s_socket == kInvalidSocket) return;
  uint8_t packet[324];
  size_t size = FzeroTelemetryBuild(&s_state, ram, ram_size, input, packet);
  if (size)
    (void)sendto(s_socket, (const char *)packet, (int)size, 0,
                 (const sockaddr *)&s_destination, s_destination_size);
}

void FzeroTelemetryShutdown(void) {
  if (!s_no_delivery && s_socket != kInvalidSocket) {
    /* Dashboards can retain the last racing packet after this process exits.
     * Publish an explicit idle state before closing the UDP transport. */
    uint8_t idle_ram[0x0be2]{};
    uint8_t packet[324]{};
    if (FzeroTelemetryBuild(&s_state, idle_ram, sizeof(idle_ram), 0, packet))
      (void)sendto(s_socket, (const char *)packet, (int)sizeof(packet), 0,
                   (const sockaddr *)&s_destination, s_destination_size);
  }
  close_socket();
  std::memset(&s_state, 0, sizeof(s_state));
#ifdef _WIN32
  if (s_winsock_started) {
    WSACleanup();
    s_winsock_started = false;
  }
#endif
}
