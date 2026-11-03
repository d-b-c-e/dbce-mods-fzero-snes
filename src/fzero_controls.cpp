// Rig-profile controls -> F-Zero's raw-wheel keys (see fzero_controls.h). The [Controls] parser is the toolkit's
// shared one (src/vendor/controls/dbce_controls.hpp, toolkit ee8014d), so this file holds only the F-Zero mapping
// and the config.ini edits.
//
// Index mapping: the profile names DirectInput objects (axis 0 X, 2 Z, 5 Rz ...; button n = DIJOYSTATE button n);
// F-Zero's keys are SDL joystick indexes. They are carried over unchanged only for devices where the two numberings
// were seen to agree (kQualified). Any other device is refused rather than guessed: SDL's numbering depends on its
// driver and on which objects a device reports.
#include "fzero_controls.h"

#include "vendor/controls/dbce_controls.hpp"

#include <cstdint>
#include <cstdio>
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

const char *const kAxisProfileKeys[] = {"SteeringProfileBinding", "AcceleratorProfileBinding", "BrakeProfileBinding"};
const char *const kAxisActions[] = {"steer", "throttle", "brake"};
struct NativeAxisProfile { ctl::Binding binding; bool present = false, valid = false; };
NativeAxisProfile g_native_axes[3];

bool axisProfile(FzeroControlsPlan *p, int slot, const ctl::Binding &binding, int schema)
{
    p->axis_profiles |= 1u << slot;
    if (schema == 1) return true; // actively clears a preceding Schema-2 request for this axis
    // The qualified SDL wheel exposes the same 16-bit positions as DI's 0..65535 range.
    // Other ranges have not been compared; never guess their raw conversion.
    if (binding.min != 0 || binding.max != 65535) {
        snprintf(p->error, sizeof(p->error), "%s: Native Schema 2 requires the qualified 0..65535 range", kAxisActions[slot]);
        return false;
    }
    ctl::Binding stored = binding;
    stored.name.clear(); // display names do not participate in normalization
    const std::string text = ctl::format(stored);
    if (text.size() >= sizeof(p->axis_profile[slot])) {
        snprintf(p->error, sizeof(p->error), "%s: axis profile too long", kAxisActions[slot]);
        return false;
    }
    std::memcpy(p->axis_profile[slot], text.c_str(), text.size() + 1);
    return true;
}

// recomp-ui raw_hat_binding.h: raw bindings 0..127 are buttons; 128 + hat * 4 + direction (up, right, down, left).
enum { kMaxButton = 127, kHatBase = 128, kMaxHat = 15 };

// Devices whose SDL indexes were observed to equal their DirectInput indexes.
struct Qualified { unsigned vendor, product; };
const Qualified kQualified[] = {
    // MOZA R12 Base: the owner's launcher capture (SDL, 2026-09-30) and Wheelkit's DirectInput capture agree on
    // every control both name (steering 0, accelerator 2, brake 5, buttons 31 and 18); hat 0 maps to 128-131.
    {0x346E, 0x0006},
};

struct Digital { const char *action, *key; };
// confirm/back follow the owner's own launcher mapping (wheel confirm -> SNES A, wheel back -> SNES B).
const Digital kDigital[] = {
    {"confirm", "ButtonA"}, {"back", "ButtonB"}, {"start", "ButtonStart"}, {"select", "ButtonSelect"},
    {"navUp", "ButtonUp"}, {"navDown", "ButtonDown"}, {"navLeft", "ButtonLeft"}, {"navRight", "ButtonRight"},
    {"nativeX", "ButtonX"}, {"nativeY", "ButtonY"}, {"nativeL", "ButtonL"}, {"nativeR", "ButtonR"},
    {"saveStateMenu", "ButtonSaveStateMenu"}, {"rewind", "ButtonRewind"},
};

bool nativeDigital(const std::string &action)
{
    if (ctl::shapeOf(action) != ctl::Shape::Unknown) return false;
    for (const Digital &d : kDigital) if (action == d.action) return true;
    return false;
}

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

// An INI file held in memory: every edit lands here, and the file is replaced once, atomically.
struct Doc {
    std::vector<std::string> lines;
    bool crlf = false;
};

std::string render(const Doc &d)
{
    std::string out;
    for (const std::string &l : d.lines) out += l + (d.crlf ? "\r\n" : "\n");
    return out;
}

