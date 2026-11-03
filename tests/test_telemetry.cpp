#include "fzero_telemetry.h"
#include "forza_packet.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%d: %s\n", __LINE__, #x); std::exit(1); } } while (0)

static void put16(uint8_t *p, uint16_t value) {
  p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
}

/* A race clock or lap record: BCD minutes, seconds, hundredths. */
static void put_time(uint8_t *p, uint8_t minutes, uint8_t seconds, uint8_t hundredths) {
  p[0] = minutes; p[1] = seconds; p[2] = hundredths;
}

/* Dash fields: bestLap +52, lastLap +56, currentLap +60, currentRaceTime +64, lapNumber +68. */
static float dash_float(const uint8_t *packet, int offset) {
  float value = 0;
  std::memcpy(&value, packet + 244 + offset, sizeof(value));
  return value;
}

static unsigned lap_number(const uint8_t *packet) {
  return packet[244 + 68] | (unsigned)packet[244 + 69] << 8;
}

static bool close_to(float a, float b) { return std::fabs(a - b) < 0.001f; }

/* No lap is known: lapNumber, lastLap and bestLap stay zero and currentLap is the race clock. */
static bool laps_unknown(const uint8_t *packet) {
  return lap_number(packet) == 0 && dash_float(packet, 56) == 0.0f && dash_float(packet, 52) == 0.0f &&
         dash_float(packet, 60) == dash_float(packet, 64);
}

