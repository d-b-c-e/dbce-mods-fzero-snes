// Rig-profile controls -> F-Zero's raw-wheel keys (see fzero_controls.h). The [Controls] parser is the toolkit's
// shared one (src/vendor/controls/dbce_controls.hpp, toolkit e4502a6), so this file holds only the F-Zero mapping
// and the config.ini edits.
//
// Index mapping: the profile names DirectInput objects (axis 0 X, 2 Z, 5 Rz ...; button n = DIJOYSTATE button n).
// SDL opens a wheel like the R12 through its DirectInput driver, which numbers axes and buttons in the same object
// order, so the index carries over unchanged. The owner's own launcher capture of 2026-09-30 agrees with Wheelkit's
// DirectInput capture for every control both name (steering 0, accelerator 2, brake 5, buttons 31 and 18).
#include "fzero_controls.h"

#include "vendor/controls/dbce_controls.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace ctl = dbce::controls;

namespace {

// recomp-ui raw_hat_binding.h: raw bindings 0..127 are buttons; 128 + hat * 4 + direction (up, right, down, left).
enum { kMaxButton = 127, kHatBase = 128, kMaxHat = 15 };

struct Digital { const char *action, *key; };
// confirm/back follow the owner's own launcher mapping (wheel confirm -> SNES A, wheel back -> SNES B).
const Digital kDigital[] = {
    {"confirm", "ButtonA"}, {"back", "ButtonB"}, {"start", "ButtonStart"}, {"select", "ButtonSelect"},
    {"navUp", "ButtonUp"}, {"navDown", "ButtonDown"}, {"navLeft", "ButtonLeft"}, {"navRight", "ButtonRight"},
};

void note(FzeroControlsPlan *p, const std::string &action, const char *why)
{
    if (p->nnotes < FZERO_CONTROLS_MAX_NOTES)
        snprintf(p->notes[p->nnotes++], sizeof(p->notes[0]), "%s: %s", action.c_str(), why);
}

void setKey(FzeroControlsPlan *p, const char *name, int value)
{
    for (int i = 0; i < p->nkeys; ++i)
        if (!strcmp(p->keys[i].key, name)) { p->keys[i].value = value; return; }
    if (p->nkeys >= FZERO_CONTROLS_MAX_KEYS) return;
    snprintf(p->keys[p->nkeys].key, sizeof(p->keys[0].key), "%s", name);
    p->keys[p->nkeys++].value = value;
}

bool iequal(const std::string &a, const std::string &b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (ctl::detail::lower(a[i]) != ctl::detail::lower(b[i])) return false;
    return true;
}

// A digital binding as F-Zero's raw value, or -1 with a note when it has none.
int digital(const std::string &action, const ctl::Binding &b, FzeroControlsPlan *p)
{
    if (b.kind == ctl::Kind::Button) {
        if (b.index <= kMaxButton) return b.index;
        note(p, action, "button above 127 (F-Zero reads 0..127)");
        return -1;
    }
    if (b.kind == ctl::Kind::Hat) {
        int dir = b.angle == 0 ? 0 : b.angle == 9000 ? 1 : b.angle == 18000 ? 2 : b.angle == 27000 ? 3 : -1;
        if (dir >= 0 && b.index <= kMaxHat) return kHatBase + b.index * 4 + dir;
        note(p, action, "hat diagonal or hat above 15 (F-Zero reads the four directions of hats 0..15)");
        return -1;
    }
    note(p, action, "an axis is not a button here");
    return -1;
}

// A pedal reads "pressed" in F-Zero when the SDL value (after AcceleratorInvert/BrakeInvert negates it) rises above
// the threshold. The contract's normalized pedal rises with the raw value when travel is +1, and inverted flips it.
int pedalInvert(const ctl::Binding &b) { return (b.travel > 0) == b.inverted ? 1 : 0; }

uint32_t fnv1a(const std::string &s)
{
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}

std::string sectionName(const std::string &line)
{
    size_t a = line.find_first_not_of(" \t");
    if (a == std::string::npos || line[a] != '[') return std::string();
    size_t b = line.find(']', a);
    return b == std::string::npos ? std::string() : line.substr(a + 1, b - a - 1);
}

bool isSection(const std::string &line) { return !sectionName(line).empty(); }

// The key of an active "key = value" line, or empty.
std::string lineKey(const std::string &line)
{
    std::string t = ctl::trim(line);
    if (t.empty() || t[0] == ';' || t[0] == '#' || t[0] == '[') return std::string();
    size_t eq = t.find('=');
    return eq == std::string::npos ? std::string() : ctl::trim(t.substr(0, eq));
}

bool readLines(const char *path, std::vector<std::string> &lines, bool &crlf)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    crlf = text.find("\r\n") != std::string::npos;
    size_t start = 0;
    while (start < text.size()) {
        size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        std::string line = text.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
        start = end + 1;
    }
    return true;
}

