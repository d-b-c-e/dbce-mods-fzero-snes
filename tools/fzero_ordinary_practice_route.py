"""Private, bounded synthetic-key route for an isolated ordinary recording.

This reads the existing session status witness. It neither writes WRAM nor
creates an input API or recording. Run only against a disposable, muted copy.
"""
import argparse
import ctypes
from ctypes import wintypes
import json
from pathlib import Path
import stat
import sys
import time


ROM = "419eed641cfa20013d1e9b8270062d90155051a441441a2118bf23846ac8d747"
PORT = 1 << 30
DOWN, A, B, RIGHT = 1 << 5, 1 << 8, 1 << 0, 1 << 7
KEYS = {"Down": (0x28, DOWN), "X": (0x58, A), "Z": (0x5A, B), "Right": (0x27, RIGHT)}
SCAN_CODES = {"Down": (0x50, True), "X": (0x2d, False),
              "Z": (0x2c, False), "Right": (0x4d, True)}
MAX_FRAMES = 6000
ALLOWED = (
    {(0, 0, 0), (0, 1, 0)},
    {(0, 1, 0)},
    {(0, 1, 0), (1, 0, 0), (1, 1, 0)},
    {(1, 1, 0), (1, 1, 1)},
    {(1, 1, 1), (1, 1, 3)},
    {(1, 1, 3), (1, 1, 4)},
    {(1, 1, 4), (1, 7, 4), (2, 1, 0), (2, 1, 1)},
    {(2, 1, 1), (2, 2, 0), (2, 2, 1), (2, 2, 2), (2, 3, 0)},
    {(2, 3, 0)},
)


class Refused(RuntimeError):
    pass


class ProcessGone(Refused):
    """The original request process cannot retain an SDL key anymore."""


class TransientStatus(Exception):
    """An atomic status replacement was between names or briefly locked."""


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise Refused("duplicate session JSON field")
        result[key] = value
    return result


def read_json(path, transient=False):
    try:
        if path.is_symlink():
            raise Refused("linked session JSON")
        info = path.stat()
        if not stat.S_ISREG(info.st_mode) or info.st_size > 65536:
            raise Refused("non-file or oversized session JSON")
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        if transient:
            raise TransientStatus from exc
        raise Refused("missing or locked session JSON") from exc
    try:
        return json.loads(text, object_pairs_hook=unique_object)
    except (UnicodeError, ValueError) as exc:
        raise Refused("malformed session JSON") from exc


