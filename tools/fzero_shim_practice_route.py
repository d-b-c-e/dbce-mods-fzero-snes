#!/usr/bin/env python3
"""Private, pinned Deluxe Practice route. Reads recorded guest state; never writes it.

Run alongside Invoke-ShimDiagnosticCopy.ps1 Run. Every positive batch goes through
that wrapper's lease/process/frame checks and the production Core mapper. No OS
input, guest-memory access, replay-state edit, force delivery or window change.
"""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import subprocess
import time

ROM = "419eed641cfa20013d1e9b8270062d90155051a441441a2118bf23846ac8d747"
NEUTRAL = 1 << 30
BUTTON = ("inject action confirm 1 ms=100",)


class Refused(ValueError):
    pass


class PracticeRoute:
    """Single-use exact offline route; unknown states stop, never guess/retry."""
    allowed = (
        {(0, 0, 0), (0, 1, 0)},
        {(0, 1, 0)},
        {(0, 1, 0), (1, 0, 0), (1, 1, 0)},
        {(1, 1, 0), (1, 1, 1)},
        {(1, 1, 1), (1, 1, 3)},
        {(1, 1, 3), (1, 1, 4)},
        {(1, 1, 4), (1, 7, 4), (2, 1, 0), (2, 1, 1)},
        {(2, 1, 1), (2, 2, 0), (2, 2, 1), (2, 2, 2), (2, 3, 0)},
        {(2, 3, 0)},
        {(2, 3, 0)},
    )
    names = ("select-Practice", "enter-Practice", "Blue-Falcon", "accept-car",
             "default-rival", "Knight-League", "Mute-City-I", "throttle",
             "throttle-steer", "end")

    def __init__(self):
        self.step = 0
        self.sequence = -1
        self.elapsed = -1.0
        self.neutral_since = None
        self.command_until = -1.0
        self.countdown = False
        self.moving = False
        self.last = None

    def observe(self, sample):
        c = sample["channels"]
        required = ["latched", "suppressNative", "run", "game.valid", "game.inputWord",
                    "game.state54", "game.state55", "game.state56", "game.simulationFrame",
                    "game.telemetry.speedMps"] + ["game.menu." + x for x in
                    ("selection", "training", "transition", "car", "course", "league",
                     "deluxeCar", "deluxeRival", "deluxePanel", "deluxeAccept", "deluxeTrack", "state5b")]
        if any(k not in c or not isinstance(c[k], (int, float)) or not math.isfinite(c[k]) for k in required):
            raise Refused("missing/non-finite source field")
        seq, elapsed = sample["sequence"], sample["elapsedSeconds"]
        if seq <= self.sequence or not math.isfinite(elapsed) or elapsed < self.elapsed:
            raise Refused("source sequence/time did not advance")
        if c["game.valid"] != 1 or c["game.simulationFrame"] != seq:
            raise Refused("no matching post-frame guest observation")
        if c["latched"] != 1 or c["suppressNative"] != 1 or c["run"] not in (1, 2, 3):
            raise Refused("latched session no longer owns the input lane")
        state = tuple(c[f"game.state{x}"] for x in (54, 55, 56))
        if state not in self.allowed[self.step]:
            raise Refused(f"unexpected state {state} at {self.names[self.step]}")
        # All post-title states must be explicitly Practice/player, not attract.
        if state[0] != 0 and (c["game.menu.training"] != 1 or c["game.menu.state5b"] != 0):
            raise Refused("not the selected player Practice route")
        # The real player countdown is substate 2 ($00:8CBD); attract can
        # leave substate 1 directly. Require the player path's actual countdown.
        if self.step >= 7 and state == (2, 2, 2):
            self.countdown = True
        if self.step >= 8 and self.countdown and state == (2, 3, 0) and c["game.telemetry.speedMps"] > 1:
            self.moving = True
        if c["game.inputWord"] == NEUTRAL:
            if self.neutral_since is None:
                self.neutral_since = elapsed
        else:
            self.neutral_since = None
        self.sequence, self.elapsed, self.last = seq, elapsed, c

    def ready(self):
        c = self.last
        if c is None or c["run"] != 3 or self.neutral_since is None:
            return False
        if self.elapsed - self.neutral_since < .15 or self.elapsed < self.command_until:
            return False
        s = tuple(c[f"game.state{x}"] for x in (54, 55, 56))
        m = lambda key: c["game.menu." + key]
        # A global fade must finish before input is eligible.
        if m("transition") != 0:
            return False
        if self.step == 0: return s == (0, 1, 0) and m("selection") == 0
        if self.step == 1: return s == (0, 1, 0) and m("selection") == 1
        if self.step == 2: return s == (1, 1, 0) and m("deluxeCar") == 0 and m("deluxePanel") == 0 and m("deluxeAccept") == 0
        if self.step == 3: return s == (1, 1, 1) and m("deluxeCar") == 0 and m("deluxePanel") == 0x87 and m("deluxeAccept") == 0
        if self.step == 4: return s == (1, 1, 3) and m("deluxeCar") == 0 and m("deluxeRival") == 0 and m("deluxePanel") == 0x1f
        if self.step == 5: return s == (1, 1, 4) and m("league") == 0 and m("deluxeRival") == 0 and m("deluxePanel") == 0x1f
        course = m("car") == 0 and m("league") == 0 and m("course") == 0 and m("deluxeTrack") == 0
        if self.step == 6: return s == (2, 1, 1) and course
        if self.step == 7: return s == (2, 3, 0) and course and self.countdown
        return s == (2, 3, 0) and course and self.countdown and self.moving

    def commands(self):
        if not self.ready():
            raise Refused("route action is not admitted")
        if self.step == 0: return ("inject action navDown 1 ms=100",)
        if self.step < 7: return BUTTON
        if self.step == 7: return ("inject action throttle 1 ms=2000",)
        if self.step == 8: return ("inject action throttle 1 ms=1000", "inject action steer 0.25 ms=1000")
        return ("end",)

    def sent(self):
        commands = self.commands()
        self.command_until = self.elapsed + (2.25 if self.step == 7 else 1.25 if self.step == 8 else .25)
        self.neutral_since = None
        self.step += 1
        return commands