bool writeLines(const char *path, const std::vector<std::string> &lines, bool crlf)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    for (const std::string &l : lines) out << l << (crlf ? "\r\n" : "\n");
    return (bool)out;
}

// Body lines of [name], or false when the section is absent.
bool readSection(const char *path, const char *name, std::vector<std::string> &body)
{
    std::vector<std::string> lines;
    bool crlf = false, in = false, found = false;
    if (!readLines(path, lines, crlf)) return false;
    for (const std::string &l : lines) {
        if (isSection(l)) { in = iequal(sectionName(l), name); found = found || in; continue; }
        if (in) body.push_back(l);
    }
    return found;
}

std::string readValue(const char *path, const char *section, const char *key)
{
    std::vector<std::string> body;
    if (!readSection(path, section, body)) return std::string();
    for (const std::string &l : body)
        if (iequal(lineKey(l), key)) return ctl::trim(l.substr(l.find('=') + 1));
    return std::string();
}

} // namespace

extern "C" void FzeroControlsPlanLines(const char *const *lines, int count, FzeroControlsPlan *p)
{
    memset(p, 0, sizeof(*p));
    std::vector<std::string> body;
    std::string joined;
    for (int i = 0; i < count; ++i) { body.push_back(lines[i] ? lines[i] : ""); joined += body.back() + "\n"; }
    ctl::Section s = ctl::parseSection(body);
    snprintf(p->profile, sizeof(p->profile), "%s", s.profile.c_str());
    if (!s.revision.empty()) snprintf(p->revision, sizeof(p->revision), "%s", s.revision.c_str());
    else snprintf(p->revision, sizeof(p->revision), "fnv1a:%08x", (unsigned)fnv1a(joined));
    if (!s.error.empty()) { snprintf(p->error, sizeof(p->error), "%s", s.error.c_str()); return; }
    for (auto &bad : s.invalid) note(p, bad.first, bad.second.c_str());

    const ctl::Binding *steer = ctl::find(s, "steer");
    if (!steer) { snprintf(p->error, sizeof(p->error), "no steer binding: F-Zero needs the wheel's steering axis"); return; }
    uint32_t id = ctl::productId(*steer);
    p->vendor = id & 0xFFFFu;
    p->product = id >> 16;
    snprintf(p->device, sizeof(p->device), "%s", steer->name.c_str());
    if (steer->inverted) note(p, "steer", "inverted steering has no F-Zero setting; steering axis left as it is");
    else setKey(p, "SteeringAxis", steer->index);

    for (const ctl::Entry &e : s.bound) {
        const ctl::Binding &b = e.binding;
        if (e.action == "steer") continue;
        bool sameDevice = b.dev == steer->dev;
        if (e.action == "throttle" || e.action == "brake") {
            if (!sameDevice) { note(p, e.action, "on another device (F-Zero reads one controller)"); continue; }
            bool gas = e.action == "throttle";
            setKey(p, gas ? "AcceleratorAxis" : "BrakeAxis", b.index);
            setKey(p, gas ? "AcceleratorInvert" : "BrakeInvert", pedalInvert(b));
            continue;
        }
        const Digital *map = nullptr;
        for (const Digital &d : kDigital) if (e.action == d.action) map = &d;
        if (!map) { note(p, e.action, "no F-Zero control"); continue; }
        if (!sameDevice) { note(p, e.action, "on another device (F-Zero reads one controller)"); continue; }
        int v = digital(e.action, b, p);
        if (v >= 0) setKey(p, map->key, v);
    }
    // Explicitly empty entries unbind what F-Zero has for them; steering stays (the wheel needs it).
    for (const std::string &a : s.unbound) {
        if (a == "throttle") { setKey(p, "AcceleratorAxis", -1); continue; }
        if (a == "brake") { setKey(p, "BrakeAxis", -1); continue; }
        bool known = false;
        for (const Digital &d : kDigital) if (a == d.action) { setKey(p, d.key, -1); known = true; }
        if (!known && a != "steer") note(p, a, "unbound; no F-Zero control");
    }
    p->ok = p->nkeys > 0;
    if (!p->ok) snprintf(p->error, sizeof(p->error), "no binding F-Zero can use");
}

