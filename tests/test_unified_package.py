"""Device-free package boundary tests using synthetic x64 PE fixtures."""
import copy
import importlib.util
import io
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile

SPEC = importlib.util.spec_from_file_location("package_unified", Path(__file__).resolve().parents[1] / "tools/package_unified.py")
pack = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(pack)
sys.modules["package_unified"] = pack
STAGE_SPEC = importlib.util.spec_from_file_location("stage_unified", pack.ROOT / "tools/stage_unified.py")
stage = importlib.util.module_from_spec(STAGE_SPEC)
STAGE_SPEC.loader.exec_module(stage)


def pe(imports=()):
    data = bytearray(2048)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 60, 128)
    data[128:132] = b"PE\0\0"
    struct.pack_into("<HH", data, 132, 0x8664, 1)
    struct.pack_into("<H", data, 148, 240)
    struct.pack_into("<H", data, 152, 0x20b)
    struct.pack_into("<I", data, 260, 16)
    struct.pack_into("<IIII", data, 400, 1536, 4096, 1536, 512)
    if imports:
        struct.pack_into("<II", data, 272, 4096, (len(imports) + 1) * 20)
        for i, name in enumerate(imports):
            start = 768 + i * 128
            struct.pack_into("<IIIII", data, 512 + i * 20, 1, 0, 0, 4096 + start - 512, 1)
            raw = name.encode() + b"\0"
            data[start:start + len(raw)] = raw
    return bytes(data)


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.payload = self.base / "payload"
        self.payload.mkdir()
        self.root = self.base / "source"
        self.root.mkdir()
        self.files = {"FZeroSNESRecomp.exe": pe(["KERNEL32.dll", "SDL3.dll"]),
                      "FZeroSNESRecompHeadless.exe": pe(),
                      "WheelFfb.dll": pe(["dinput8.dll"]), "SDL3.dll": pe(),
                      "assets/shaders/basic.glsl": b"fixture shader"}
        for name in pack.UI_ASSETS:
            self.files[name] = b"Fixture runtime asset"
            if name.endswith(".ttf"):
                self.files["licenses/" + Path(name).stem + ".txt"] = b"Fixture font notice"
                if Path(name).name in pack.OFL_FILES:
                    license_path, _ = pack.OFL_FILES[Path(name).name]
                    self.files["licenses/" + Path(name).stem + ".txt"] += pack.OFL_SEPARATOR + (pack.ROOT / license_path).read_bytes()
        for name in ("snesrecomp", "recomp-ui", "imgui", "wheel-toolkit"):
            self.files[f"licenses/{name}.txt"] = b"Fixture notice"
        for name in pack.IMAGE_NOTICES:
            self.files[name] = b'Fixture asset notice'
        self.files['licenses/flag-font-OFL.txt'] = (pack.ROOT / pack.OFL_FILES['NotoSansSymbols2-Regular.ttf'][0]).read_bytes()
        self.wheel_hash = pack.digest(self.files["WheelFfb.dll"])
        self.receipt = {"schema": "dbce.fzero-build", "version": 1,
                        "productVersion": "1.7.0", "sourceRevision": "1" * 40,
                        "sourceTree": "2" * 40, "dirty": False, "bsDeluxe": False,
                        "dependencies": {"snesrecomp": "3" * 40, "recomp-ui": "4" * 40}}
        self.write_payload()

    def write_payload(self):
        for name, data in self.files.items():
            path = self.payload / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        self.receipt["files"] = {n: pack.digest(d) for n, d in self.files.items()}

    def read(self):
        return pack.read_payload(self.payload, self.receipt, "1.7.0", self.wheel_hash)

    def source(self):
        for name in pack.SOURCE_FILES:
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("Fixture source\n")
        (self.root / "VERSION").write_text("1.7.0\n")
        product = json.loads((pack.ROOT / 'game-product.json').read_text())
        product['upstreamBaseVersion'] = '1.7.0'
        (self.root / "game-product.json").write_text(json.dumps(product))
        (self.root / "lib/toolkit/MANIFEST.txt").write_text(self.wheel_hash + "  native/WheelFfb.dll\n")
        for name in pack.DOCS:
            path = self.root / "docs" / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("Fixture documentation\n")
        path = self.root / "assets/shaders/basic.glsl"
        path.parent.mkdir(parents=True)
        path.write_bytes(self.files["assets/shaders/basic.glsl"])
        self.git("init", "-q")
        self.git("add", ".")
        for name, pin in self.receipt["dependencies"].items():
            self.git("update-index", "--add", "--cacheinfo", "160000," + pin + "," + name)
            (self.root / name).mkdir()
        self.git("-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid", "commit", "-qm", "fixture")
        self.receipt["sourceRevision"] = self.git("rev-parse", "HEAD")
        self.receipt["sourceTree"] = self.git("rev-parse", "HEAD^{tree}")

    def git(self, *args):
        return subprocess.check_output(["git", "-C", str(self.root), *args], text=True, stderr=subprocess.DEVNULL).strip()

    def test_complete_runtime_and_dynamic_wheel(self):
        self.assertEqual(self.read(), self.files)

    def test_private_shared_receipt_and_runtime_are_not_public_payload(self):
        for value in (None, False, {}, {"distribution": "private-only"}):
            self.receipt["privateSharedInput"] = value
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, "Private shared-input receipt"):
                self.read()
        del self.receipt["privateSharedInput"]
        for name in ("dbce_shim.dll", "DBCE_SHIM.DLL"):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, "Private shared-input runtime"):
                pack.approved(name)
        self.files["dbce_shim.dll"] = pe()
        self.write_payload()
        with self.assertRaisesRegex(ValueError, "Private shared-input runtime"):
            self.read()

    def test_private_staging_pins_exact_header_dll_and_keeps_provenance(self):
        vendor = self.root / "vendor/dbce-shim"
        vendor.mkdir(parents=True)
        build = self.base / "private-build"
        build.mkdir()
        dll, header, notice = pe(["dinput8.dll"]), b"ABI2 fixture", b"Private only. No distribution grant."
        (build / "dbce_shim.dll").write_bytes(dll)
        (vendor / "dbce_shim.h").write_bytes(header)
        (vendor / "PROVENANCE.md").write_bytes(notice)
        pin = {"schema": "dbce.fzero-shared-input-pin", "version": 1, "abi": 2,
               "sourceRevision": "5" * 40, "distribution": "private-only",
               "files": {"dbce_shim.dll": pack.digest(dll), "dbce_shim.h": pack.digest(header)}}
        (vendor / "PIN.json").write_text(json.dumps(pin))
        files, receipt = stage.private_shared_files(build, self.root)
        self.assertEqual(files["dbce_shim.dll"], dll)
        self.assertEqual(files["shared-input/dbce_shim.h"], header)
        self.assertEqual(files["shared-input/PROVENANCE.md"], notice)
        self.assertEqual(json.loads(files["shared-input/PIN.json"]), pin)
        self.assertEqual(receipt["distribution"], "private-only")
        self.assertEqual(receipt["runtimeSha256"], pack.digest(dll))
        self.assertEqual(receipt["controls"]["consumer"], "FZeroSNESRecomp.exe")
        self.assertEqual(receipt["controls"]["optIn"]["default"], "Native")
        for path, original in ((build / "dbce_shim.dll", dll), (vendor / "dbce_shim.h", header)):
            path.write_bytes(original + b"changed")
            with self.subTest(path=path.name), self.assertRaisesRegex(ValueError, "differ from source pin"):
                stage.private_shared_files(build, self.root)
            path.write_bytes(original)
        (build / "dbce_shim.dll").unlink()
        with self.assertRaises(FileNotFoundError):
            stage.private_shared_files(build, self.root)

    def test_private_controls_contract_matches_production_mapper(self):
        source = (pack.ROOT / "src/fzero_shim_map.c").read_text()
        actions_block = source.split("kFzeroShimActions[DBCE_SHIM_ACTIONS] = {", 1)[1].split("};", 1)[0]
        actions = re.findall(r'"([^"\\]+)"', actions_block)
        unsupported_block = source.split("const char kFzeroShimUnsupported[] =", 1)[1].split(";", 1)[0]
        text = "".join(json.loads('"' + value + '"') for value in re.findall(r'"((?:[^"\\]|\\.)*)"', unsupported_block))
        unsupported = dict(line.split("=", 1) for line in text.splitlines())
        contract = stage.shared_controls_contract()
        self.assertEqual(contract["unsupported"], unsupported)
        self.assertEqual(len(contract["delivered"]), 11)
        self.assertEqual(set(contract["delivered"]), set(actions) - set(unsupported))
        self.assertEqual(contract["requires"], {"directRom": True, "forceFeedbackDisabled": True})
        self.assertEqual(contract["profile"], "config.ini")
        self.assertEqual(contract["settings"], "config.ini")

    def test_private_payload_receipt_reuses_outer_inventory_without_self_hash(self):
        folder = self.base / "receipts"
        (folder / "payload").mkdir(parents=True)
        receipt = {"schema": "dbce.fzero-build", "version": 1,
                   "files": {"FZeroSNESRecomp.exe": "a" * 64},
                   "privateSharedInput": {"controls": stage.shared_controls_contract()}}
        stage.write_receipts(folder, receipt)
        self.assertEqual((folder / "build-receipt.json").read_bytes(),
                         (folder / "payload/build-receipt.json").read_bytes())
        self.assertNotIn("build-receipt.json", json.loads((folder / "build-receipt.json").read_text())["files"])
        (folder / "payload/build-receipt.json").unlink()
        del receipt["privateSharedInput"]
        stage.write_receipts(folder, receipt)
        self.assertFalse((folder / "payload/build-receipt.json").exists())

    def test_shared_capability_requires_matching_linked_consumer_not_only_configured_header(self):
        stamp = "v1.8.3-200-g0123456789ab"
        stage.validate_shared_consumer(stamp.encode() + b"\0[fzero-shim]", stamp)
        for image in (stamp.encode() + b"-dirty\0[fzero-shim]", b"old-build\0[fzero-shim]",
                      stamp.encode() + b"\0", stamp.encode() + b"[fzero-shim]"):
            with self.subTest(image=image), self.assertRaisesRegex(ValueError, "linked source stamp"):
                stage.validate_shared_consumer(image, stamp)

    def test_private_pin_agrees_with_compiled_pin_and_no_public_grant(self):
        vendor = pack.ROOT / "vendor/dbce-shim"
        pin = json.loads((vendor / "PIN.json").read_text())
        self.assertEqual(pin["distribution"], "private-only")
        self.assertEqual(pin["files"]["dbce_shim.h"], pack.digest((vendor / "dbce_shim.h").read_bytes()))
        self.assertIn(pin["files"]["dbce_shim.dll"], (pack.ROOT / "CMakeLists.txt").read_text())
        for value in pin["files"].values():
            self.assertIn(value, (vendor / "PROVENANCE.md").read_text())

    def test_private_stage_option_does_not_bypass_embedded_deluxe_refusal(self):
        build = self.base / "deluxe-build"
        build.mkdir()
        for name in ("FZERO_DELUXE_GEN_DIR", "FZERO_DELUXE_DATA_FILE"):
            (build / "CMakeCache.txt").write_text(name + ":PATH=private-cartridge-data\n")
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, "Stock-only build required"):
                stage.stage(build, self.base / "must-not-exist", private_shared_input=True)
            self.assertFalse((self.base / "must-not-exist").exists())

    def test_private_header_pin_survives_fresh_windows_checkout(self):
        # Read Git's actual index, not this potentially stale CRLF worktree.
        name = "vendor/dbce-shim/dbce_shim.h"
        stored = subprocess.check_output(["git", "-C", str(pack.ROOT), "show", ":" + name])
        pin = json.loads((pack.ROOT / "vendor/dbce-shim/PIN.json").read_text())
        expected = pin["files"]["dbce_shim.h"]
        self.assertEqual(pack.digest(stored), expected)
        source = self.base / "fresh-source"
        source.mkdir()
        def git(*args):
            return subprocess.check_output(["git", "-C", str(source), *args], stderr=subprocess.DEVNULL)
        git("init", "-q")
        git("config", "core.autocrlf", "true")
        (source / ".gitattributes").write_bytes((pack.ROOT / ".gitattributes").read_bytes())
        path = source / name
        path.parent.mkdir(parents=True)
        path.write_bytes(stored)
        git("add", ".gitattributes", name)
        self.assertEqual(pack.digest(git("show", ":" + name)), expected)
        checkout = self.base / "fresh-checkout"
        checkout.mkdir()
        git("checkout-index", "--prefix=" + checkout.as_posix() + "/", "--", name)
        self.assertEqual((checkout / name).read_bytes(), stored)
        self.assertEqual(pack.digest((checkout / name).read_bytes()), expected)

    def test_canonical_preview_identity_and_rejected_legacy_or_mismatched_metadata(self):
        product=json.loads((pack.ROOT/'game-product.json').read_text())
        identity,archive=pack.preview_identity(product,'1.8.3','1'*40)
        self.assertEqual(identity,'DBCE F-Zero SNES Unified Preview 1.8.3+g111111111111')
        self.assertEqual(archive,'DBCE-FZeroSNES-unified-preview-1.8.3-g111111111111-windows-x64.zip')
        for field,value in (('name','FZeroSNESRecomp'),('upstream','https://example.invalid'),
                            ('upstreamBaseVersion','1.8.4'),('entrypoint','new.exe')):
            changed=copy.deepcopy(product); changed[field]=value
            with self.subTest(field=field), self.assertRaises(ValueError):
                pack.preview_identity(changed,'1.8.3','1'*40)
        changed=copy.deepcopy(product); changed['release']['artifactStem']='upstream-release'
        with self.assertRaises(ValueError):pack.preview_identity(changed,'1.8.3','1'*40)
        changed=copy.deepcopy(product); changed['documentation']['setup']='docs/missing.md'
        with self.assertRaises(ValueError):pack.preview_identity(changed,'1.8.3','1'*40)
        for field,value in (('channel','release'),('preserveUserSettings',False),('preserveUserSettings',1)):
            changed=copy.deepcopy(product); changed['release'][field]=value
            with self.subTest(field=field,value=value), self.assertRaises(ValueError):
                pack.preview_identity(changed,'1.8.3','1'*40)
        for version,revision in (('1.8.3-preview','1'*40),('1.8.3','short')):
            with self.subTest(version=version,revision=revision), self.assertRaises(ValueError):
                pack.preview_identity(product,version,revision)

    def test_canonical_documentation_links_exist_and_settings_identity_preserved(self):
        product=json.loads((pack.ROOT/'game-product.json').read_text())
        for name in product['documentation'].values():
            self.assertTrue((pack.ROOT/name).is_file(),name)
            self.assertIn(Path(name).name,pack.DOCS)
        self.assertEqual(product['id'],'fzero-snes-recomp')
        self.assertEqual(product['settings'],['config.ini','fzero-video.ini','keybinds.ini','rom.cfg'])
        self.assertTrue(product['release']['preserveUserSettings'])
        self.assertEqual((pack.ROOT/'VERSION').read_text().strip(),product['upstreamBaseVersion'])

    def test_repository_rename_preserves_stable_identity_and_rejects_old_current_url(self):
        product=json.loads((pack.ROOT/'game-product.json').read_text())
        self.assertEqual(product['fork'],'https://github.com/d-b-c-e/dbce-mods-fzero-snes')
        self.assertEqual(product['repositoryId'],1380896600)
        self.assertEqual(product['upstream'],'https://github.com/mstan/FZeroSNESRecomp')
        for field,value in (('fork','https://github.com/d-b-c-e/FZeroSNESRecomp'),
                            ('fork','https://github.com/example/dbce-mods-fzero-snes'),
                            ('repositoryId',1380896601),('repositoryId','1380896600'),
                            ('repositoryId',1380896600.0),('repositoryId',True),
                            ('repositoryId',None)):
            changed=copy.deepcopy(product); changed[field]=value
            with self.subTest(field=field,value=value), self.assertRaises(ValueError):
                pack.preview_identity(changed,'1.8.3','1'*40)

    def test_rehashed_archive_cannot_change_repository_identity(self):
        self.source()
        original=pack.package(self.payload,self.receipt,self.base/'out',self.root)
        with zipfile.ZipFile(original) as archive:
            original_files={n:archive.read(n) for n in archive.namelist()}
        for field,value in (('fork','https://github.com/d-b-c-e/FZeroSNESRecomp'),
                            ('repositoryId',1380896601)):
            files=dict(original_files)
            product=json.loads(files['game-product.json']); product[field]=value
            files['game-product.json']=json.dumps(product).encode()
            manifest=json.loads(files['manifest.json'])
            manifest['files']['game-product.json']=pack.digest(files['game-product.json'])
            files['manifest.json']=json.dumps(manifest).encode()
            with self.subTest(field=field), self.assertRaisesRegex(ValueError,'Canonical unified product identity'):
                pack.verify(io.BytesIO(pack.zip_bytes(files)))

    def test_rehashed_manifest_cannot_relabel_preview_as_upstream(self):
        self.source()
        original=pack.package(self.payload,self.receipt,self.base/'out',self.root)
        with zipfile.ZipFile(original) as archive:
            files={n:archive.read(n) for n in archive.namelist()}
        product=json.loads(files['game-product.json']); product['name']='FZeroSNESRecomp'
        files['game-product.json']=json.dumps(product).encode()
        manifest=json.loads(files['manifest.json'])
        manifest['files']['game-product.json']=pack.digest(files['game-product.json'])
        files['manifest.json']=json.dumps(manifest).encode()
        with self.assertRaisesRegex(ValueError,'Canonical unified product identity'):
            pack.verify(io.BytesIO(pack.zip_bytes(files)))

    def test_missing_import(self):
        del self.receipt["files"]["SDL3.dll"]
        with self.assertRaisesRegex(ValueError, "Unresolved"):
            self.read()

    def test_missing_dynamic_wheel(self):
        del self.receipt["files"]["WheelFfb.dll"]
        with self.assertRaisesRegex(ValueError, "Missing required"):
            self.read()

    def test_missing_replay_runner(self):
        del self.receipt["files"]["FZeroSNESRecompHeadless.exe"]
        with self.assertRaisesRegex(ValueError, "Missing required"):
            self.read()

    def test_mutated_binary(self):
        (self.payload / "FZeroSNESRecomp.exe").write_bytes(b"changed")
        with self.assertRaisesRegex(ValueError, "hash mismatch"):
            self.read()

    def test_private_payload_and_stock_receipt(self):
        self.files["FZeroSNESRecomp.exe"] += b"BSDELX1"
        self.write_payload()
        with self.assertRaisesRegex(ValueError, "Embedded private"):
            self.read()
        self.receipt["bsDeluxe"] = True
        with self.assertRaisesRegex(ValueError, "stock-only"):
            self.read()

    def test_forbidden_user_files_and_escapes(self):
        for name in ("config.ini", "rom.cfg", "src/gen/private.c", "mods/bs-deluxe.dat",
                     "saves/fzero.sav", "../evil.dll", "C:/evil.dll", "a\\evil.dll",
                     "assets/shaders/crt-geom.glsl", "record.fzpt"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                pack.approved(name)

    def test_original_game_artwork_and_content_rejected(self):
        for name in ('assets/img/boxart.tga','assets/img/F-Zero.png',
                     'docs/screenshots/attract-race.png','patches/bs-deluxe-usa.ips',
                     'game.sfc','game.smc','game.bs','state.bin','recording.jsonl',
                     'fzero-video.ini','keybinds.ini','rom.cfg','mods/bs-deluxe.dat'):
            with self.subTest(name=name), self.assertRaisesRegex(ValueError,'Unapproved'):
                pack.approved(name)
        self.assertNotIn('assets/img/boxart.tga',pack.UI_ASSETS)
        self.assertFalse(any(n.startswith('docs/screenshots/') for n in pack.SOURCE_FILES))
        self.files['assets/img/boxart.tga']=b'Original artwork supplied with a self-consistent receipt'
        self.write_payload()
        with self.assertRaisesRegex(ValueError,'Unapproved'):
            self.read()

    def test_missing_asset_notices_and_mutated_flag_license_rejected(self):
        for name in pack.IMAGE_NOTICES:
            original=self.receipt['files'].pop(name)
            with self.subTest(name=name), self.assertRaisesRegex(ValueError,'Missing required'):
                self.read()
            self.receipt['files'][name]=original
        self.files['licenses/flag-font-OFL.txt']=b'OFL link only'
        self.write_payload()
        with self.assertRaisesRegex(ValueError,'Complete pinned OFL'):
            self.read()

    def test_case_collision(self):
        self.receipt["files"]["WHEELFFB.dll"] = self.receipt["files"]["WheelFfb.dll"]
        with self.assertRaisesRegex(ValueError, "Case-colliding"):
            self.read()

    def test_wrong_architecture_and_delay_imports(self):
        data = bytearray(pe())
        struct.pack_into("<H", data, 132, 0x14c)
        with self.assertRaisesRegex(ValueError, "x64"):
            pack.pe_imports(data)
        data = bytearray(pe())
        struct.pack_into("<I", data, 368, 4096)
        with self.assertRaisesRegex(ValueError, "Delay imports"):
            pack.pe_imports(data)

    def test_missing_notice(self):
        del self.receipt["files"]["licenses/wheel-toolkit.txt"]
        with self.assertRaisesRegex(ValueError, "Missing required"):
            self.read()

    def test_abbreviated_or_mutated_ofl_is_rejected(self):
        for font in pack.OFL_FILES:
            name = "licenses/" + Path(font).stem + ".txt"
            original = self.files[name]
            for invalid in (b"SIL OPEN FONT LICENSE 1.1 https://scripts.sil.org/OFL",
                            original[:-1], pack.OFL_SEPARATOR + original.split(pack.OFL_SEPARATOR)[1]):
                with self.subTest(font=font, notice=invalid[:30]):
                    self.files[name] = invalid
                    self.write_payload()
                    with self.assertRaisesRegex(ValueError, "Complete upstream OFL"):
                        self.read()
            self.files[name] = original
            self.write_payload()

    def test_staging_preserves_embedded_notice_and_full_ofl(self):
        embedded = 'Copyright fixture author with Reserved Font Name "Fixture"'
        raw = embedded.encode("utf-16-be")
        font = bytearray(46 + len(raw))
        struct.pack_into(">H", font, 4, 1)
        struct.pack_into(">4sIII", font, 12, b"name", 0, 28, 18 + len(raw))
        struct.pack_into(">HHH", font, 28, 0, 1, 18)
        struct.pack_into(">6H", font, 34, 3, 1, 0x409, 0, len(raw), 0)
        font[46:] = raw
        for name, (path, expected) in pack.OFL_FILES.items():
            notice = stage.font_notice(font, name)
            prefix, _, full_license = notice.partition(pack.OFL_SEPARATOR)
            self.assertIn(embedded.encode(), prefix)
            self.assertEqual(full_license, (pack.ROOT / path).read_bytes())
            self.assertEqual(pack.digest(full_license), expected)

    def test_missing_launcher_asset(self):
        del self.receipt["files"]["assets/fonts/LatoLatin-Regular.ttf"]
        with self.assertRaisesRegex(ValueError, "Missing required"):
            self.read()

    def test_wheel_override(self):
        with self.assertRaisesRegex(ValueError, "repin"):
            pack.read_payload(self.payload, self.receipt, "1.7.0", "0" * 64)

    def test_roundtrip_deterministic_and_no_source_settings(self):
        self.source()
        first = pack.package(self.payload, self.receipt, self.base / "out1", self.root)
        second = pack.package(self.payload, self.receipt, self.base / "out2", self.root)
        self.assertEqual(first.read_bytes(), second.read_bytes())
        result = pack.verify(first)
        self.assertEqual(result["build"]["sourceRevision"], self.receipt["sourceRevision"])
        with zipfile.ZipFile(first) as archive:
            self.assertEqual(archive.read("Setup.cmd"), pack.SETUP)
            self.assertIn("tools/Recording.ps1", archive.namelist())
            self.assertIn("docs/RECORDING-QUICKSTART.md", archive.namelist())
            self.assertNotIn("dbce_shim.dll", archive.namelist())
            self.assertNotIn("config.ini", archive.namelist())
            self.assertNotIn('assets/img/boxart.tga',archive.namelist())
            self.assertFalse(any(n.startswith('docs/screenshots/') for n in archive.namelist()))
            self.assertTrue(result['previewIdentity'].startswith('DBCE F-Zero SNES Unified Preview '))
            self.assertTrue(first.name.startswith('DBCE-FZeroSNES-unified-preview-'))
        with self.assertRaises(FileExistsError):
            pack.package(self.payload, self.receipt, self.base / "out1", self.root)

    def test_dirty_or_mismatched_source(self):
        self.source()
        (self.root / "README.md").write_text("changed")
        with self.assertRaisesRegex(ValueError, "clean build"):
            pack.package(self.payload, self.receipt, self.base / "out", self.root)
        self.git("checkout", "--", "README.md")
        self.receipt["sourceTree"] = "0" * 40
        with self.assertRaisesRegex(ValueError, "tree"):
            pack.package(self.payload, self.receipt, self.base / "out", self.root)

    def test_tampered_archive_and_unapproved_extras(self):
        self.source()
        original = pack.package(self.payload, self.receipt, self.base / "out", self.root)
        with zipfile.ZipFile(original) as archive:
            files = {n: archive.read(n) for n in archive.namelist()}
        files["Setup.cmd"] += b"changed"
        with self.assertRaisesRegex(ValueError, "hash mismatch"):
            pack.verify(io.BytesIO(pack.zip_bytes(files)))
        files["Setup.cmd"] = pack.SETUP
        files["config.ini"] = b"settings"
        manifest = json.loads(files["manifest.json"])
        manifest["files"]["config.ini"] = pack.digest(files["config.ini"])
        files["manifest.json"] = json.dumps(manifest).encode()
        with self.assertRaisesRegex(ValueError, "Unapproved"):
            pack.verify(io.BytesIO(pack.zip_bytes(files)))


if __name__ == "__main__":
    unittest.main()