def verify_header(header):
    if len(header) < 40 or header[:8] != b"FZPT0001" or header[8:40].hex() != ROM:
        raise Refused("effective Deluxe cartridge does not match the reviewed route")


class SourceReader:
    def __init__(self, path, nonce):
        self.path, self.nonce = path, nonce
        self.offset = 0
        self.started = None

    def read(self):
        if not self.path.exists(): return []
        rows = []
        with self.path.open("rb") as stream:
            if self.path.stat().st_size < self.offset: raise Refused("source was truncated")
            stream.seek(self.offset)
            while True:
                line = stream.readline()
                if not line.endswith(b"\n"): break
                self.offset = stream.tell()
                obj = json.loads(line)
                if obj["kind"] == "metadata":
                    m = obj["metadata"]
                    if self.started or m["game"] != "fzero-snes" or m["pluginVersion"] != "fzero-shim-diagnostic-3" or m["properties"]["runNonce"] != self.nonce or not m["properties"]["context"].startswith("diagnostic-injected-original;"):
                        raise Refused("source provenance mismatch")
                    self.started = datetime.fromisoformat(m["startedUtc"].replace("Z", "+00:00")).timestamp()
                elif obj["kind"] == "sample":
                    if self.started is None: raise Refused("sample before metadata")
                    rows.append(obj["sample"])
        return rows

    def fresh(self, route):
        age = time.time() - (self.started + route.elapsed)
        if not -.5 <= age <= 2:
            raise Refused(f"source is not fresh ({age:.3f}s)")