class StatusStream:
    def __init__(self, result, request_id, expected_rom=ROM):
        self.result = Path(result).resolve(strict=True)
        self.request_id = request_id
        self.expected_rom = expected_rom
        self.launch = read_json(self.result / "companion.json")
        if (self.launch.get("schema") != "fzero.companion-session@1" or
                self.launch.get("requestId") != request_id or self.launch.get("action") != "record" or
                self.launch.get("inputOrigin") != "automated" or not self.launch.get("privateSaveRoot")):
            raise Refused("not this automated isolated recording")
        self.pid = self.launch["processId"]
        self.created = self.launch["processCreationFileTime"]
        self.last_frame = -1
        self.last_new = time.monotonic()
        self.intervals_ms = []
        self.guard = lambda: None

    def accept(self, status):
        if (status.get("schema") != "fzero.session-status@1" or
                status.get("requestId") != self.request_id or
                status.get("processId") != self.pid or
                status.get("processCreationFileTime") != self.created or
                status.get("effectiveRomSha256") != self.expected_rom or
                status.get("action") != "record" or
                status.get("outputMuted") is not True):
            raise Refused("foreign or unmuted session status")
        if status.get("state") == "starting":
            return None
        if status.get("state") != "recording":
            raise Refused("ordinary recording ended")
        witness = status.get("routeWitness")
        if (not isinstance(witness, dict) or witness.get("valid") is not True or
                witness.get("limited") is not False):
            raise Refused("route witness unavailable or capped")
        frame = witness.get("simulationFrame")
        tape_frames = status.get("frames")
        word = witness.get("inputWord")
        if (not isinstance(frame, int) or isinstance(frame, bool) or
                not isinstance(tape_frames, int) or isinstance(tape_frames, bool) or
                not isinstance(word, int) or isinstance(word, bool) or
                frame < 0 or frame >= MAX_FRAMES or tape_frames != frame + 1 or
                word < 0 or word > 0xffffffff or not (word & PORT)):
            raise Refused("witness is not the matching recorded input frame")
        discrete = ("state54", "state55", "state56", "selection", "training", "transition",
                    "car", "course", "league", "deluxeCar", "deluxeRival", "deluxePanel",
                    "deluxeAccept", "deluxeTrack", "state5b")
        for field in discrete:
            value = witness.get(field)
            if not isinstance(value, int) or isinstance(value, bool) or value < 0:
                raise Refused("invalid route field: " + field)
        for field in ("speedMps", "distanceM"):
            value = witness.get(field)
            if (not isinstance(value, (int, float)) or isinstance(value, bool) or
                    value != value or value < 0 or value == float("inf")):
                raise Refused("invalid route field: " + field)
        if frame <= self.last_frame:
            return None
        if self.last_frame < 0 and frame > 180:
            raise Refused("route attached after the early title frames")
        now = time.monotonic()
        if self.last_frame >= 0:
            self.intervals_ms.append((now - self.last_new) * 1000 / (frame - self.last_frame))
        self.last_frame = frame
        self.last_new = now
        return witness

    def timing(self):
        values = sorted(self.intervals_ms)
        if not values:
            return {"observedStatusFrameIntervals": 0}
        return {"observedStatusFrameIntervals": len(values),
                "observedStatusP95Ms": values[(len(values) * 95 - 1) // 100],
                "observedStatusP99Ms": values[(len(values) * 99 - 1) // 100],
                "observedStatusMaxMs": values[-1]}

    def next(self, deadline):
        while time.monotonic() < deadline:
            self.guard()
            path = self.result / "status.json"
            try:
                document = read_json(path, transient=True)
            except TransientStatus:
                document = None
            if document is not None:
                row = self.accept(document)
                if row is not None:
                    return row
            if self.last_frame >= 0 and time.monotonic() - self.last_new > .5:
                raise Refused("stale simulation witness")
            time.sleep(.01)
        raise Refused("route deadline exceeded")


def state(row):
    return tuple(row["state" + str(i)] for i in (54, 55, 56))


def practice(row):
    # $5B is retained as evidence, but its attract value has not been pinned.
    return row["training"] == 1


def require_state(step, row):
    s = state(row)
    if s not in ALLOWED[step] or (s[0] != 0 and not practice(row)):
        raise Refused("unexpected or non-Practice state at step " + str(step))


def ready(step, row, countdown=False, moving=False):
    s = state(row)
    m = row
    if m["transition"] != 0:
        return False
    if step == 0:
        return s == (0, 1, 0) and m["selection"] == 0
    if step == 1:
        return s == (0, 1, 0) and m["selection"] == 1
    if not practice(row):
        return False
    if step == 2:
        return s == (1, 1, 0) and m["deluxeCar"] == m["deluxePanel"] == m["deluxeAccept"] == 0
    if step == 3:
        return s == (1, 1, 1) and m["deluxeCar"] == m["deluxeAccept"] == 0 and m["deluxePanel"] == 0x87
    if step == 4:
        return s == (1, 1, 3) and m["deluxeCar"] == m["deluxeRival"] == 0 and m["deluxePanel"] == 0x1f
    if step == 5:
        return s == (1, 1, 4) and m["league"] == m["deluxeRival"] == 0 and m["deluxePanel"] == 0x1f
    course = m["car"] == m["league"] == m["course"] == m["deluxeTrack"] == 0
    if step == 6:
        return s == (2, 1, 1) and course
    if step == 7:
        return s == (2, 3, 0) and course and countdown
    return s == (2, 3, 0) and course and countdown and moving


class Keyboard:
    """Post bounded keys only to the exact owned SDL game window."""
    def __init__(self, pid, created, exe, window_wait=10):
        if sys.platform != "win32":
            raise Refused("synthetic keyboard route is Windows-only")
        self.user = ctypes.WinDLL("user32", use_last_error=True)
        self.kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        self.user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
        self.user.IsWindowVisible.argtypes = [wintypes.HWND]
        self.user.GetClassNameW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        self.user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
        self.user.MapVirtualKeyW.argtypes = [wintypes.UINT, wintypes.UINT]
        self.user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
        self.kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
        self.kernel.OpenProcess.restype = wintypes.HANDLE
        self.kernel.GetProcessTimes.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p,
                                                ctypes.c_void_p, ctypes.c_void_p]
        self.kernel.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
        self.kernel.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD,
                                                           wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)]
        self.kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        self.pid, self.created = pid, created
        try:
            self.exe = str(Path(exe).resolve(strict=True)).casefold()
        except (OSError, TypeError) as exc:
            raise Refused("owned game path is unavailable") from exc
        self.held = set()
        self._process_owned()
        window_deadline = time.monotonic() + window_wait
        while True:
            try:
                self.window = self._window()
                break
            except Refused:
                self._process_owned()
                if time.monotonic() >= window_deadline:
                    raise
                time.sleep(.05)
        self.check()

    def _process_owned(self):
        handle = self.kernel.OpenProcess(0x1000, False, self.pid)
        if not handle:
            if ctypes.get_last_error() == 87:  # ERROR_INVALID_PARAMETER: no such PID
                raise ProcessGone("owned game process is gone")
            raise Refused("owned game process is gone")
        try:
            created = wintypes.FILETIME()
            exited = wintypes.FILETIME()
            kernel = wintypes.FILETIME()
            user = wintypes.FILETIME()
            if not self.kernel.GetProcessTimes(handle, ctypes.byref(created), ctypes.byref(exited),
                                               ctypes.byref(kernel), ctypes.byref(user)):
                raise Refused("cannot verify owned process creation")
            actual = (created.dwHighDateTime << 32) | created.dwLowDateTime
            if actual != self.created:
                raise ProcessGone("process ID was reused")
            exit_code = wintypes.DWORD()
            if not self.kernel.GetExitCodeProcess(handle, ctypes.byref(exit_code)):
                raise Refused("cannot verify owned process exit state")
            if exit_code.value != 259:
                raise ProcessGone("owned game process exited")
            image = ctypes.create_unicode_buffer(32768)
            size = wintypes.DWORD(len(image))
            if not self.kernel.QueryFullProcessImageNameW(handle, 0, image, ctypes.byref(size)):
                raise Refused("cannot verify owned process path")
            try:
                actual_path = str(Path(image.value).resolve(strict=True)).casefold()
            except OSError as exc:
                raise Refused("owned game process path is unavailable") from exc
            if actual_path != self.exe:
                raise Refused("owned game process path changed")
        finally:
            self.kernel.CloseHandle(handle)

    def _window(self):
        found = []
        callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        def visit(hwnd, unused):
            pid = wintypes.DWORD()
            self.user.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
            if pid.value == self.pid and self.user.IsWindowVisible(hwnd):
                kind = ctypes.create_unicode_buffer(256)
                title = ctypes.create_unicode_buffer(256)
                if (self.user.GetClassNameW(hwnd, kind, len(kind)) and
                        self.user.GetWindowTextW(hwnd, title, len(title)) and
                        kind.value == "SDL_app" and title.value.startswith("F-Zero")):
                    found.append(hwnd)
            return True
        self.user.EnumWindows(callback(visit), 0)
        if len(found) != 1:
            raise Refused("expected one visible owned game window")
        return found[0]

    def check(self):
        self._process_owned()
        if self._window() != self.window:
            raise Refused("owned SDL game window changed during route")

    def _send(self, name, up=False):
        self.check()
        vk = KEYS[name][0]
        scan = self.user.MapVirtualKeyW(vk, 4)  # MAPVK_VK_TO_VSC_EX
        low, extended = SCAN_CODES[name]
        # This Windows installation returns 0x50/0x4d, not E0 50/E0 4d,
        # for the arrows. SDL maps the plain values to keypad 2/6. Require
        # the expected physical scan, then encode the arrow E0 bit explicitly.
        if scan not in ((low, 0xe000 | low) if extended else (low,)):
            raise Refused("keyboard scan code unavailable or unexpected")
        # WM_KEY*: repeat=1, scan in bits 16-23, E0 in bit 24;
        # key-up sets previous-state and transition bits.
        lparam = 1 | (low << 16) | (0x1000000 if extended else 0)
        if up:
            lparam |= 0xc0000000
        if not self.user.PostMessageW(self.window, 0x101 if up else 0x100, vk, lparam):
            raise Refused("PostMessageW refused owned game key edge")

    def down(self, name):
        self.check()
        self._send(name)
        self.held.add(name)

    def up(self, name):
        if name in self.held:
            self._send(name, True)
            self.held.remove(name)

    def close(self):
        errors = []
        for name in list(self.held):
            try:
                self.up(name)
            except Refused as exc:
                errors.append(exc)
        if errors:
            raise errors[0]

    @staticmethod
    def release_fixed(result, request_id):
        """Runner cleanup: four key-ups to this request's live owned SDL HWND."""
        stream = StatusStream(result, request_id)
        try:
            sender = Keyboard(stream.pid, stream.created, stream.launch.get("exe"), window_wait=0)
        except ProcessGone as exc:
            print(json.dumps({"releaseOnly": "no-op", "reason": str(exc)}), flush=True)
            return
        errors = []
        for name in ("Down", "X", "Z", "Right"):
            try:
                sender._send(name, True)
            except Refused as exc:
                errors.append(exc)
        if errors:
            raise errors[0]


