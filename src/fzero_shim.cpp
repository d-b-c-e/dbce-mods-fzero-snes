#include "fzero_shim.h"
#include "fzero_shim_map.h"
extern "C" {
#include "fzero_hotkeys.h"
#include "fzero_output_latch.h"
}
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {
bool requested, failed;
std::string config_path, profile, recording_path;
std::filesystem::path run_path;
FzeroShimMap map{};
dbce_shim_frame frame{};
void *handle;
bool pending;
std::chrono::steady_clock::time_point started;
using Clock = std::chrono::steady_clock;
/* A bounded diagnostic, not a newly enabled player backend. No test-command
 * worker is opened in this slice: capture only, physical wheel read only. */
constexpr double kSeconds = 240; // ABI2 recorder's default limit is 300 seconds.
constexpr double kHz = 61; // 60.0988 emulated frames/s, independent of rendering.
using Int = int32_t (*)();
using Name = const char *(*)(int32_t);
using Open = int32_t (*)(const dbce_shim_open_args *, void **, char *, int32_t);
using Tick = int32_t (*)(void *, dbce_shim_frame *);
using Record = int32_t (*)(void *, uint64_t, const double *, int32_t);
using Text = int32_t (*)(void *, char *, int32_t, int32_t *);
Open open2; Tick tick; Record record; Text stop;

bool refuse(const char *why) {
  failed = true;
  std::fprintf(stderr, "[fzero-shim] diagnostic refused/failed: %s\n", why);
  return false;
}
bool present(const char *s) { return s && *s; }
std::string trim(std::string s) {
  auto a = s.find_first_not_of(" \t\r\n");
  return a == std::string::npos ? "" : s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}
bool read_profile(const char *path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return refuse("config.ini missing");
  std::string all((std::istreambuf_iterator<char>(in)), {});
  if (all.size() > 1024 * 1024) return refuse("config.ini exceeds 1 MiB");
  size_t at = all.compare(0, 3, "\xef\xbb\xbf") == 0 ? 3 : 0;
  size_t begin = std::string::npos, end = all.size();
  for (; at < all.size();) {
    auto newline = all.find('\n', at);
    auto next = newline == std::string::npos ? all.size() : newline + 1;
    auto line = trim(all.substr(at, next-at));
    for (char &c : line) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (!line.empty() && line[0] == '[') {
      if (line == "[controls]") {
        if (begin != std::string::npos) return refuse("duplicate Controls section");
        begin = at;
      } else if (begin != std::string::npos && end == all.size()) end = at;
    }
    at = next;
  }
  if (begin == std::string::npos || end-begin > 65536) return refuse("missing/oversized Controls section");
  profile = all.substr(begin, end-begin); // exact bytes; Core owns binding validation.
  return true;
}
#ifdef _WIN32
template<class T> T entry(HMODULE module, const char *name) {
  return reinterpret_cast<T>(GetProcAddress(module, name));
}
bool load() {
  wchar_t exe[32768];
  DWORD n = GetModuleFileNameW(nullptr, exe, 32768);
  if (!n || n >= 32768) return refuse("executable path unavailable");
  auto dll = std::filesystem::path(exe).parent_path() / L"dbce_shim.dll";
  HMODULE module = LoadLibraryW(dll.c_str());
  if (!module) return refuse("dbce_shim.dll missing beside executable or failed to load");
  /* Deliberately resident until process exit, matching the shared host's
   * unresolved-stop contract. Never unload workers that may still exist. */
  auto abi = entry<Int>(module, "dbce_shim_abi_version");
  auto size = entry<Int>(module, "dbce_shim_frame_size");
  auto args = entry<Int>(module, "dbce_shim_open_args_size");
  auto name = entry<Name>(module, "dbce_shim_action_name");
  open2 = entry<Open>(module, "dbce_shim_open2");
  tick = entry<Tick>(module, "dbce_shim_tick");
  record = entry<Record>(module, "dbce_shim_record");
  stop = entry<Text>(module, "dbce_shim_stop");
  if (!abi || !size || !args || !name || !open2 || !tick || !record || !stop ||
      abi() != DBCE_SHIM_ABI || size() != sizeof(frame) || args() != sizeof(dbce_shim_open_args))
    return refuse("shim ABI/layout/export mismatch");
  for (int i = 0; i < DBCE_SHIM_ACTIONS; ++i) {
    const char *actual = name(i);
    if (!actual || std::strcmp(actual, kFzeroShimActions[i])) return refuse("shim action order mismatch");
  }
  return !name(DBCE_SHIM_ACTIONS) || refuse("shim has unknown extra actions");
}
#endif
}

