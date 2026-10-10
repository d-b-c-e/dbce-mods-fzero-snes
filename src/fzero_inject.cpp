// Test injection for the raw wheel (see fzero_inject.h). The grammar, the table of running samples and their delivery
// are the toolkit's (src/vendor/controls/dbce_inject_table.hpp), shared with the DirectInput proxies; this file holds
// only the F-Zero side: the arming conditions, the command file and the SDL <-> DirectInput sample conversion.
#include "fzero_inject.h"

#include "fzero_controls.h"
#include "fzero_output_latch.h"
extern "C" {
#include "fzero_hotkeys.h"   // a C header without its own C++ guards
}
#include "vendor/controls/dbce_inject_table.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace ctl = dbce::controls;

namespace {

constexpr size_t kMaxFileBytes = 4096;
constexpr int kMaxFileCommands = 32;
constexpr int kMaxSessionCommands = 2000;
constexpr uint64_t kMaxSessionSeconds = 3600;

std::atomic<bool> g_armed;
std::string g_device;   // the wheel's SDL GUID text ([ControlsApplied] Device)
std::string g_wheel;    // the profile's wheel instance (its steering binding's dev)
ctl::Section g_section; // the applied [Controls], for "inject action"
ctl::InjectionTable g_table;
std::string g_nonce;    // inject.on's; inject.txt names it on its first line
uint64_t g_expiresMs;   // on nowMs()'s clock; 0 = no expiry (tests)
int g_sessionCommands;
bool g_testing;
uint64_t g_testClock, g_nextPoll;
std::string g_commands;
#ifdef _WIN32
FILETIME g_lastWrite;
#endif

uint64_t nowMs()
{
#ifdef _WIN32
    return g_testing ? g_testClock : (uint64_t)GetTickCount64();
#else
    return g_testClock;
#endif
}

// The session ends at its expiry: nothing more runs, though the process stays without force (the latch never clears).
bool sessionLive()
{
    if (!g_armed) return false;
    if (g_expiresMs && nowMs() >= g_expiresMs) {
        g_armed = false;
        const int n = g_table.clearFor("");
        std::fprintf(stderr, "[fzero-inject] test session expired; %d running sample(s) dropped; injection off\n", n);
        return false;
    }
    return true;
}

// inject.on: "nonce=<8-64 letters/digits>" and "expires=<unix seconds>" in (now, now + 1 h].
bool parseSession(const std::string &text, uint64_t nowUnix, std::string &nonce, uint64_t &expiresUnix, std::string &why)
{
    nonce.clear(); expiresUnix = 0;
    if (text.size() > 512) { why = "inject.on is larger than 512 bytes"; return false; }
    std::istringstream in(text);
    std::string line;
    bool haveExpiry = false;
    while (std::getline(in, line)) {
        const std::string t = ctl::trim(line);
        if (t.rfind("nonce=", 0) == 0) nonce = t.substr(6);
        else if (t.rfind("expires=", 0) == 0) {
            const std::string v = t.substr(8);
            if (v.empty() || v.size() > 12 || v.find_first_not_of("0123456789") != std::string::npos) { why = "expires= is not unix seconds"; return false; }
            expiresUnix = std::strtoull(v.c_str(), nullptr, 10);
            haveExpiry = true;
        }
    }
    if (nonce.size() < 8 || nonce.size() > 64) { why = "inject.on names no nonce= of 8-64 letters/digits"; return false; }
    for (char c : nonce)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) { why = "the nonce is not letters/digits"; return false; }
    if (!haveExpiry) { why = "inject.on names no expires="; return false; }
    if (expiresUnix <= nowUnix) { why = "the test session in inject.on has expired"; return false; }
    if (expiresUnix - nowUnix > kMaxSessionSeconds) { why = "the test session in inject.on ends more than an hour from now"; return false; }
    return true;
}

std::string dbcePath(const char *file)
{
    const char *base = std::getenv("LOCALAPPDATA");
    return base ? std::string(base) + "\\dbce\\fzero\\" + file : std::string();
}

