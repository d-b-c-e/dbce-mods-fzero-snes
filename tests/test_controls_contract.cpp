// Canonical toolkit vector harness (ee8014d); no game or device access.
// test_controls.exe [vectors file]
#include "vendor/controls/dbce_controls.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
// Exercise the production mapper without adding a game/SDL dependency to this offline target.
#include "../src/fzero_controls.cpp"
#include "raw_hat_binding.h"

using namespace dbce::controls;

static int g_checks, g_failures;
static void check(bool ok, const std::string &what)
{
    ++g_checks;
    if (!ok) { ++g_failures; std::cerr << "FAIL " << what << "\n"; }
}
static std::vector<std::string> fields(const std::string &line)
{
    std::vector<std::string> f;
    size_t p = 0;
    for (;;) {
        size_t q = line.find(" | ", p);
        if (q == std::string::npos) { f.push_back(line.substr(p)); break; }
        f.push_back(line.substr(p, q - p));
        p = q + 3;
    }
    return f;
}
static Binding must(const std::string &text, const std::string &where, int schema = 1)
{
    Binding b;
    std::string why;
    check(parse(text, b, why, schema), where + ": vector binding parses (" + why + ")");
    return b;
}

static FzeroControlsPlan nativePlan(int schema, const std::vector<std::string> &actions)
{
    std::vector<std::string> lines = {
        "Schema = " + std::to_string(schema), "Revision = native-actions",
        "steer = axis 0 dev={11111111-2222-3333-4444-555555555555} "
        "prod={0006346e-0000-0000-0000-504944564944} range=0..65535 rest=32768 travel=-1 inverted" +
        std::string(schema == 2 ? " cal=1000..30000..60000" : "")
    };
    lines.insert(lines.end(), actions.begin(), actions.end());
    std::vector<const char *> pointers;
    for (const auto &line : lines) pointers.push_back(line.c_str());
    FzeroControlsPlan plan;
    FzeroControlsPlanLines(pointers.data(), (int)pointers.size(), &plan);
    return plan;
}

static int mapped(const FzeroControlsPlan &plan, const char *name)
{
    for (int i = 0; i < plan.nkeys; ++i) if (std::string(plan.keys[i].key) == name) return plan.keys[i].value;
    return -1000;
}

