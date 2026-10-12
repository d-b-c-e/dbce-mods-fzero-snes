"""ROM-free checks of the actual private router's admission and source reader."""
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("practice_route", Path(__file__).parents[1] / "tools/fzero_shim_practice_route.py")
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)


def sample(seq, elapsed, state=(0, 1, 0), **menu):
    c = {"latched": 1, "suppressNative": 1, "run": 3, "game.valid": 1,
         "game.inputWord": r.NEUTRAL, "game.simulationFrame": seq,
         "game.telemetry.speedMps": 0}
    c.update({f"game.state{k}": v for k, v in zip((54, 55, 56), state)})
    fields = dict(selection=0, training=1, transition=0, car=0, course=0, league=0,
                  deluxeCar=0, deluxeRival=0, deluxePanel=0, deluxeAccept=0, deluxeTrack=0, state5b=0)
    fields.update(menu)
    c.update({"game.menu." + k: v for k, v in fields.items()})
    return {"sequence": seq, "elapsedSeconds": elapsed, "channels": c}


class RouteChecks(unittest.TestCase):
    def ready(self, route, state=(0, 1, 0), **menu):
        seq = route.sequence + 1
        t = max(route.elapsed + .3, route.command_until + .3)
        route.observe(sample(seq, t, state, **menu))
        route.observe(sample(seq + 1, t + .2, state, **menu))
        return route.ready()

    def test_exact_route_and_player_proof(self):
        route = r.PracticeRoute()
        menu_steps = [((0, 1, 0), {}), ((0, 1, 0), {"selection": 1}),
                      ((1, 1, 0), {}), ((1, 1, 1), {"deluxePanel": 0x87}),
                      ((1, 1, 3), {"deluxePanel": 0x1f}),
                      ((1, 1, 4), {"deluxePanel": 0x1f}), ((2, 1, 1), {})]
        for state, menu in menu_steps:
            self.assertTrue(self.ready(route, state, **menu))
            self.assertEqual(len(route.sent()), 1)
        # A live-looking attract/race flag is insufficient without countdown.
        self.assertFalse(self.ready(route, (2, 3, 0)))
        route.observe(sample(route.sequence + 1, route.elapsed + .1, (2, 2, 1)))
        self.assertFalse(self.ready(route, (2, 3, 0)))
        route.observe(sample(route.sequence + 1, route.elapsed + .1, (2, 2, 2)))
        self.assertTrue(self.ready(route, (2, 3, 0)))
        self.assertEqual(route.sent(), ("inject action throttle 1 ms=2000",))
        self.assertFalse(self.ready(route, (2, 3, 0)))
        s = sample(route.sequence + 1, route.elapsed + .1, (2, 3, 0))
        s["channels"]["game.telemetry.speedMps"] = 2
        route.observe(s)
        self.assertTrue(route.ready())
        self.assertEqual(route.sent(), ("inject action throttle 1 ms=1000", "inject action steer 0.25 ms=1000"))
        self.assertTrue(self.ready(route, (2, 3, 0)))
        self.assertEqual(route.sent(), ("end",))
        self.assertTrue(route.countdown and route.moving)
        self.assertEqual(route.step, 10)

    def test_unexpected_attract_or_gp_stops(self):
        for state in ((2, 3, 0), (3, 4, 0), (1, 1, 2)):
            route = r.PracticeRoute()
            with self.assertRaises(r.Refused): route.observe(sample(0, 0, state))
        route = r.PracticeRoute(); route.step = 4
        with self.assertRaises(r.Refused): route.observe(sample(0, 0, (1, 1, 2)))

    def test_wrong_training_and_attract_flag_refused(self):
        for kwargs in ({"training": 0}, {"state5b": 1}):
            route = r.PracticeRoute(); route.step = 7
            with self.assertRaises(r.Refused): route.observe(sample(0, 0, (2, 3, 0), **kwargs))

    def test_no_action_on_wrong_selection_or_during_panel_fade(self):
        cases = [(0, (0, 1, 0), {"selection": 1}), (1, (0, 1, 0), {"selection": 0}),
                 (2, (1, 1, 0), {"deluxeCar": 1}), (3, (1, 1, 1), {"deluxePanel": 0x86}),
                 (3, (1, 1, 1), {"deluxePanel": 0x87, "deluxeAccept": 1}),
                 (4, (1, 1, 3), {"deluxeRival": 1, "deluxePanel": 0x1f}),
                 (5, (1, 1, 4), {"league": 1, "deluxePanel": 0x1f}),
                 (6, (2, 1, 1), {"course": 1}), (6, (2, 1, 1), {"deluxeTrack": 1}),
                 (0, (0, 1, 0), {"transition": 2})]
        for step, state, menu in cases:
            route = r.PracticeRoute(); route.step = step
            self.assertFalse(self.ready(route, state, **menu))
            with self.assertRaises(r.Refused): route.commands()

    def test_release_required_and_capture_recheck(self):
        route = r.PracticeRoute()
        s = sample(0, 0); s["channels"]["game.inputWord"] |= 8
        route.observe(s); self.assertFalse(route.ready())
        route.observe(sample(1, .1)); self.assertFalse(route.ready())
        route.observe(sample(2, .3)); self.assertTrue(route.ready())
        # Equivalent to a selection change while WGC captures the image.
        route.observe(sample(3, .4, selection=1)); self.assertFalse(route.ready())

    def test_source_identity_and_invalid_values(self):
        for key, value in (("latched", 0), ("suppressNative", 0), ("run", 4),
                           ("game.valid", 0), ("game.menu.training", float("nan")),
                           ("game.simulationFrame", 9)):
            route = r.PracticeRoute(); s = sample(0, 0); s["channels"][key] = value
            with self.assertRaises(r.Refused): route.observe(s)
        route = r.PracticeRoute(); route.observe(sample(0, 0))
        with self.assertRaises(r.Refused): route.observe(sample(0, .1))

    def test_exact_rom_pin(self):
        good = b"FZPT0001" + bytes.fromhex(r.ROM)
        r.verify_header(good)
        for bad in (b"", good[:-1], b"x" + good[1:], good[:-1] + b"\x00"):
            with self.assertRaises(r.Refused): r.verify_header(bad)

    def test_incremental_complete_lines_and_nonce(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "source.jsonl"
            reader = r.SourceReader(path, "nonce")
            self.assertEqual(reader.read(), [])
            meta = {"kind": "metadata", "metadata": {"game": "fzero-snes", "pluginVersion": "fzero-shim-diagnostic-3",
                    "startedUtc": "2026-10-12T10:00:00Z", "properties": {"runNonce": "nonce", "context": "diagnostic-injected-original; private"}}}
            row = json.dumps({"kind": "sample", "sample": sample(0, 0)})
            path.write_text(json.dumps(meta) + "\n" + row[:20])
            self.assertEqual(reader.read(), [])
            with path.open("a") as stream: stream.write(row[20:] + "\n")
            self.assertEqual(reader.read(), [sample(0, 0)])
            self.assertEqual(reader.read(), [])
            wrong = r.SourceReader(path, "other")
            with self.assertRaises(r.Refused): wrong.read()


if __name__ == "__main__": unittest.main()
