"""Focused package-time identity/contract checks; no helper/process execution."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import setup_bundle


class SetupBundleTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="fz-setup-bundle-")
        self.root = Path(self.temp.name)
        (self.root / "DbceModSetup.exe").write_bytes(b"inert-apphost")
        (self.root / "Wheelkit.dll").write_bytes(b"inert-dependency")
        self.manifest = {"schema": 1, "kind": "private-standalone-helper", "rid": "win-x64",
                         "selfContained": True, "sourceCommit": "1" * 40,
                         "supportedGames": ["f-zero"], "waitForProcessContract": setup_bundle.CONTRACT,
                         "waitForProcessTimeoutMs": 60000, "files": [
                             {"path": file.name, "bytes": file.stat().st_size,
                              "sha256": setup_bundle.digest(file.read_bytes())}
                             for file in sorted(self.root.iterdir())]}
        self.write(self.manifest)

    def tearDown(self):
        self.temp.cleanup()

    def write(self, manifest):
        (self.root / "bundle-manifest.json").write_text(json.dumps(manifest), encoding="utf-8")

    def test_exact_bundle_generates_only_bounded_runtime_pins(self):
        info = setup_bundle.admit(self.root)
        header = setup_bundle.header(info)
        self.assertIn(info["exeSha256"], header)
        self.assertIn(info["manifestSha256"], header)
        self.assertNotIn("Wheelkit.dll", header)
        self.assertIn("FZERO_SETUP_QUALIFIED 0", setup_bundle.header())

    def test_report_is_opt_in_and_old_bundles_keep_controls_only(self):
        self.assertFalse(setup_bundle.admit(self.root)["reportQualified"])
        self.assertIn("FZERO_SETUP_REPORT_QUALIFIED 0", setup_bundle.header(setup_bundle.admit(self.root)))
        self.manifest["report"] = {"contract": setup_bundle.REPORT_CONTRACT, "games": ["f-zero"], "browserDraftOnly": True}
        self.write(self.manifest)
        info = setup_bundle.admit(self.root)
        self.assertTrue(info["reportQualified"])
        self.assertIn("FZERO_SETUP_REPORT_QUALIFIED 1", setup_bundle.header(info))
        for key, value in (("contract", "unknown"), ("games", ["other"]), ("browserDraftOnly", False)):
            bad = copy.deepcopy(self.manifest); bad["report"][key] = value; self.write(bad)
            with self.assertRaises(ValueError): setup_bundle.admit(self.root)

    def test_contract_and_game_are_required(self):
        for key, value in (("supportedGames", ["art-of-rally"]), ("waitForProcessContract", "old"),
                           ("waitForProcessTimeoutMs", 0)):
            bad = copy.deepcopy(self.manifest); bad[key] = value; self.write(bad)
            with self.assertRaises(ValueError): setup_bundle.admit(self.root)

    def test_drift_and_unowned_files_refuse_at_package_time(self):
        (self.root / "Wheelkit.dll").write_bytes(b"different-bytes")
        with self.assertRaises(ValueError): setup_bundle.admit(self.root)
        (self.root / "Wheelkit.dll").write_bytes(b"inert-dependency")
        (self.root / "unowned.txt").write_text("extra", encoding="utf-8")
        with self.assertRaises(ValueError): setup_bundle.admit(self.root)

    def test_inventory_path_cannot_escape_or_duplicate(self):
        for name in ("../elsewhere", "C:/elsewhere", "a/../elsewhere", "DbceModSetup.exe"):
            bad = copy.deepcopy(self.manifest)
            bad["files"].append({**bad["files"][0], "path": name}); self.write(bad)
            with self.assertRaises(ValueError): setup_bundle.admit(self.root)

    def test_private_staging_requires_linked_exact_host_pins(self):
        info = setup_bundle.admit(self.root)
        with self.assertRaises(ValueError): setup_bundle.payload(self.root, b"unrelated-game")
        image = (info["manifestSha256"] + "\0" + info["exeSha256"] + "\0").encode("ascii")
        files, metadata = setup_bundle.payload(self.root, image)
        self.assertEqual(set(files), {"controls-setup/DbceModSetup.exe", "controls-setup/Wheelkit.dll",
                                      "controls-setup/bundle-manifest.json"})
        self.assertEqual(metadata["waitForProcessContract"], setup_bundle.CONTRACT)
        self.assertEqual(files["controls-setup/DbceModSetup.exe"], b"inert-apphost")


if __name__ == "__main__":
    unittest.main()