bool sameName(const std::string &a, const char *b)
{
    if (a.size() != std::strlen(b)) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (ctl::detail::lower(a[i]) != ctl::detail::lower(b[i])) return false;
    return true;
}

// The [Controls] body lines, as fzero_controls.cpp reads them (the section name's letter case ignored).
std::vector<std::string> controlsLines(const char *path)
{
    std::vector<std::string> body;
    std::ifstream in(path, std::ios::binary);
    std::string line;
    bool inside = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::string t = ctl::trim(line);
        if (!t.empty() && t[0] == '[') {
            const size_t end = t.find(']');
            inside = end != std::string::npos && sameName(t.substr(1, end - 1), "Controls");
            continue;
        }
        if (inside) body.push_back(line);
    }
    return body;
}

const char *armWith(const std::string &device, const std::vector<std::string> &lines)
{
    g_section = ctl::parseSection(lines);
    if (!g_section.error.empty()) return "the [Controls] profile does not parse";
    const ctl::Binding *steer = ctl::find(g_section, "steer");
    if (!steer) return "the [Controls] profile has no steering binding (F-Zero's one controller)";
    if (device.empty()) return "[ControlsApplied] names no device";
    g_table.clearFor("");   // a new arming starts with nothing running
    g_wheel = steer->dev;
    g_device = device;
    g_sessionCommands = 0;
    g_armed = true;
    return nullptr;
}

int addLine(const std::string &line, std::string &why)
{
    if (!sessionLive()) { why = "no test session"; return 0; }
    if (g_sessionCommands >= kMaxSessionCommands) { why = "the session's 2000 commands are used"; return 0; }
    ctl::InjectCommand c;
    if (!ctl::parseInject(line, &g_section, c, why)) return 0;
    if (c.binding.dev != g_wheel) { why = "dev is not the profile's wheel (F-Zero reads one controller)"; return 0; }
    why = g_table.add(c, nowMs());
    if (!why.empty()) return 0;
    ++g_sessionCommands;
    return 1;
}

// One inject.txt as read: "nonce=<the session's>" first, then at most 32 commands. Logs each; returns those accepted.
int takeFile(const std::string &text, std::string &why)
{
    why.clear();
    if (text.size() > kMaxFileBytes) { why = "inject.txt is larger than 4096 bytes; nothing read"; return 0; }
    std::istringstream in(text);
    std::string line;
    bool first = true;
    int commands = 0, accepted = 0;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::string t = ctl::trim(line);
        if (t.empty()) continue;
        if (first) {
            first = false;
            if (t != "nonce=" + g_nonce) { why = "inject.txt does not name this session's nonce; nothing read"; return 0; }
            continue;
        }
        if (++commands > kMaxFileCommands) { why = "inject.txt has more than 32 commands; the rest were not read"; break; }
        std::string w;
        if (addLine(t, w)) { ++accepted; std::fprintf(stderr, "[fzero-inject] accepted: %s\n", t.c_str()); }
        else std::fprintf(stderr, "[fzero-inject] refused \"%s\": %s\n", t.c_str(), w.c_str());
    }
    if (first) why = "inject.txt is empty";
    return accepted;
}

unsigned long hatAngle(uint8_t bits)   // SDL hat bits (up 1, right 2, down 4, left 8) -> DirectInput POV
{
    switch (bits) {
    case 1: return 0; case 3: return 4500; case 2: return 9000; case 6: return 13500;
    case 4: return 18000; case 12: return 22500; case 8: return 27000; case 9: return 31500;
    default: return 0xFFFFFFFFul;
    }
}

uint8_t hatBits(unsigned long angle)
{
    if ((angle & 0xFFFF) == 0xFFFF || angle >= 36000) return 0;
    static const uint8_t bits[8] = {1, 3, 2, 6, 4, 12, 8, 9};
    return bits[((angle + 2250) / 4500) % 8];
}

} // namespace

