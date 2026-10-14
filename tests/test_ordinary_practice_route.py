"""ROM-free identity and causal-action checks for the ordinary route driver."""
import importlib.util
import json
from pathlib import Path
import tempfile
import time
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location(
    "ordinary_route", Path(__file__).parents[1] / "tools/fzero_ordinary_practice_route.py")
route = importlib.util.module_from_spec(spec)
spec.loader.exec_module(route)


def witness(frame=0, state=(0, 1, 0), word=route.PORT, **menu):
    value = dict(valid=True, limited=False, simulationFrame=frame, inputWord=word,
                 state54=state[0], state55=state[1], state56=state[2],
                 selection=0, training=1, transition=0, car=0, course=0, league=0,
                 deluxeCar=0, deluxeRival=0, deluxePanel=0, deluxeAccept=0,
                 deluxeTrack=0, state5b=0, speedMps=0.0, distanceM=0.0)
    value.update(menu)
    return value


def status(frame=0, **changes):
    row = dict(schema="fzero.session-status@1", requestId="a" * 32, action="record",
               state="recording", processId=123, processCreationFileTime=456,
               frames=frame + 1, effectiveRomSha256=route.ROM, outputMuted=True,
               routeWitness=witness(frame))
    row.update(changes)
    return row


class FakeStream:
    def __init__(self, rows):
        self.rows = iter(rows)
    def next(self, deadline):
        try:
            return next(self.rows)
        except StopIteration as exc:
            raise route.Refused("no fresh simulation frame") from exc


class FakeKeyboard:
    def __init__(self):
        self.events = []
    def down(self, name):
        self.events.append(("down", name))
    def up(self, name):
        self.events.append(("up", name))


class OrdinaryRouteChecks(unittest.TestCase):
    def test_deluxe_rom_pin_is_independent(self):
        expected = "419eed641cfa20013d1e9b8270062d90155051a441441a2118bf23846ac8d747"
        self.assertEqual(len(expected), 64)
        self.assertEqual(route.ROM, expected)

    def stream(self, root):
        (root / "companion.json").write_text(json.dumps(dict(
            schema="fzero.companion-session@1", requestId="a" * 32,
            action="record", inputOrigin="automated", privateSaveRoot="C:/private",
            processId=123, processCreationFileTime=456)), encoding="utf-8")
        return route.StatusStream(root, "a" * 32)

    def test_identity_freshness_and_terminal_refusal(self):
        with tempfile.TemporaryDirectory() as folder:
            stream = self.stream(Path(folder))
            self.assertIsNone(stream.accept(status(0, state="starting")))
            first = stream.accept(status(0))
            self.assertEqual(first["simulationFrame"], 0)
            self.assertIsNone(stream.accept(status(0)))
            self.assertEqual(stream.accept(status(1))["simulationFrame"], 1)
            for changes in (dict(requestId="b" * 32), dict(processCreationFileTime=457),
                            dict(effectiveRomSha256="0" * 64), dict(outputMuted=False),
                            dict(state="saved"), dict(frames=9),
                            dict(routeWitness=dict(valid=False, limited=True))):
                with self.assertRaises(route.Refused):
                    stream.accept(status(2, **changes))
            late = self.stream(Path(folder))
            with self.assertRaises(route.Refused):
                late.accept(status(181))

    def test_duplicate_json_field_refused(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "status.json"
            path.write_text('{"requestId":"a","requestId":"b"}', encoding="utf-8")
            with self.assertRaises(route.Refused):
                route.read_json(path)

    def test_transient_status_miss_waits_for_next_row(self):
        with tempfile.TemporaryDirectory() as folder:
            stream = self.stream(Path(folder))
            with patch.object(route, "read_json", side_effect=[route.TransientStatus(), status(0)]):
                self.assertEqual(stream.next(time.monotonic() + 1)["simulationFrame"], 0)

    def test_practice_guards_reject_attract_and_wrong_menu(self):
        accepted = (
            (0, witness()),
            (1, witness(selection=1)),
            (2, witness(state=(1, 1, 0))),
            (3, witness(state=(1, 1, 1), deluxePanel=0x87)),
            (4, witness(state=(1, 1, 3), deluxePanel=0x1f)),
            (5, witness(state=(1, 1, 4), deluxePanel=0x1f)),
            (6, witness(state=(2, 1, 1))),
        )
        for step, row in accepted:
            self.assertTrue(route.ready(step, row), step)
        self.assertFalse(route.ready(1, witness()))
        self.assertFalse(route.ready(2, witness(state=(1, 1, 0), training=0)))
        self.assertTrue(route.ready(2, witness(state=(1, 1, 0), state5b=1)))
        self.assertFalse(route.ready(3, witness(state=(1, 1, 1), deluxePanel=0x86)))
        live = witness(state=(2, 3, 0), speedMps=3.0)
        self.assertFalse(route.ready(7, live))
        self.assertTrue(route.ready(7, live, countdown=True))
        self.assertTrue(route.ready(8, live, countdown=True, moving=True))
        self.assertFalse(route.ready(8, live, countdown=True, moving=False))
        with self.assertRaises(route.Refused):
            route.require_state(7, witness(state=(2, 3, 0), training=0))
        with self.assertRaises(route.Refused):
            route.require_state(1, witness(state=(2, 3, 0)))

    def test_down_requires_consumed_word_and_result_transition(self):
        rows = [witness(i) for i in range(9)]
        rows += [witness(i, word=route.PORT | route.DOWN) for i in range(9, 15)]
        rows += [witness(i, selection=1) for i in range(15, 26)]
        keyboard = FakeKeyboard()
        result = route.pulse(FakeStream(rows), keyboard, "Down", 0, 1,
                             time.monotonic() + 2)
        self.assertEqual(result["selection"], 1)
        self.assertEqual(keyboard.events, [("down", "Down"), ("up", "Down")])
        missing = [witness(i) for i in range(9)] + [witness(i) for i in range(9, 18)]
        keyboard = FakeKeyboard()
        with self.assertRaises(route.Refused):
            route.pulse(FakeStream(missing), keyboard, "Down", 0, 1,
                        time.monotonic() + 2)
        self.assertEqual(keyboard.events, [("down", "Down"), ("up", "Down")])
        wrong_state = [witness(i) for i in range(9)] + [witness(9, (2, 3, 0), route.PORT | route.DOWN)]
        keyboard = FakeKeyboard()
        with self.assertRaises(route.Refused):
            route.pulse(FakeStream(wrong_state), keyboard, "Down", 0, 1,
                        time.monotonic() + 2)
        self.assertEqual(keyboard.events, [("down", "Down"), ("up", "Down")])


if __name__ == "__main__":
    unittest.main()