def run(probe, wrapper):
    if not probe.is_dir() or (probe / "route.json").exists():
        raise Refused("requires a fresh prepared copy with no prior route")
    report = {"schema": "fzero.private-practice-route", "version": 1,
              "startedUtc": datetime.now(timezone.utc).isoformat(), "effectiveRomSha256": ROM,
              "steps": [], "playerDriveObserved": False, "errors": []}
    route = PracticeRoute()
    deadline = time.monotonic() + 70
    def invoke(action, *args):
        remaining = deadline - time.monotonic()
        if remaining <= 0: raise Refused("route deadline")
        result = subprocess.run(["pwsh", "-NoProfile", "-File", str(wrapper), "-Action", action,
                                 "-Probe", str(probe), *map(str, args)], capture_output=True, text=True,
                                timeout=min(10, remaining), creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        if result.returncode: raise Refused(result.stderr.strip() or result.stdout.strip())
        return result.stdout
    try:
        while not (probe / "process.json").exists():
            if time.monotonic() >= deadline: raise Refused("no diagnostic process")
            time.sleep(.05)
        identity = json.loads((probe / "process.json").read_text(encoding="utf-8-sig"))
        nonce = (probe / "control/nonce.txt").read_text().strip()
        reader = SourceReader(probe / "run/capture/source.jsonl", nonce)
        verified_rom = False
        window = None
        while time.monotonic() < deadline:
            for sample in reader.read(): route.observe(sample)
            if route.last is None: time.sleep(.05); continue
            reader.fresh(route)
            if not verified_rom:
                tape = probe / "run/original.fzpt"
                if not tape.exists() or tape.stat().st_size < 40: time.sleep(.05); continue
                with tape.open("rb") as stream: verify_header(stream.read(40))
                verified_rom = True
            if not route.ready(): time.sleep(.03); continue
            if window is None:
                # Wrapper verifies child identity/lease and retains the window inventory.
                invoke("Capture")
                windows = json.loads((probe / "frames/windows.json").read_text(encoding="utf-8-sig"))
                choices = [w for w in windows["windows"] if w["Title"] == "F-Zero" and w["Class"] == "SDL_app" and w["Area"] > 0]
                if windows["process"] != identity["pid"] or len(choices) != 1:
                    raise Refused("no unique owned F-Zero window")
                window = choices[0]["Handle"]
            before_seq = route.sequence
            output = invoke("Capture", "-WindowHandle", window)
            frames = [Path(line.strip()) for line in output.splitlines() if line.strip().endswith(".png")]
            if len(frames) != 1: raise Refused("capture did not return one frame")
            for sample in reader.read(): route.observe(sample)
            reader.fresh(route)
            if route.sequence <= before_seq or not route.ready():
                raise Refused("route changed while capturing; no command sent")
            commands = route.commands()
            # Use -Command with a literal script block rather than PS array argument
            # passing through -File, which treats trailing commands as positional args.
            # Every string here is a fixed route command, not user input.
            if len(commands) == 1:
                raw = invoke("Command", "-InspectedFrame", frames[0], "-Commands", commands[0])
            else:
                escaped = lambda s: "'" + str(s).replace("'", "''") + "'"
                script = "& " + escaped(wrapper) + " -Action Command -Probe " + escaped(probe) + " -InspectedFrame " + escaped(frames[0]) + " -Commands " + ",".join(map(escaped, commands))
                result = subprocess.run(["pwsh", "-NoProfile", "-Command", script], capture_output=True, text=True, timeout=min(10, deadline-time.monotonic()), creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
                if result.returncode: raise Refused(result.stderr.strip())
                raw = result.stdout
            report["steps"].append({"name": route.names[route.step], "sequence": route.sequence,
                                    "elapsed": route.elapsed, "state": route.last, "frame": str(frames[0]),
                                    "batch": json.loads(raw)})
            done = route.step == 9
            route.sent()
            if done:
                report["playerDriveObserved"] = route.moving and route.countdown
                break
        else: raise Refused("route deadline reached")
    except Exception as exc:
        report["errors"].append(str(exc))
        try: report["end"] = json.loads(invoke("Command", "-Commands", "end"))
        except Exception as stop: report["endError"] = str(stop)
    finally:
        report["endedUtc"] = datetime.now(timezone.utc).isoformat()
        report["nextStep"] = route.step
        report["lastObservation"] = route.last
        (probe / "route.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps({k: v for k, v in report.items() if k != "steps"}, indent=2))
    return 1 if report["errors"] else 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--wrapper", type=Path, default=Path(__file__).with_name("Invoke-ShimDiagnosticCopy.ps1"))
    args = parser.parse_args()
    raise SystemExit(run(args.probe.resolve(), args.wrapper.resolve()))