extern "C" void FzeroInjectInit(const char *config_path)
{
    g_armed = false;
#ifdef _WIN32
    const std::string on = dbcePath("inject.on");
    if (on.empty() || GetFileAttributesA(on.c_str()) == INVALID_FILE_ATTRIBUTES) return;   // not requested
    auto refuse = [](const char *why) { std::fprintf(stderr, "[fzero-inject] test injection refused: %s\n", why); };
    // The request alone latches "no force" for this process, before anything below can refuse it.
    if (!FzeroLatchNoForce()) return refuse("force output already started in this process");
    std::fprintf(stderr, "[fzero-inject] test injection requested (inject.on): no force output in this process\n");
    std::string sessionText;
    {
        std::ifstream f(on, std::ios::binary);
        char buf[513];
        f.read(buf, sizeof buf);
        sessionText.assign(buf, (size_t)f.gcount());
    }
    std::string nonce, why;
    uint64_t expiresUnix = 0;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    const uint64_t nowUnix = ((((uint64_t)ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10000000ull) - 11644473600ull;
    if (!parseSession(sessionText, nowUnix, nonce, expiresUnix, why)) return refuse(why.c_str());
    int ffb = 1;
    FzeroIniReadInt(config_path, "ForceFeedback", "Enabled", &ffb);
    if (ffb != 0) return refuse("config.ini [ForceFeedback] Enabled is not 0 (injection runs only when no force runs)");
    FzeroControlsPlan plan;
    if (!FzeroControlsPlanFile(config_path, &plan) || !plan.ok) return refuse("no applicable [Controls] profile in config.ini");
    if (FzeroControlsPending(config_path, &plan)) return refuse("the [Controls] profile is not applied yet ([ControlsApplied] Revision differs)");
    char device[64] = "";
    FzeroIniReadString(config_path, "ControlsApplied", "Device", device, sizeof device);
    if (const char *w = armWith(device, controlsLines(config_path))) return refuse(w);
    g_nonce = nonce;
    g_expiresMs = nowMs() + (expiresUnix - nowUnix) * 1000ull;
    g_commands = dbcePath("inject.txt");
    // Only commands written after this start count: the file's time as it is now is the baseline.
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (GetFileAttributesExA(g_commands.c_str(), GetFileExInfoStandard, &a)) g_lastWrite = a.ftLastWriteTime;
    else g_lastWrite = FILETIME{};
    std::fprintf(stderr, "[fzero-inject] test injection ARMED (inject.on, no force): profile '%s' revision %s on %s; "
                 "session ends in %llu s; commands from %s\n", plan.profile, plan.revision, g_device.c_str(),
                 (unsigned long long)(expiresUnix - nowUnix), g_commands.c_str());
#else
    (void)config_path;
#endif
}

extern "C" int FzeroInjectArmed(void) { return g_armed ? 1 : 0; }

extern "C" void FzeroInjectPoll(void)
{
#ifdef _WIN32
    if (!sessionLive() || g_testing || g_commands.empty()) return;
    const uint64_t now = nowMs();
    if (now < g_nextPoll) return;
    g_nextPoll = now + 100;
    WIN32_FILE_ATTRIBUTE_DATA a;
    if (!GetFileAttributesExA(g_commands.c_str(), GetFileExInfoStandard, &a)) return;
    if (CompareFileTime(&a.ftLastWriteTime, &g_lastWrite) <= 0) return;
    g_lastWrite = a.ftLastWriteTime;   // each version is read once, whatever its fate
    std::string text;
    if (a.nFileSizeHigh || a.nFileSizeLow > kMaxFileBytes) text.assign(kMaxFileBytes + 1, ' ');   // refused unread
    else {
        std::ifstream in(g_commands, std::ios::binary);
        char buf[kMaxFileBytes + 1];
        in.read(buf, sizeof buf);
        text.assign(buf, (size_t)in.gcount());
    }
    std::string why;
    takeFile(text, why);
    if (!why.empty()) std::fprintf(stderr, "[fzero-inject] %s\n", why.c_str());
#endif
}

extern "C" void FzeroInjectDeviceClosed(void)
{
    const int n = g_table.clearFor("");
    if (n) std::fprintf(stderr, "[fzero-inject] the wheel closed; %d running sample(s) dropped\n", n);
}

extern "C" int FzeroInjectRawRead(const char *sdl_guid, int16_t *axes, int axis_count, unsigned char *buttons,
                                  int button_count, uint8_t *hats, int hat_count)
{
    if (!sessionLive() || !g_table.any() || !sdl_guid || g_device != sdl_guid) return 0;
    if (axis_count < 0 || axis_count > 8 || button_count < 0 || button_count > 128 || hat_count < 0 || hat_count > 4) return 0;
    // The read in DirectInput terms (axes 0..65535, buttons 0x80, POV angles), as the bindings and the grammar name them.
    long ax[8];
    unsigned char bt[128] = {0};
    unsigned long pv[4];
    for (int i = 0; i < 8; ++i) ax[i] = i < axis_count && axes ? (long)axes[i] + 32768 : 32768;
    for (int i = 0; i < button_count; ++i) bt[i] = buttons && buttons[i] ? 0x80 : 0;
    for (int i = 0; i < 4; ++i) pv[i] = i < hat_count && hats ? hatAngle(hats[i]) : 0xFFFFFFFFul;
    ctl::StateView v;
    v.axes = axis_count > 0 && axes ? ax : nullptr;
    v.buttons = button_count > 0 && buttons ? bt : nullptr;
    v.buttonCount = button_count;
    v.povs = hat_count > 0 && hats ? pv : nullptr;
    v.povCount = hat_count;
    std::vector<std::string> lines;
    const int n = g_table.apply(g_wheel, v, nowMs(), [axis_count](int axis, long &mn, long &mx) {
        if (axis >= axis_count) return false;   // the stick has no such axis: never delivered
        mn = 0; mx = 65535;
        return true;
    }, lines);
    for (auto &l : lines) std::fprintf(stderr, "[fzero-inject] %s\n", l.c_str());
    for (int i = 0; i < axis_count && axes; ++i) {
        const long s = ax[i] - 32768;
        axes[i] = (int16_t)(s < -32768 ? -32768 : s > 32767 ? 32767 : s);
    }
    for (int i = 0; i < button_count && buttons; ++i) buttons[i] = bt[i] ? 1 : 0;
    for (int i = 0; i < hat_count && hats; ++i) hats[i] = hatBits(pv[i]);
    return n;
}

extern "C" int FzeroInjectTestArm(const char *device, const char *const *controls_lines, int count)
{
    g_testing = true;
    g_armed = false;
    std::vector<std::string> lines;
    for (int i = 0; i < count; ++i) lines.push_back(controls_lines[i] ? controls_lines[i] : "");
    g_nonce = "test";
    g_expiresMs = 0;
    return armWith(device ? device : "", lines) == nullptr ? 1 : 0;
}

extern "C" int FzeroInjectTestCommand(const char *line, char *why, int why_size)
{
    std::string w;
    const int ok = line ? addLine(line, w) : 0;
    if (why && why_size > 0) std::snprintf(why, (size_t)why_size, "%s", w.c_str());
    return ok;
}

extern "C" int FzeroInjectTestFile(const char *text, size_t size, char *why, int why_size)
{
    std::string w;
    const int n = takeFile(std::string(text ? text : "", text ? size : 0), w);
    if (why && why_size > 0) std::snprintf(why, (size_t)why_size, "%s", w.c_str());
    return n;
}

extern "C" void FzeroInjectTestExpire(uint64_t expires_ms) { g_expiresMs = expires_ms; }

extern "C" int FzeroInjectTestSession(const char *text, uint64_t now_unix, char *why, int why_size)
{
    std::string nonce, w;
    uint64_t expires = 0;
    const bool ok = parseSession(text ? text : "", now_unix, nonce, expires, w);
    if (why && why_size > 0) std::snprintf(why, (size_t)why_size, "%s", ok ? nonce.c_str() : w.c_str());
    return ok ? 1 : 0;
}

extern "C" void FzeroInjectTestClock(uint64_t now_ms) { g_testClock = now_ms; }