extern "C" int FzeroControlsPlanFile(const char *config_path, FzeroControlsPlan *p)
{
    memset(p, 0, sizeof(*p));
    std::vector<std::string> body;
    if (!config_path || !readSection(config_path, "Controls", body)) return 0;
    std::vector<const char *> ptrs;
    for (const std::string &l : body) ptrs.push_back(l.c_str());
    FzeroControlsPlanLines(ptrs.data(), (int)ptrs.size(), p);
    return 1;
}

extern "C" int FzeroControlsPending(const char *config_path, const FzeroControlsPlan *p)
{
    return readValue(config_path, "ControlsApplied", "Revision") != p->revision;
}

extern "C" int FzeroControlsIniSet(const char *path, const char *section, const char *key, const char *value)
{
    std::vector<std::string> lines;
    bool crlf = false;
    readLines(path, lines, crlf); // a missing file starts empty
    std::string assign = std::string(key) + " = " + (value ? value : "");
    int start = -1, end = (int)lines.size();
    for (int i = 0; i < (int)lines.size(); ++i) {
        if (!isSection(lines[i])) continue;
        if (start >= 0) { end = i; break; }
        if (iequal(sectionName(lines[i]), section)) start = i + 1;
    }
    if (start < 0) {
        if (!lines.empty() && !ctl::trim(lines.back()).empty()) lines.push_back("");
        lines.push_back("[" + std::string(section) + "]");
        lines.push_back(assign);
        return writeLines(path, lines, crlf);
    }
    for (int i = start; i < end; ++i)
        if (iequal(lineKey(lines[i]), key)) { lines[i] = assign; return writeLines(path, lines, crlf); }
    int at = end;
    while (at > start && ctl::trim(lines[at - 1]).empty()) --at;
    lines.insert(lines.begin() + at, assign);
    return writeLines(path, lines, crlf);
}

extern "C" int FzeroControlsWrite(const char *config_path, const char *guid, const FzeroControlsPlan *p)
{
    if (!p->ok || !guid || !guid[0]) return 0;
    std::string backup = std::string(config_path) + ".before-profile-controls";
    if (!std::ifstream(backup, std::ios::binary)) {
        std::ifstream in(config_path, std::ios::binary);
        if (in) {
            std::ofstream out(backup, std::ios::binary);
            out << in.rdbuf();
            if (!out) return 0;
        }
    }
    std::string section = std::string("Controller.") + guid;
    char number[16];
    for (int i = 0; i < p->nkeys; ++i) {
        snprintf(number, sizeof(number), "%d", p->keys[i].value);
        if (!FzeroControlsIniSet(config_path, section.c_str(), p->keys[i].key, number)) return 0;
    }
    return FzeroControlsIniSet(config_path, "Controller", "GuidP1", guid) &&
           FzeroControlsIniSet(config_path, "ControlsApplied", "Revision", p->revision) &&
           FzeroControlsIniSet(config_path, "ControlsApplied", "Device", guid);
}