int main() {
  uint8_t ram[0x20000]{};
  uint8_t packet[324]{};
  FzeroTelemetryState state{};
  ram[0x54] = 2; ram[0x55] = 3;
  put16(ram + 0x0b70, 0x1ffe);
  put16(ram + 0x0b90, 100);
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
  CHECK(packet[323] == 'F');

  /* Cross the X wrap: 0x1ffe -> 2 is four units, not an 8192-unit jump. */
  put16(ram + 0x0b70, 2);
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0x0081, packet) == 324);
  float speed = 0;
  std::memcpy(&speed, packet + dbce::forza::OFF_SPEED_HORIZON, sizeof(speed));
  CHECK(speed > 0.0f && speed < 100.0f);
  CHECK(packet[244 + 71] == 255); /* accel */
  CHECK((int8_t)packet[dbce::forza::OFF_STEER_HORIZON] == 127);
  CHECK(packet[dbce::forza::OFF_GEAR_HORIZON] == 1);

  /* A save-state load or rewind moves the craft in one frame. That is a jump,
   * not speed: speed and distance hold, and the next real step counts again. */
  float distance = 0, after = 0;
  std::memcpy(&distance, packet + 244 + 48, sizeof(distance));
  put16(ram + 0x0b70, 0x0402);
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
  std::memcpy(&after, packet + dbce::forza::OFF_SPEED_HORIZON, sizeof(after));
  CHECK(after == speed);
  std::memcpy(&after, packet + 244 + 48, sizeof(after));
  CHECK(after == distance);
  put16(ram + 0x0b70, 0x0408);
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
  std::memcpy(&after, packet + 244 + 48, sizeof(after));
  CHECK(after == distance + 1.5f);
  std::memcpy(&speed, packet + dbce::forza::OFF_SPEED_HORIZON, sizeof(speed));
  CHECK(speed > 0.0f && speed < 100.0f);

  /* The game's rank counter is 1-based; no rank is inferred from invalid,
   * short or out-of-race snapshots, and no lap number is invented. */
  const unsigned ranks[] = {0, 1, 3, 7, 20, 21, 255};
  for (unsigned rank : ranks) {
    ram[0x0dc8] = (uint8_t)rank;
    CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
    CHECK(packet[244 + 70] == (rank >= 1 && rank <= 20 ? rank : 0));
    CHECK(packet[244 + 68] == 0 && packet[244 + 69] == 0);
  }
  ram[0x0dc8] = 3;
  CHECK(FzeroTelemetryBuild(&state, ram, 0x0dc8, 0, packet) == 324);
  CHECK(packet[244 + 70] == 0);
  CHECK(FzeroTelemetryBuild(&state, ram, 0x0be2, 0, packet) == 324);
  CHECK(packet[244 + 70] == 0);

  /* The race clock is BCD: 1'23"45 is 83.45 s, not the 1 * 60 + 0x23 +
   * 0x45 / 100 = 95.69 s a binary read gives. Before any timed lap, lap 1's
   * time is the race clock. */
  put_time(ram + 0xc0, 0x01, 0x23, 0x45);
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
  CHECK(close_to(dash_float(packet, 64), 83.45f) && laps_unknown(packet));

  /* Two timed laps at 0'25"10 and 0'52"30; the clock is at 1'03"00. The
   * player then reversed over the line ($0D40 one lower): still two laps. */
  std::memset(ram + 0x0e90, 0xff, 15);
  put_time(ram + 0x0e90, 0x00, 0x25, 0x10);
  put_time(ram + 0x0e93, 0x00, 0x52, 0x30);
  ram[0x0f53] = 2;
  ram[0x0d40] = 1;
  put_time(ram + 0xc0, 0x01, 0x03, 0x00);
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
  CHECK(lap_number(packet) == 2 && close_to(dash_float(packet, 56), 27.20f) &&
        close_to(dash_float(packet, 52), 25.10f) && close_to(dash_float(packet, 60), 10.70f) &&
        close_to(dash_float(packet, 64), 63.0f));

  /* The finish: lap 5 is timed at 2'11"50 and the clock stops there. */
  put_time(ram + 0x0e96, 0x01, 0x20, 0x00);
  put_time(ram + 0x0e99, 0x01, 0x45, 0x50);
  put_time(ram + 0x0e9c, 0x02, 0x11, 0x50);
  ram[0x0f53] = 5;
  put_time(ram + 0xc0, 0x02, 0x11, 0x50);
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
  CHECK(lap_number(packet) == 5 && close_to(dash_float(packet, 56), 26.0f) &&
        close_to(dash_float(packet, 52), 25.10f) && close_to(dash_float(packet, 60), 0.0f) &&
        close_to(dash_float(packet, 64), 131.5f));

  /* Nothing is guessed from a count past the finish, a record that is not a
   * BCD time, not recorded, earlier than the one before or later than the
   * clock, or a clock that is not BCD; nor from a short or menu snapshot. */
  ram[0x0f53] = 6;
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
  CHECK(laps_unknown(packet) && close_to(dash_float(packet, 64), 131.5f));
  ram[0x0f53] = 2;
  const uint8_t bad_records[][3] = {{0x00, 0x5a, 0x30}, {0xff, 0xff, 0xff}, {0x00, 0x20, 0x00}, {0x01, 0x05, 0x00}};
  for (const auto &record : bad_records) {
    put_time(ram + 0x0e93, record[0], record[1], record[2]);
    put_time(ram + 0xc0, 0x01, 0x03, 0x00);
    CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
    CHECK(laps_unknown(packet) && close_to(dash_float(packet, 64), 63.0f));
  }
  put_time(ram + 0x0e93, 0x00, 0x52, 0x30);
  put_time(ram + 0xc0, 0x01, 0x63, 0x00);
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
  CHECK(laps_unknown(packet) && dash_float(packet, 64) == 0.0f);
  put_time(ram + 0xc0, 0x01, 0x03, 0x00);
  CHECK(FzeroTelemetryBuild(&state, ram, 0x0f53, 0, packet) == 324);
  CHECK(laps_unknown(packet) && close_to(dash_float(packet, 64), 63.0f));
  ram[0x55] = 0;
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
  CHECK(laps_unknown(packet));
  ram[0x55] = 3;
  std::memset(ram + 0x0e90, 0xff, 15);
  ram[0x0f53] = 0;
  ram[0x0d40] = 0;
  put_time(ram + 0xc0, 0, 0, 0);

  ram[0x55] = 0;
  CHECK(FzeroTelemetryBuild(&state, ram, sizeof(ram), 0, packet) == 324);
  std::memcpy(&speed, packet + dbce::forza::OFF_SPEED_HORIZON, sizeof(speed));
  CHECK(speed == 0.0f && state.have_position == 0);
  CHECK(packet[244 + 70] == 0);
  CHECK(FzeroTelemetryBuild(&state, ram, 16, 0, packet) == 0);