// "key = value" inside [section]; every other line is kept; the section or key is added when absent.
void setInDoc(Doc &d, const std::string &section, const std::string &key, const std::string &value)
{
    std::vector<std::string> &lines = d.lines;
    std::string assign = key + " = " + value;
    int start = -1, end = (int)lines.size();
    for (int i = 0; i < (int)lines.size(); ++i) {
        if (!isSection(lines[i])) continue;
        if (start >= 0) { end = i; break; }
        if (iequal(sectionName(lines[i]), section)) start = i + 1;
    }
    if (start < 0) {
        if (!lines.empty() && !ctl::trim(lines.back()).empty()) lines.push_back("");
        lines.push_back("[" + section + "]");
        lines.push_back(assign);
        return;
    }
    for (int i = start; i < end; ++i)
        if (iequal(lineKey(lines[i]), key)) { lines[i] = assign; return; }
    int at = end;
    while (at > start && ctl::trim(lines[at - 1]).empty()) --at;
    lines.insert(lines.begin() + at, assign);
}

std::string docValue(const Doc &d, const std::string &section, const std::string &key)
{
    bool in = false;
    for (const std::string &l : d.lines) {
        if (isSection(l)) { in = iequal(sectionName(l), section); continue; }
        if (in && iequal(lineKey(l), key)) return ctl::trim(l.substr(l.find('=') + 1));
    }
    return std::string();
}

int g_fault; // FzeroControlsTestFault: 1 fails writing the temporary file, 2 fails the replace

// Writes text beside path, then moves it over path in one step: path holds either its old bytes or all the new ones.
bool replaceFile(const char *path, const std::string &text)
{
    std::string tmp = std::string(path) + ".controls-tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        if (g_fault == 1) {
            out << text.substr(0, text.size() / 2);
            out.close();
            std::remove(tmp.c_str());
            return false;
        }
        out << text;
        out.flush();
        if (!out) { out.close(); std::remove(tmp.c_str()); return false; }
    }
    bool moved = g_fault != 2;