int FzeroShimRequested(void) { return requested; }
const char *FzeroShimRecordingPath(void) { return recording_path.c_str(); }
int FzeroShimPrepare(const char *config, int direct_rom, int replay, int script, int explicit_recording) {
  const char *run = std::getenv("FZERO_SHIM_DIAGNOSTIC_RUN");
  if (!present(run)) return 1;
  requested = true;
  /* Must precede every refusal: a malformed diagnostic request cannot turn
   * into a normal force-enabled launch. */
  if (!FzeroLatchNoForce()) return refuse("force already initialized");
  if (!direct_rom || replay || script || explicit_recording)
    return refuse("requires an explicit ROM, no launcher and no other record/replay mode");
  for (const char *key : {"FZERO_STATE_SAVE_AT", "FZERO_STATE_LOAD_AT", "FZERO_VIEWPORT_SCRIPT", "FZERO_OVERLAY_SELFTEST"})
    if (present(std::getenv(key))) return refuse("state/viewport/overlay scripts cannot accompany shim capture");
#ifndef _WIN32
  return refuse("the shared input provider requires Windows");
#else
  try {
    run_path = std::filesystem::path(run);
    if (!run_path.is_absolute()) return refuse("run folder must be absolute");
    if (!read_profile(config)) return 0;
    if (!std::filesystem::create_directory(run_path)) return refuse("run folder already exists");
    config_path = config;
    recording_path = (run_path / "original.fzpt").string();
    std::ofstream out(run_path / "controls.ini", std::ios::binary);
    out.write(profile.data(), profile.size()); out.close();
    if (!out) return refuse("could not snapshot profile");
    std::fprintf(stderr, "[fzero-shim] DIAGNOSTIC ONLY: no physical force; original=%s; "
                 "independent SNES X/Y/L/R and save/rewind bindings are absent in ABI 2; normal controls stay unchanged\n",
                 recording_path.c_str());
    return 1;
  } catch (const std::exception &e) { return refuse(e.what()); }
#endif
}
int FzeroShimStart(void *window, const char *legacy_guid) {
  if (!requested) return 1;
  if (failed || !FzeroNoForceLatched()) return refuse("diagnostic not admitted or force latch missing");
#ifdef _WIN32
  try {
    if (!load()) return 0;
    auto folder = run_path.wstring(), capture = (run_path / "capture").wstring();
    std::string nonce = "fzero" + std::to_string(GetCurrentProcessId()) +
        std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    dbce_shim_open_args a{};
    a.struct_size = sizeof(a); a.flags = DBCE_SHIM_CAPTURE;
    a.folder = folder.c_str(); a.nonce = nonce.c_str(); a.profile = profile.data(); a.profile_len = (int32_t)profile.size();
    a.unsupported = kFzeroShimUnsupported; a.hwnd = window; a.capture_folder = capture.c_str();
    a.game_id = "fzero-snes"; a.game_channels = "simulationFrame\ninputWord";
    a.context = "diagnostic-original; no physical force; partial SNES action set";
    a.adapter_version = "fzero-shim-diagnostic-1"; a.hz = kHz; a.seconds = kSeconds;
    char why[512]{};
    if (open2(&a, &handle, why, sizeof(why)) != DBCE_SHIM_OK || !handle) return refuse(why);
    int range = 100, response = 50, threshold = 0;
    std::string section = "Controller." + std::string(legacy_guid ? legacy_guid : "");
    FzeroIniReadInt(config_path.c_str(), section.c_str(), "SteeringRangePercent", &range);
    FzeroIniReadInt(config_path.c_str(), section.c_str(), "SteeringResponsePercent", &response);
    FzeroIniReadInt(config_path.c_str(), section.c_str(), "PedalThreshold", &threshold);
    float normalized_threshold = (float)(threshold + 32768.0) / 65535.f;
    FzeroShimMapInit(&map, range, response, normalized_threshold);
    started = Clock::now();
    std::fprintf(stderr, "[fzero-shim] ABI 2 capture opened; native SDL controller lane disabled; "
                  "steering range=%d response=%d; Core calibration; keyboard guest input retained\n", range, response);
    return 1;
  } catch (const std::exception &e) { return refuse(e.what()); }
#else
  (void)window; (void)legacy_guid;
  return 0;
#endif
}
int FzeroShimInput(uint32_t *word) {
  if (word) *word = 0;
  if (!requested || failed || !handle || !word) return 0;
  if (pending) return refuse("previous simulation input was not consumed");
  if (std::chrono::duration<double>(Clock::now()-started).count() >= kSeconds)
    return refuse("240-second diagnostic deadline exceeded");
  frame = {}; frame.struct_size = sizeof(frame);
  if (tick(handle, &frame) != DBCE_SHIM_OK || !FzeroShimMapFrame(&map, &frame, word))
    return refuse("input tick/sequence/layout failed");
  pending = true;
  return 1;
}
int FzeroShimConsumed(uint64_t simulation_frame, uint32_t input) {
  if (!requested) return 1;
  if (!pending || failed || !handle) return refuse("no pending simulation input");
  double values[2]; FzeroShimConsumedValues(simulation_frame, input, values);
  pending = false;
  return record(handle, frame.seq, values, 2) == DBCE_SHIM_OK || refuse("consumed-input record failed");
}
void FzeroShimNeutral(void) { FzeroShimMapNeutral(&map); }
int FzeroShimStop(void) {
  if (handle) {
    char report[4096]{}; int32_t needed = 0;
    int r = stop(handle, report, sizeof(report), &needed);
    handle = nullptr;
    std::fprintf(stderr, "[fzero-shim] stop=%d %s\n", r, report);
    if (r != DBCE_SHIM_OK) refuse("shared provider/capture stop unresolved");
  }
  return !failed;
}