#ifdef _WIN32
  /* Exercise the configured transport as well as the pure serializer. */
  WSADATA wsa{};
  CHECK(WSAStartup(MAKEWORD(2, 2), &wsa) == 0);
  SOCKET receiver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  CHECK(receiver != INVALID_SOCKET);
  sockaddr_in local{};
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  CHECK(bind(receiver, (sockaddr *)&local, sizeof(local)) == 0);
  int local_size = sizeof(local);
  CHECK(getsockname(receiver, (sockaddr *)&local, &local_size) == 0);
  DWORD timeout_ms = 1000;
  CHECK(setsockopt(receiver, SOL_SOCKET, SO_RCVTIMEO,
                   (const char *)&timeout_ms, sizeof(timeout_ms)) == 0);

  const char *config_path = "fzero_telemetry_test.ini";
  FILE *config = std::fopen(config_path, "wb");
  CHECK(config != nullptr);
  std::fprintf(config, "[Telemetry]\nEnabled=1\nHost=127.0.0.1\nPort=%u\n",
               (unsigned)ntohs(local.sin_port));
  std::fclose(config);
  FzeroTelemetryInit(config_path);
  FzeroTelemetryFrame(ram, sizeof(ram), 0x0001);
  uint8_t received[324]{};
  CHECK(recv(receiver, (char *)received, sizeof(received), 0) == 324);
  CHECK(received[323] == 'F');
  ram[0x55] = 3;
  FzeroTelemetryFrame(ram, sizeof(ram), 0x0001);
  CHECK(recv(receiver, (char *)received, sizeof(received), 0) == 324);
  int32_t race_on = 0;
  std::memcpy(&race_on, received, sizeof(race_on));
  CHECK(race_on == 1);
  FzeroTelemetryShutdown();
  CHECK(recv(receiver, (char *)received, sizeof(received), 0) == 324);
  std::memcpy(&race_on, received, sizeof(race_on));
  CHECK(race_on == 0 && received[244 + 71] == 0); /* idle, no throttle */

  /* Failed initialization owns no transport; it must release its Winsock
   * reference so the diagnostic delivery mute can still be latched. */
  config = std::fopen(config_path, "wb");
  CHECK(config != nullptr);
  std::fprintf(config, "[Telemetry]\nEnabled=1\nHost=::g\nPort=%u\n",
               (unsigned)ntohs(local.sin_port));
  std::fclose(config);
  FzeroTelemetryInit(config_path);
  CHECK(FzeroTelemetryLatchNoDelivery() == 1);
  CHECK(FzeroTelemetryDeliveryBlocked() == 1);

  config = std::fopen(config_path, "wb");
  CHECK(config != nullptr);
  std::fprintf(config, "[Telemetry]\nEnabled=1\nHost=127.0.0.1\nPort=%u\n",
               (unsigned)ntohs(local.sin_port));
  std::fclose(config);
  FzeroTelemetryInit(config_path);
  FzeroTelemetryFrame(ram, sizeof(ram), 0x0001);
  FzeroTelemetryShutdown();
  CHECK(FzeroTelemetryDeliveryBlocked() == 1);
  CHECK(recv(receiver, (char *)received, sizeof(received), 0) == SOCKET_ERROR);
  CHECK(WSAGetLastError() == WSAETIMEDOUT);
  closesocket(receiver);
  WSACleanup();
  std::remove(config_path);
#endif

  std::puts("F-Zero Forza telemetry packet tests passed");
  return 0;
}
