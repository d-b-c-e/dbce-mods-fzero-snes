#include "fzero_shim.h"
#include "fzero_shim_map.h"
#include "fzero_ffb.h"
#include "fzero_telemetry.h"
extern "C" {
#include "fzero_hotkeys.h"
#include "fzero_output_latch.h"
}
#include <chrono>
#include <algorithm>
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
bool requested, failed, latched, completed, host_persistence, host_debug;
std::string config_path, profile, recording_path, save_root, session_nonce;
std::filesystem::path session_file, batch_file;
FzeroFfbState force_state{};
FzeroTelemetryState telemetry_state{};
int strength = 40, steering_strength = 40, impact_strength = 20;
bool sine_impact;
std::filesystem::path run_path;
FzeroShimMap map{};
dbce_shim_frame frame{};
void *handle;
bool pending;
std::chrono::steady_clock::time_point started;
using Clock = std::chrono::steady_clock;
/* A bounded diagnostic, not a newly enabled player backend. */
constexpr double kSeconds = 240; // ABI2 recorder's default limit is 300 seconds.
constexpr double kHz = 61; // 60.0988 emulated frames/s, independent of rendering.
constexpr const char *kChannels = "simulationFrame\ninputWord\nstate54\nstate55\nstate56\n"
  "ffb.constantFallback\nffb.spring\nffb.damper\nffb.road\nffb.roadFrequencyMilliHz\n"
  "ffb.collisionPulse\nffb.racing\ntelemetry.speedMps\ntelemetry.distanceM\nffb.impactMagnitude\n"
  "ffb.impactDurationMs\nffb.impactFrequencyMilliHz\n"
  "menu.selection\nmenu.training\nmenu.transition\nmenu.car\nmenu.course\nmenu.league\n"
  "menu.deluxeCar\nmenu.deluxeRival\nmenu.deluxePanel\nmenu.deluxeAccept\nmenu.deluxeTrack\nmenu.state5b";
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
int FzeroShimLatched(void) { return latched; }
int FzeroShimCompleted(void) { return completed && !failed; }
const char *FzeroShimSaveRoot(void) { return save_root.c_str(); }
void FzeroShimHostFences(const char *selected_save_root, int debug_disabled) {
  std::string actual = selected_save_root ? selected_save_root : "";
  host_persistence = !save_root.empty() && (actual == save_root || actual == save_root + "/bs-deluxe" ||
      actual == save_root + "/msu1" || actual == save_root + "/bs-deluxe/msu1");
  host_debug = debug_disabled != 0;
}
const char *FzeroShimRecordingPath(void) { return recording_path.c_str(); }
int FzeroShimPrepare(const char *config, int direct_rom, int replay, int script, int explicit_recording) {
  const char *run = std::getenv("FZERO_SHIM_DIAGNOSTIC_RUN");
  if (!present(run)) return 1;
  requested = true;
  /* Must precede every refusal: a malformed diagnostic request cannot turn
   * into a normal force-enabled launch. */
  if (!FzeroLatchNoForce()) return refuse("force already initialized");
  if (!FzeroTelemetryLatchNoDelivery()) return refuse("telemetry delivery already initialized");
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
    // Existing engine Deluxe save-root buffers are 96 bytes including suffix.
    save_root = (run_path / "saves").string();
    if (save_root.size() > 68) return refuse("diagnostic save root exceeds 68 bytes; use a short private path");
    const char *nonce = std::getenv("FZERO_SHIM_SESSION_NONCE");
    const char *session = std::getenv("FZERO_SHIM_SESSION_FILE");
    const char *batch = std::getenv("FZERO_SHIM_BATCH_FILE");
    latched = present(nonce) || present(session) || present(batch);
    if (latched) {
      if (!present(nonce) || !present(session) || !present(batch))
        return refuse("latched mode requires all three session/nonce/batch variables");
      session_nonce = nonce; session_file = session; batch_file = batch;
      if (session_nonce.size() < 8 || session_nonce.size() > 64 ||
          session_nonce.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != std::string::npos ||
          !session_file.is_absolute() || !batch_file.is_absolute() ||
          !std::filesystem::is_regular_file(session_file) || !std::filesystem::is_regular_file(batch_file) ||
          std::filesystem::equivalent(session_file, batch_file))
        return refuse("invalid nonce or session/batch paths");
      // Shared control worker parses the exact session/expiry and requires this
      // nonce. It alone admits batches: this host never invents action values.
    }
    if (!read_profile(config)) return 0;
    if (!std::filesystem::create_directory(run_path)) return refuse("run folder already exists");
    if (!std::filesystem::create_directory(save_root)) return refuse("could not isolate save root");
    config_path = config;
    recording_path = (run_path / "original.fzpt").string();
    std::ofstream out(run_path / "controls.ini", std::ios::binary);
    out.write(profile.data(), profile.size()); out.close();
    if (!out) return refuse("could not snapshot profile");
    std::filesystem::copy_file(config_path, run_path / "config.ini");
    config_path = (run_path / "config.ini").string(); // force/PDM tuning provenance is frozen for this run.
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
  const bool fences = host_persistence && host_debug && FzeroTelemetryDeliveryBlocked();
  if (!fences) return refuse("host persistence/debug/telemetry fences not established");
#ifdef _WIN32
  try {
    if (!load()) return 0;
    auto folder = run_path.wstring(), capture = (run_path / "capture").wstring();
    std::string nonce = "fzero" + std::to_string(GetCurrentProcessId()) +
        std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    auto session = session_file.wstring(), batch = batch_file.wstring();
    dbce_shim_open_args a{};
    a.struct_size = sizeof(a); a.flags = DBCE_SHIM_CAPTURE | (latched ? DBCE_SHIM_LATCHED : 0);
    if (latched) {
      a.fences_verified = fences; a.force_absent = FzeroNoForceLatched();
      a.session_file = session.c_str(); a.batch_file = batch.c_str();
    }
    a.folder = folder.c_str(); a.nonce = latched ? session_nonce.c_str() : nonce.c_str(); a.profile = profile.data(); a.profile_len = (int32_t)profile.size();
    a.unsupported = kFzeroShimUnsupported; a.hwnd = window; a.capture_folder = capture.c_str();
    a.game_id = "fzero-snes"; a.game_channels = kChannels;
    a.context = latched ? "diagnostic-injected-original; keyboard suppressed; software signals only; partial SNES actions" :
                         "diagnostic-owner-original; keyboard retained; software signals only; partial SNES actions";
      a.adapter_version = "fzero-shim-diagnostic-3"; a.hz = kHz; a.seconds = kSeconds;
    char why[512]{};
    if (open2(&a, &handle, why, sizeof(why)) != DBCE_SHIM_OK || !handle) return refuse(why);
    int range = 100, response = 50, threshold = 0;
    std::string section = "Controller." + std::string(legacy_guid ? legacy_guid : "");
    FzeroIniReadInt(config_path.c_str(), section.c_str(), "SteeringRangePercent", &range);
    FzeroIniReadInt(config_path.c_str(), section.c_str(), "SteeringResponsePercent", &response);
    FzeroIniReadInt(config_path.c_str(), section.c_str(), "PedalThreshold", &threshold);
    float normalized_threshold = (float)(threshold + 32768.0) / 65535.f;
    FzeroShimMapInit(&map, range, response, normalized_threshold);
    FzeroIniReadInt(config_path.c_str(), "ForceFeedback", "Strength", &strength);
    strength = std::clamp(strength, 0, 100); steering_strength = strength;
    FzeroIniReadInt(config_path.c_str(), "ForceFeedback", "SteeringStrength", &steering_strength);
    FzeroIniReadInt(config_path.c_str(), "ForceFeedback", "ImpactStrength", &impact_strength);
    steering_strength = std::clamp(steering_strength, 0, 100); impact_strength = std::clamp(impact_strength, 0, 100);
    char impact_type[32] = "Constant";
    FzeroIniReadString(config_path.c_str(), "ForceFeedback", "ImpactType", impact_type, sizeof(impact_type));
    sine_impact = std::strcmp(impact_type, "Sine") == 0;
    std::ofstream origin(run_path / "origin.txt", std::ios::binary);
    origin << "origin=" << (latched ? "diagnostic-injected" : "diagnostic-owner") << "\nforce=software-only\ntelemetry=software-only\n"
      << "strength=" << strength << "\nsteeringStrength=" << steering_strength << "\nimpactStrength=" << impact_strength
      << "\nimpactType=" << (sine_impact ? "Sine" : "Constant") << "\n";
    origin.close(); if (!origin) return refuse("origin receipt write failed");
    started = Clock::now();
    std::fprintf(stderr, "[fzero-shim] ABI 2 capture opened; native SDL controller lane disabled; "
                  "steering range=%d response=%d; Core calibration; keyboard=%s\n", range, response, latched ? "suppressed" : "retained");
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
  if (latched) {
    if (!frame.latched || !frame.suppress_native || frame.run < 1 || frame.run > 4)
      return refuse("latched session refused, expired or failed");
    for (const auto &a : frame.action)
      if (a.status == 12 && !a.injected) return refuse("latched action lacks injected provenance");
    completed = frame.run == 4; // consume and record the terminal neutral frame.
  } else if (frame.latched || frame.suppress_native || frame.run != 0)
    return refuse("capture-only provider unexpectedly owns keyboard lane");
  pending = true;
  return 1;
}
int FzeroShimConsumed(uint64_t simulation_frame, uint32_t input, const uint8_t *ram, size_t ram_size) {
  if (!requested) return 1;
  if (!pending || failed || !handle) return refuse("no pending simulation input");
  if (!ram || ram_size < 0x0be2) return refuse("post-frame WRAM unavailable");
  FzeroFfbOutput f{}; uint8_t packet[324];
  FzeroFfbComputeSteering(&force_state, ram, ram_size, input, strength, steering_strength, &f);
  if (FzeroTelemetryBuild(&telemetry_state, ram, ram_size, input, packet) != sizeof(packet))
    return refuse("software telemetry observation failed");
  double values[17+FZERO_SHIM_MENU_VALUES]; FzeroShimConsumedValues(simulation_frame, input, values);
  if (!FzeroShimMenuValues(ram, ram_size, values+17))
    return refuse("post-frame menu WRAM unavailable");
  values[2]=ram[0x54]; values[3]=ram[0x55]; values[4]=ram[0x56];
  values[5]=f.constant_force; values[6]=f.spring_coefficient; values[7]=f.damper_coefficient;
  values[8]=f.road_magnitude; values[9]=f.road_frequency_millihz; values[10]=f.collision_pulse;
  values[11]=f.racing; values[12]=telemetry_state.speed_mps; values[13]=telemetry_state.total_distance_m;
  values[14]=f.collision_pulse ? impact_strength*100 : 0;
  values[15]=sine_impact ? 140 : 120; values[16]=sine_impact ? 32000 : 0;
  pending = false;
  return record(handle, frame.seq, values, 17+FZERO_SHIM_MENU_VALUES) == DBCE_SHIM_OK || refuse("consumed-input record failed");
}
void FzeroShimNeutral(void) { FzeroShimMapNeutral(&map); }
int FzeroShimStop(void) {
  if (latched && !completed) refuse("latched session closed without an explicit completed end batch");
  if (handle) {
    char report[4096]{}; int32_t needed = 0;
    int r = stop(handle, report, sizeof(report), &needed);
    handle = nullptr;
    std::fprintf(stderr, "[fzero-shim] stop=%d %s\n", r, report);
    if (r != DBCE_SHIM_OK) refuse("shared provider/capture stop unresolved");
  }
  return !failed;
}