def wait_for(stream, predicate, deadline, key=None, pressed=False, minimum_frames=1, step=None):
    count = 0
    while True:
        row = stream.next(deadline)
        if step is not None:
            require_state(step, row)
        if key is not None:
            bit = KEYS[key][1]
            if bool(row["inputWord"] & bit) != pressed:
                count = 0
                continue
        if predicate(row):
            count += 1
            if count >= minimum_frames:
                return row
        else:
            count = 0


def pulse(stream, keyboard, name, current, next_step, deadline, next_ready=None):
    bit = KEYS[name][1]
    wait_for(stream, lambda r: ready(current, r), deadline, name, False, 9, current)
    keyboard.down(name)
    try:
        first = None
        hold_deadline = min(deadline, time.monotonic() + .35)
        while True:
            seen = stream.next(hold_deadline)
            require_state(current + 1, seen)
            if not (seen["inputWord"] & bit):
                if first is not None:
                    raise Refused("consumed key bit dropped while held")
                continue
            if first is None:
                first = seen["simulationFrame"]
            if seen["simulationFrame"] >= first + 5:
                break
    finally:
        keyboard.up(name)
    wait_for(stream, lambda r: not (r["inputWord"] & bit), deadline, name, False, 9, current + 1)
    result = wait_for(stream, next_ready or (lambda r: ready(next_step, r)),
                      deadline, minimum_frames=2, step=current + 1)
    print(json.dumps({"step": next_step, "pressedFrame": seen["simulationFrame"],
                      "resultFrame": result["simulationFrame"], "state": state(result)}), flush=True)
    return result