static void nativeActions()
{
    const char *actions[] = {"nativeX", "nativeY", "nativeL", "nativeR", "saveStateMenu", "rewind"};
    const char *keys[] = {"ButtonX", "ButtonY", "ButtonL", "ButtonR", "ButtonSaveStateMenu", "ButtonRewind"};
    const std::string wheel = " dev={11111111-2222-3333-4444-555555555555} prod={0006346e-0000-0000-0000-504944564944}";
    for (int schema : {1, 2}) {
        for (int i = 0; i < 6; ++i) {
            const std::string action = actions[i];
            auto button = nativePlan(schema, {action + " = button 127" + wheel});
            check(button.ok && button.nnotes == 0 && mapped(button, keys[i]) == 127, action + " button");
            for (int direction = 0; direction < 4; ++direction) {
                auto hat = nativePlan(schema, {action + " = hat 3 " + std::to_string(direction * 9000) + wheel});
                check(hat.ok && mapped(hat, keys[i]) == 140 + direction, action + " hat direction");
            }
            auto clear = nativePlan(schema, {action + " ="});
            check(clear.ok && mapped(clear, keys[i]) == -1, action + " explicit clear");
            auto absent = nativePlan(schema, {});
            check(absent.ok && mapped(absent, keys[i]) == -1000, action + " absent");
            for (const auto &bad : {"axis 1" + wheel + " range=0..65535 rest=0 travel=+1",
                                    "hat 0 4500" + wheel, "hat 4 0" + wheel, "button 128" + wheel,
                                    std::string("button 1"), "button 1" + wheel + " cal=0..1"}) {
                if (schema == 1 && bad.find("cal=") != std::string::npos) continue; // schema-1 ignores cal attributes
                auto refused = nativePlan(schema, {action + " = " + bad});
                // Schema 1 reports/skips invalid entries; Schema 2 refuses the transaction.
                check(mapped(refused, keys[i]) == -1000 && (schema == 1 ? refused.nnotes > 0 : !refused.ok), action + " refusal");
            }
            auto other = nativePlan(schema, {action + " = button 1 dev={66666666-7777-8888-9999-aaaaaaaaaaaa} "
                "prod={0006346e-0000-0000-0000-504944564944}"});
            check(mapped(other, keys[i]) == -1000 && (schema == 1 ? other.nnotes > 0 : !other.ok), action + " other device refused");
        }
        for (const std::string unknown : {"NativeX", "savestatemenu", "nativeZ"}) {
            auto refused = nativePlan(schema, {unknown + " = button 1" + wheel});
            check(schema == 1 ? refused.nnotes > 0 : !refused.ok, unknown + " unknown/case refused");
        }
        // A full profile must retain all keys, including the final Native tools.
        std::vector<std::string> full = {"throttle = axis 2" + wheel + " range=0..65535 rest=0 travel=+1",
            "brake = axis 5" + wheel + " range=0..65535 rest=0 travel=+1"};
        for (const char *action : {"confirm", "back", "start", "select", "navUp", "navDown", "navLeft", "navRight"})
            full.push_back(std::string(action) + " = button 1" + wheel);
        for (const char *action : actions) full.push_back(std::string(action) + " = button 2" + wheel);
        auto plan = nativePlan(schema, full);
        check(plan.ok && plan.nkeys == 20 && mapped(plan, "ButtonRewind") == 2 && mapped(plan, "SteeringInvert") == 1,
              "full profile fits and retains inversion");
    }
    // Common actions use the same selected-device contract as Native host tools.
    const char *common[] = {"confirm", "back", "start", "select", "navUp", "navDown", "navLeft", "navRight"};
    const char *commonKeys[] = {"ButtonA", "ButtonB", "ButtonStart", "ButtonSelect", "ButtonUp", "ButtonDown", "ButtonLeft", "ButtonRight"};
    for (int schema : {1, 2}) {
        for (int i = 0; i < 8; ++i) {
            const std::string action = common[i];
            for (int hat = 0; hat < 4; ++hat)
                for (int direction = 0; direction < 4; ++direction) {
                    auto plan = nativePlan(schema, {action + " = hat " + std::to_string(hat) + " " + std::to_string(direction * 9000) + wheel});
                    check(plan.ok && mapped(plan, commonKeys[i]) == 128 + hat * 4 + direction, action + " cardinal admission");
                }
            for (const auto &bad : {"hat 0 4500" + wheel,
                    std::string("hat 0 0 dev={66666666-7777-8888-9999-aaaaaaaaaaaa} prod={0006346e-0000-0000-0000-504944564944}"),
                    std::string("hat 0 0 dev={11111111-2222-3333-4444-555555555555} prod={0001346e-0000-0000-0000-504944564944}")}) {
                auto plan = nativePlan(schema, {action + " = " + bad});
                check(mapped(plan, commonKeys[i]) == -1000 && (schema == 1 ? plan.nnotes > 0 : !plan.ok), action + " diagonal/identity refusal");
            }
        }
        // A stale product ID on the same instance needs actionable, accurate feedback.
        const char *conflictActions[] = {"confirm", "back", "start", "select", "navUp", "navDown", "navLeft", "navRight", "throttle", "saveStateMenu"};
        const char *conflictKeys[] = {"ButtonA", "ButtonB", "ButtonStart", "ButtonSelect", "ButtonUp", "ButtonDown", "ButtonLeft", "ButtonRight", "AcceleratorAxis", "ButtonSaveStateMenu"};
        for (int i = 0; i < 10; ++i) {
            const char *action = conflictActions[i];
            const bool pedal = std::string(action) == "throttle";
            auto plan = nativePlan(schema, {std::string(action) + " = " +
                (pedal ? "axis 2" : "button 1") +
                " dev={11111111-2222-3333-4444-555555555555} prod={0001346e-0000-0000-0000-504944564944}" +
                (pedal ? " range=0..65535 rest=0 travel=+1" : "")});
            const std::string message = std::string(action) +
                ": wheel product mismatch; rebind to selected wheel";
            check(schema == 1 ? plan.ok && plan.nnotes == 1 && std::string(plan.notes[0]) == message
                              : !plan.ok && std::string(plan.error) == message,
                  std::string(action) + " exact product-conflict feedback");
            check(mapped(plan, conflictKeys[i]) == -1000,
                  std::string(action) + " conflicting product not mapped");
        }
        for (const char *unsupported : {"clutch", "neutral", "gear1", "shiftUp", "restart"}) {
            auto plan = nativePlan(schema, {std::string(unsupported) + " = button 1" + wheel});
            check(schema == 1 ? plan.nnotes > 0 : !plan.ok, std::string(unsupported) + " is not a supported game action");
        }
    }
    for (const auto &duplicate : {std::vector<std::string>{"nativeX =", "nativeX = button 1" + wheel},
                                 std::vector<std::string>{"rewind =", "rewind ="}})
        check(!nativePlan(2, duplicate).ok, "schema2 duplicate native request refuses");
    // Shared/Core's common action parser must retain its old admission behavior.
    check(!parseSection({"Schema = 1", "nativeX = button 1" + wheel}).invalid.empty(), "Shared table unchanged");

    const auto directory = std::filesystem::temp_directory_path() /
        ("fzero-native-actions-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto path = (directory / "config.ini").string();
    { std::ofstream out(path); out << "[Controller.abc]\nDeadzone = 17\nButtonY = 8\n"; }
    std::vector<std::string> all;
    for (int i = 0; i < 6; ++i) all.push_back(std::string(actions[i]) + " = button " + std::to_string(i + 10) + wheel);
    auto plan = nativePlan(2, all);
    check(FzeroControlsWrite(path.c_str(), "abc", &plan) == 1, "native roundtrip writes");
    for (int i = 0; i < 6; ++i)
        check(readValue(path.c_str(), "Controller.abc", keys[i]) == std::to_string(i + 10), "canonical native readback");
    check(readValue(path.c_str(), "Controller.abc", "Deadzone") == "17" && !FzeroControlsPending(path.c_str(), &plan),
          "native write preserves settings and records revision");
    FzeroControlsNativeLoad(path.c_str(), "Controller.abc");
    double normalized = 0;
    check(FzeroControlsNativeNormalize(0, 0, 1, 1000 - 32768, &normalized) == 1 && normalized == 1,
          "native actions retain calibrated steering and inversion");
    FzeroControlsNativeLoad(nullptr, nullptr);
    for (int i = 0; i < 6; ++i) {
        plan = nativePlan(2, {std::string(actions[i]) + " ="});
        check(FzeroControlsWrite(path.c_str(), "abc", &plan) == 1, "clear roundtrip writes");
        for (int j = 0; j < 6; ++j)
            check(readValue(path.c_str(), "Controller.abc", keys[j]) == (j <= i ? "-1" : std::to_string(j + 10)),
                  "clear and absent roundtrip independently");
    }
    auto refused = nativePlan(2, {"NativeX ="});
    check(!FzeroControlsWrite(path.c_str(), "abc", &refused) && readValue(path.c_str(), "Controller.abc", "ButtonX") == "-1",
          "refused request cannot write");
    std::filesystem::remove_all(directory);
}

// Use the launcher's production decoder, with the bit-membership operation in
// fzero_gamepad.c. No SDL device (including virtual devices) is opened here.
static void cardinalConsumer()
{
    const std::string wheel = " dev={11111111-2222-3333-4444-555555555555} prod={0006346e-0000-0000-0000-504944564944}";
    const char *actions[] = {"navUp", "navRight", "navDown", "navLeft"};
    const char *keys[] = {"ButtonUp", "ButtonRight", "ButtonDown", "ButtonLeft"};
    const int bits[] = {1, 2, 4, 8};
    for (int schema : {1, 2})
        for (int hat = 0; hat < 4; ++hat)
            for (int direction = 0; direction < 4; ++direction) {
                auto plan = nativePlan(schema, {std::string(actions[direction]) + " = hat " +
                    std::to_string(hat) + " " + std::to_string(direction * 9000) + wheel});
                int binding = mapped(plan, keys[direction]);
                check(plan.ok && recomp_raw_hat_index(binding) == hat &&
                    recomp_raw_hat_value(binding) == bits[direction], "planned cardinal decodes to selected hat/direction");
                // All SDL hat states: center, cardinals, diagonals, and opposing
                // bit combinations. A diagonal is two pressed cardinal bindings.
                for (int state = 0; state < 16; ++state)
                    check(bool(state & recomp_raw_hat_value(binding)) == bool(state & bits[direction]),
                          "consumer direction/center membership");
                auto cleared = nativePlan(schema, {std::string(actions[direction]) + " ="});
                check(recomp_raw_hat_index(mapped(cleared, keys[direction])) == -1 &&
                    recomp_raw_hat_value(mapped(cleared, keys[direction])) == 0,
                    "explicit unbind cannot retain a hat direction");
            }
    for (int binding : {-1000, -1, 0, 127, 192})
        check(recomp_raw_hat_index(binding) == -1 && recomp_raw_hat_value(binding) == 0,
              "absent/button/out-of-range is not a hat");
}

int main(int argc, char **argv)
{
    nativeActions();
    cardinalConsumer();
    const char *path = argc > 1 ? argv[1] : "controls-vectors.txt";
    std::ifstream in(path);
    if (!in) { std::cerr << "cannot open " << path << "\n"; return 2; }
    std::string line;
    int n = 0, vectors = 0;
    while (std::getline(in, line)) {
        ++n;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        auto f = fields(line);
        const std::string where = std::string(path) + ":" + std::to_string(n);
        ++vectors;
        // "parse2", "norm2", ...: the same operation reading the binding as schema 2.
        const int schema = f[0].size() > 1 && f[0].back() == '2' ? 2 : 1;
        if (schema == 2) f[0].pop_back();
        if (f[0] == "parse" && f.size() == 3) {
            Binding b; std::string why;
            bool ok = parse(f[1], b, why, schema);
            check(ok, where + " parses (" + why + ")");
            if (ok) {
                check(format(b) == f[2], where + " canonical: " + format(b));
                Binding again; check(parse(format(b), again, why, schema) && format(again) == f[2], where + " round trip");
            }
        } else if (f[0] == "reject" && f.size() == 3) {
            Binding b; std::string why;
            bool ok = parse(f[1], b, why, schema);
            check(!ok && why == f[2], where + " rejected as " + f[2] + " (got " + (ok ? "accepted" : why) + ")");
        } else if (f[0] == "fits" && f.size() == 4) {
            Binding b = must(f[2], where, schema);
            std::string why = fits(f[1], b);
            check((why.empty() ? std::string("ok") : why) == f[3], where + " fits " + f[1] + ": " + (why.empty() ? "ok" : why));
        } else if (f[0] == "norm" && f.size() == 5) {
            Binding b = must(f[2], where, schema);
            double got = normalize(f[1], b, std::strtol(f[3].c_str(), nullptr, 10));
            check(std::fabs(got - std::strtod(f[4].c_str(), nullptr)) <= 1e-6, where + " norm " + f[1] + " " + f[3] + " = " + std::to_string(got));
        } else if (f[0] == "denorm" && f.size() == 5) {
            Binding b = must(f[2], where, schema);
            long got = denormalize(f[1], b, std::strtod(f[3].c_str(), nullptr));
            check(got == std::strtol(f[4].c_str(), nullptr, 10), where + " denorm " + f[1] + " " + f[3] + " = " + std::to_string(got));
        } else if (f[0] == "rescale" && f.size() == 7) {
            long v[6];
            for (int i = 0; i < 6; ++i) v[i] = std::strtol(f[i + 1].c_str(), nullptr, 10);
            check(rescale(v[0], v[1], v[2], v[3], v[4]) == v[5], where + " rescale");
        } else if (f[0] == "dibutton" && f.size() == 3) {
            check((diButtonDown(std::strtol(f[1].c_str(), nullptr, 10)) ? 1 : 0) == std::strtol(f[2].c_str(), nullptr, 10), where + " dibutton");
        } else check(false, where + " unknown vector: " + line);
    }
    check(vectors >= 70, "at least 70 vectors read (" + std::to_string(vectors) + ")");

    std::cout << vectors << " vectors, " << g_checks << " checks, " << g_failures << " failures\n";
    return g_failures ? 1 : 0;
}