#ifdef _WIN32
    moved = moved && MoveFileExA(tmp.c_str(), path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#else
    moved = moved && std::rename(tmp.c_str(), path) == 0;
#endif
    if (!moved) std::remove(tmp.c_str());
    return moved;
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
    // Native extensions stay local: Shared/Core keeps the toolkit's existing action table.
    std::vector<std::string> common;
    for (const auto &line : body) if (!nativeDigital(lineKey(line))) common.push_back(line);
    ctl::Section s = ctl::parseSection(common, 2);
    if (s.error.empty()) for (const auto &line : body) {
        const std::string action = lineKey(line);
        if (!nativeDigital(action)) continue;
        const std::string value = ctl::trim(line.substr(line.find('=') + 1));
        if (value.empty()) { s.unbound.push_back(action); continue; }
        ctl::Binding binding;
        std::string why;
        if (ctl::parse(value, binding, why, s.schema)) why = ctl::fits(ctl::Shape::Digital, binding);
        if (!why.empty()) { s.invalid.push_back({action, why}); continue; }
        if (ctl::find(s, action)) { s.invalid.push_back({action, "duplicate"}); continue; }
        s.bound.push_back({action, binding});
    }
    snprintf(p->profile, sizeof(p->profile), "%s", s.profile.c_str());
    if (!s.revision.empty()) snprintf(p->revision, sizeof(p->revision), "%s", s.revision.c_str());
    else snprintf(p->revision, sizeof(p->revision), "fnv1a:%08x", (unsigned)fnv1a(joined));
    if (!s.error.empty()) { snprintf(p->error, sizeof(p->error), "%s", s.error.c_str()); return; }
    // Schema 1 retains its per-entry notes. A malformed Schema-2 request must not
    // partially apply new calibration alongside an old axis or direction.
    if (s.schema == 2 && !s.invalid.empty()) {
        snprintf(p->error, sizeof(p->error), "%s: %s", s.invalid[0].first.c_str(), s.invalid[0].second.c_str());
        return;
    }
    if (s.schema == 2) {
        std::vector<std::string> seen;
        for (const auto &line : body) {
            const std::string key = lineKey(line);
            if (ctl::shapeOf(key) == ctl::Shape::Unknown && !nativeDigital(key)) continue;
            for (const auto &previous : seen) if (previous == key) {
                snprintf(p->error, sizeof(p->error), "%s: duplicate action", key.c_str());
                return;
            }
            seen.push_back(key);
        }
    }
    for (auto &bad : s.invalid) note(p, bad.first, bad.second.c_str());

    const ctl::Binding *steer = ctl::find(s, "steer");
    if (!steer) { snprintf(p->error, sizeof(p->error), "no steer binding: F-Zero needs the wheel's steering axis"); return; }
    uint32_t id = ctl::productId(*steer);
    p->vendor = id & 0xFFFFu;
    p->product = id >> 16;
    snprintf(p->device, sizeof(p->device), "%s", steer->name.c_str());
    bool qualified = false;
    for (const Qualified &q : kQualified) qualified = qualified || (q.vendor == p->vendor && q.product == p->product);
    if (!qualified) {
        snprintf(p->error, sizeof(p->error),
                 "%04x:%04x is not qualified (its SDL and DirectInput numbering were never compared)", p->vendor,
                 p->product);
        return;
    }
    if (!axisProfile(p, 0, *steer, s.schema)) return;
    setKey(p, "SteeringAxis", steer->index);
    // Always write both directions: a later noninverted profile must clear a
    // prior launcher/profile inversion instead of silently inheriting it.
    setKey(p, "SteeringInvert", steer->inverted ? 1 : 0);

    for (const ctl::Entry &e : s.bound) {
        const ctl::Binding &b = e.binding;
        if (e.action == "steer") continue;
        bool sameDevice = b.dev == steer->dev && b.prod == steer->prod;
        const char *identityMismatch = b.dev == steer->dev
            ? "wheel product mismatch; rebind to selected wheel"
            : nullptr;
        if (e.action == "throttle" || e.action == "brake") {
            if (b.kind != ctl::Kind::Axis) {
                if (s.schema == 2) { snprintf(p->error, sizeof(p->error), "%s: Native pedals require axes", e.action.c_str()); return; }
                note(p, e.action, "a button pedal has no F-Zero setting (pedals are axes)");
                continue;
            }
            if (!sameDevice) {
                if (s.schema == 2) { snprintf(p->error, sizeof(p->error), "%s: %s", e.action.c_str(), identityMismatch ? identityMismatch : "Native reads one controller"); return; }
                note(p, e.action, identityMismatch ? identityMismatch : "on another device (F-Zero reads one controller)"); continue;
            }
            bool gas = e.action == "throttle";
            if (!axisProfile(p, gas ? 1 : 2, b, s.schema)) return;
            setKey(p, gas ? "AcceleratorAxis" : "BrakeAxis", b.index);
            setKey(p, gas ? "AcceleratorInvert" : "BrakeInvert", pedalInvert(b));
            continue;
        }
        const Digital *map = nullptr;
        for (const Digital &d : kDigital) if (e.action == d.action) map = &d;
        if (!map) {
            if (s.schema == 2) { snprintf(p->error, sizeof(p->error), "%s: no Native F-Zero control", e.action.c_str()); return; }
            note(p, e.action, "no F-Zero control"); continue;
        }
        if (!sameDevice) {
            if (s.schema == 2) { snprintf(p->error, sizeof(p->error), "%s: %s", e.action.c_str(), identityMismatch ? identityMismatch : "Native reads one controller"); return; }
            note(p, e.action, identityMismatch ? identityMismatch : "on another device (F-Zero reads one controller)"); continue;
        }
        int v = digital(e.action, b, p);
        if (v < 0 && s.schema == 2) {
            snprintf(p->error, sizeof(p->error), "%s: binding cannot be represented by Native", e.action.c_str());
            return;
        }
        if (v >= 0) setKey(p, map->key, v);
    }
    // Explicitly empty entries unbind what F-Zero has for them; steering stays (the wheel needs it).
    for (const std::string &a : s.unbound) {
        if (a == "throttle") { setKey(p, "AcceleratorAxis", -1); p->axis_profiles |= 2u; continue; }
        if (a == "brake") { setKey(p, "BrakeAxis", -1); p->axis_profiles |= 4u; continue; }
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
    // A UTF-8 BOM hides the first section from the game's reader and the launcher's writer alike (FzeroIniReadString,
    // launcher_ini_kv_write): refuse rather than edit a file the game reads differently from this code.
    if (config_path) {
        std::ifstream in(config_path, std::ios::binary);
        char head[3] = {0, 0, 0};
        if (in.read(head, 3) && (unsigned char)head[0] == 0xEF && (unsigned char)head[1] == 0xBB && (unsigned char)head[2] == 0xBF) {
            snprintf(p->error, sizeof(p->error), "config.ini starts with a UTF-8 BOM, which the game's reader does not skip; remove it");
            return 1;
        }
    }
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
    Doc d;
    readLines(path, d.lines, d.crlf); // a missing file starts empty
    setInDoc(d, section, key, value ? value : "");
    return replaceFile(path, render(d));
}

extern "C" void FzeroControlsTestFault(int fault) { g_fault = fault; }

// One transaction: the backup first (once), then every key, GuidP1 and the [ControlsApplied] record in a single
// document that is read back before it replaces config.ini. A failure at any step leaves config.ini's bytes as they were.
extern "C" int FzeroControlsWrite(const char *config_path, const char *guid, const FzeroControlsPlan *p)
{
    if (!p->ok || !guid || !guid[0]) return 0;
    Doc d;
    if (!readLines(config_path, d.lines, d.crlf)) return 0;
    const std::string original = render(d);
    std::string backup = std::string(config_path) + ".before-profile-controls";
    if (!std::ifstream(backup, std::ios::binary)) {
        std::ifstream in(config_path, std::ios::binary);
        std::stringstream bytes;
        bytes << in.rdbuf();
        if (!in || !replaceFile(backup.c_str(), bytes.str())) return 0;
    }
    const std::string section = std::string("Controller.") + guid;
    char number[16];
    for (int i = 0; i < p->nkeys; ++i) {
        snprintf(number, sizeof(number), "%d", p->keys[i].value);
        setInDoc(d, section, p->keys[i].key, number);
    }
    for (int i = 0; i < 3; ++i) if ((p->axis_profiles & (1u << i)) &&
        (p->axis_profile[i][0] || !docValue(d, section, kAxisProfileKeys[i]).empty()))
        setInDoc(d, section, kAxisProfileKeys[i], p->axis_profile[i]);
    setInDoc(d, "Controller", "GuidP1", guid);
    setInDoc(d, "ControlsApplied", "Revision", p->revision);
    setInDoc(d, "ControlsApplied", "Device", guid);
    for (int i = 0; i < p->nkeys; ++i) {
        snprintf(number, sizeof(number), "%d", p->keys[i].value);
        if (docValue(d, section, p->keys[i].key) != number) return 0;
    }
    for (int i = 0; i < 3; ++i) if ((p->axis_profiles & (1u << i)) &&
        docValue(d, section, kAxisProfileKeys[i]) != p->axis_profile[i]) return 0;
    if (docValue(d, "Controller", "GuidP1") != guid || docValue(d, "ControlsApplied", "Revision") != p->revision)
        return 0;
    const std::string text = render(d);
    return text == original || replaceFile(config_path, text);
}

extern "C" void FzeroControlsNativeLoad(const char *config_path, const char *section)
{
    for (int i = 0; i < 3; ++i) {
        auto &axis = g_native_axes[i];
        axis = NativeAxisProfile{};
        if (!config_path || !section) continue;
        const std::string text = readValue(config_path, section, kAxisProfileKeys[i]);
        if (text.empty()) continue;
        axis.present = true;
        std::string why;
        axis.valid = ctl::parse(text, axis.binding, why, 2) && axis.binding.kind == ctl::Kind::Axis &&
            ctl::fits(kAxisActions[i], axis.binding).empty() && axis.binding.min == 0 && axis.binding.max == 65535;
        if (!axis.valid) fprintf(stderr, "[fzero-controls] %s is invalid; axis held neutral until repaired or reapplied\n", kAxisProfileKeys[i]);
    }
}

extern "C" int FzeroControlsNativeNormalize(int slot, int axis, int inverted, int sdl_raw, double *value)
{
    if (!value || slot < 0 || slot >= 3) return -1;
    *value = 0.0;
    auto &profile = g_native_axes[slot];
    if (!profile.present) return 0;
    if (!profile.valid) return -1;
    auto &binding = profile.binding;
    if (binding.index != axis) return 0; // deliberate launcher reassignment; never transfer calibration to that axis
    // Native pedals express inversion relative to the SDL axis; the contract
    // expresses it after travel/calibration. Translate the switch, not the value.
    binding.inverted = slot == 0 ? inverted != 0 : (binding.travel > 0) == (inverted != 0);
    const long raw = static_cast<long>(ctl::clampd(double(sdl_raw), -32768.0, 32767.0)) + 32768;
    *value = ctl::normalize(slot == 0 ? ctl::Shape::Centred : ctl::Shape::Pedal, binding, raw);
    return 1;
}