def drive(stream, keyboard, deadline):
    row = wait_for(stream, lambda r: ready(0, r), deadline, minimum_frames=9, step=0)
    print(json.dumps({"step": 0, "frame": row["simulationFrame"]}), flush=True)
    row = pulse(stream, keyboard, "Down", 0, 1, deadline)
    for step in range(1, 6):
        row = pulse(stream, keyboard, "X", step, step + 1, deadline)
    row = pulse(stream, keyboard, "X", 6, 7, deadline,
                next_ready=lambda r: practice(r) and state(r) in ((2, 2, 0), (2, 2, 1), (2, 2, 2)))
    # This exact player countdown was observed in the diagnostic Practice path;
    # live-looking attract frames alone never admit throttle.
    countdown = wait_for(stream, lambda r: practice(r) and state(r) == (2, 2, 2), deadline, step=7)
    row = wait_for(stream, lambda r: ready(7, r, countdown=True), deadline, step=7)
    print(json.dumps({"countdownFrame": countdown["simulationFrame"],
                      "liveFrame": row["simulationFrame"]}), flush=True)
    wait_for(stream, lambda r: ready(7, r, countdown=True), deadline, "Z", False, 9, 8)
    keyboard.down("Z")
    try:
        first = wait_for(stream, lambda r: ready(7, r, countdown=True), deadline, "Z", True, 2, 8)
        moving = wait_for(stream, lambda r: ready(8, r, countdown=True, moving=True) and
                          r["speedMps"] > 1 and r["distanceM"] > 0 and
                          r["simulationFrame"] >= first["simulationFrame"] + 110,
                          deadline, "Z", True, step=8)
    finally:
        keyboard.up("Z")
    wait_for(stream, lambda r: ready(8, r, countdown=True, moving=True), deadline, "Z", False, 9, 8)
    wait_for(stream, lambda r: ready(8, r, countdown=True, moving=True), deadline, "Right", False, 9, 8)
    baseline = moving["distanceM"]
    keyboard.down("Z")
    keyboard.down("Right")
    try:
        combo = wait_for(stream, lambda r: (r["inputWord"] & (B | RIGHT)) == (B | RIGHT),
                         deadline, minimum_frames=2, step=8)
        steered = wait_for(stream, lambda r: ready(8, r, countdown=True, moving=True) and
                           r["distanceM"] > baseline and r["simulationFrame"] >= combo["simulationFrame"] + 55 and
                           (r["inputWord"] & (B | RIGHT)) == (B | RIGHT), deadline, step=8)
    finally:
        keyboard.up("Right")
        keyboard.up("Z")
    wait_for(stream, lambda r: ready(8, r, countdown=True, moving=True), deadline, "Z", False, 9, 8)
    print(json.dumps({"result": "player Practice driving", "frame": steered["simulationFrame"],
                      "speedMps": steered["speedMps"], "distanceM": steered["distanceM"]}), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--result")
    parser.add_argument("--request-id")
    parser.add_argument("--seconds", type=int, default=75)
    parser.add_argument("--release-only", action="store_true",
                        help="after driver exit, send only fixed Down/X/Z/Right key-ups")
    args = parser.parse_args()
    if not args.result or not args.request_id:
        raise Refused("route requires one result folder and request ID")
    if (len(args.request_id) != 32 or any(c not in "0123456789abcdef" for c in args.request_id) or
            not 30 <= args.seconds <= 100):
        raise Refused("invalid bounded route request")
    if args.release_only:
        Keyboard.release_fixed(args.result, args.request_id)
        return
    stream = StatusStream(args.result, args.request_id)
    first = stream.next(time.monotonic() + 15)
    require_state(0, first)  # Attach early, before the bounded window wait.
    keyboard = Keyboard(stream.pid, stream.created, stream.launch.get("exe"))
    stream.last_new = time.monotonic()  # Window startup is not status-frame pacing.
    stream.guard = keyboard.check
    try:
        drive(stream, keyboard, time.monotonic() + args.seconds)
    finally:
        keyboard.close()
        print(json.dumps({"framePacing": stream.timing()}), flush=True)


if __name__ == "__main__":
    try:
        main()
    except Refused as exc:
        print("route refused: " + str(exc), file=sys.stderr)
        sys.exit(2)
