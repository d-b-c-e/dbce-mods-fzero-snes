"""Windows PS5.1 helper check against a harmless argv/exit-code fixture, not a game."""
import base64
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
POWERSHELL = Path(os.environ.get("WINDIR", "C:/Windows")) / "System32/WindowsPowerShell/v1.0/powershell.exe"
CSC = Path(os.environ.get("WINDIR", "C:/Windows")) / "Microsoft.NET/Framework64/v4.0.30319/csc.exe"


@unittest.skipUnless(os.name == "nt" and POWERSHELL.is_file() and CSC.is_file(), "Windows PS5.1 and .NET fixture compiler required")
class RecordingHelperTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.base = Path(cls.temp.name)
        cls.app = cls.base / "candidate & [one]"
        (cls.app / "tools").mkdir(parents=True)
        shutil.copyfile(ROOT / "tools/Recording.ps1", cls.app / "tools/Recording.ps1")
        source = cls.base / "Fixture.cs"
        source.write_text('using System; using System.Text; class Fixture { static int Main(string[] args) { '
                          'Console.WriteLine("CWD:"+Convert.ToBase64String(Encoding.UTF8.GetBytes(Environment.CurrentDirectory))); '
                          'foreach(string arg in args) Console.WriteLine("ARG:"+Convert.ToBase64String(Encoding.UTF8.GetBytes(arg))); '
                          'return 17; } }')
        compiled = subprocess.run([str(CSC), "/nologo", "/target:exe", "/out:" + str(cls.app / "FZeroSNESRecomp.exe"), str(source)],
                                  capture_output=True, text=True)
        if compiled.returncode:
            raise RuntimeError(compiled.stdout + compiled.stderr)
        cls.caller = cls.base / "caller (recordings)"
        cls.caller.mkdir()
        cls.rom = cls.caller / "own $rom & [USA].sfc"
        cls.rom.write_bytes(b"fixture only, no ROM")
        cls.config = cls.app / "config.ini"
        cls.config.write_bytes(b"[Input]\r\nBackend=Native\r\n")

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def invoke(self, action, tape):
        result = subprocess.run([str(POWERSHELL), "-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                                 str(self.app / "tools/Recording.ps1"), action,
                                 "-Rom", self.rom.name, "-Tape", tape], cwd=self.caller,
                                capture_output=True, text=True, timeout=20)
        args = [base64.b64decode(line[4:]).decode() for line in result.stdout.splitlines() if line.startswith("ARG:")]
        child_cwd = [base64.b64decode(line[4:]).decode() for line in result.stdout.splitlines() if line.startswith("CWD:")]
        if args:
            self.assertEqual(child_cwd, [str(self.app)])
        self.assertEqual(self.config.read_bytes(), b"[Input]\r\nBackend=Native\r\n")
        return result, args

    def test_record_arguments_paths_and_exit_are_preserved(self):
        tape = "fresh $tape & [one].fzpt"
        result, args = self.invoke("Record", tape)
        self.assertEqual(result.returncode, 17, result.stderr)
        self.assertEqual(args, ["--record", str(self.caller / tape), str(self.rom)])
        self.assertFalse((self.caller / tape).exists())

    def test_replay_pair_and_nonzero_native_exit_are_preserved(self):
        tape = self.caller / "replay [one].fzpt"
        tape.write_bytes(b"fixture")
        Path(str(tape) + ".state").write_bytes(b"fixture state")
        result, args = self.invoke("Replay", tape.name)
        self.assertEqual(result.returncode, 17, result.stderr)
        self.assertEqual(args, ["--replay", str(tape), str(self.rom)])
        self.assertEqual(tape.read_bytes(), b"fixture")

    def test_record_existing_tape_or_state_refuses_without_invocation(self):
        tape = self.caller / "existing.fzpt"
        for path in (tape, Path(str(tape) + ".state")):
            path.write_bytes(b"retain")
            result, args = self.invoke("Record", tape.name)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(args, [])
            self.assertEqual(path.read_bytes(), b"retain")
            path.unlink()

    def test_missing_pair_and_missing_folder_refuse_without_invocation(self):
        for action, tape in (("Replay", "missing.fzpt"), ("Record", "absent/fresh.fzpt")):
            result, args = self.invoke(action, tape)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(args, [])


if __name__ == "__main__":
    unittest.main()
