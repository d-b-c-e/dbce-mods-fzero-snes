"""Package a receipted Windows candidate; default stock-only, explicit private Deluxe.

Build receipt JSON:
 {"schema":"dbce.fzero-build", "version":1, "productVersion":"1.8.3",
  "sourceRevision":"<40 hex>", "sourceTree":"<40 hex>", "dirty":false,
  "bsDeluxe":false, "dependencies":{"snesrecomp":"<40 hex>",
  "recomp-ui":"<40 hex>"}, "files":{"FZeroSNESRecomp.exe":"<sha256>",
  "WheelFfb.dll":"<sha256>", "licenses/wheel-toolkit.txt":"<sha256>", ...}}
The receipt must be generated at build time from the exact approved staging
inputs; this tool validates consistency, not compiler provenance.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import struct
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
# Qualified Native2 helper with an absent-config baseline for fresh packages.
NATIVE2_HELPER_SOURCE = "d0f333c60892dc44beb36e277b19bd6a650507b7"
OFL_SEPARATOR = b"\n--- Complete upstream font license ---\n"
OFL_FILES = {
    "LatoLatin-Bold.ttf": ("licenses/fonts/Lato-OFL.txt", "74ba064d03f1f1c4a952da936c3eb71866c34404916734de3cae73b34357e59e"),
    "LatoLatin-Regular.ttf": ("licenses/fonts/Lato-OFL.txt", "74ba064d03f1f1c4a952da936c3eb71866c34404916734de3cae73b34357e59e"),
    "NotoSansSymbols2-Regular.ttf": ("licenses/fonts/Noto-OFL.txt", "b118dd41337806a5d4797052c77caf3bd096aed783e5eb21b4d11154351e1ac0"),
}
# STD-050: recording and replay are developer tools, so their docs and helpers
# (PLAYTHROUGH_RECORDING.md, RECORDING-QUICKSTART.md, tools/Recording.ps1,
# tools/fzero_replay_adapter.py) stay out of the player package.
DOCS = ("SETUP.md", "UNIFIED-PRODUCT.md",
        "TRIPLE_SCREEN_FEASIBILITY.md", "TELEMETRY_SIGNALS.md",
        "ADAPTIVE_RENDERER.md", "BS_DELUXE_EXPLORATION.md", "HD_MODE7.md",
        "HD_MODE7_PERFORMANCE.md", "PERFORMANCE_DIAGNOSTICS.md", "SAVE_STATES.md",
        "WHEELFFB_EXPERIMENTAL_RECEIPT.md", "DISTRIBUTION-AUDIT.md", "PRODUCT-IDENTITY.md")
SOURCE_FILES = ("README.md", "LICENSE", "VERSION", "game-product.json",
                "lib/toolkit/MANIFEST.txt", "lib/toolkit/VERSION", "lib/toolkit/LICENSE.txt",
                "lib/toolkit/SOURCE-PROVENANCE.json", "lib/toolkit/source-to-binary-manifest.json",
                "assets/README.md")
UI_ASSETS = tuple("assets/img/" + n for n in (
    "brand_mark.tga", "flags.png", "pad.tga", "verdict_bad.tga",
    "verdict_none.tga", "verdict_ok.tga", "verdict_warn.tga")) + tuple(
    "assets/fonts/" + n for n in ("LatoLatin-Bold.ttf", "LatoLatin-Regular.ttf",
    "NotoSansSymbols2-Regular.ttf", "OpenMoji-black-glyf.ttf"))
IMAGE_NOTICES = ("licenses/launcher-images.txt", "licenses/launcher-fonts.txt",
                 "licenses/flag-font-OFL.txt")
SETUP = b'@echo off\r\ncd /d "%~dp0"\r\n"%~dp0FZeroSNESRecomp.exe" --launcher\r\n'
SYSTEM_DLLS = {"kernel32.dll", "user32.dll", "gdi32.dll", "advapi32.dll",
              "shell32.dll", "ole32.dll", "oleaut32.dll", "comdlg32.dll",
              "comctl32.dll", "ws2_32.dll", "winmm.dll", "imm32.dll",
              "version.dll", "setupapi.dll", "hid.dll", "dinput8.dll",
              "dxguid.dll", "opengl32.dll", "dwmapi.dll", "uxtheme.dll",
              "ntdll.dll", "msvcrt.dll", "ucrtbase.dll", "shlwapi.dll",
              "cfgmgr32.dll", "rpcrt4.dll", "bcrypt.dll", "secur32.dll"}
SYSTEM_DLLS |= {"d2d1.dll", "dwrite.dll"}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def preview_identity(product, version, revision):
    """Canonical metadata identity; numeric VERSION remains the upstream base."""
    if not isinstance(product, dict):
        raise ValueError('Product metadata must be an object')
    required = {'id': 'fzero-snes-recomp',
                'name': 'DBCE F-Zero SNES Unified Preview',
                'upstream': 'https://github.com/mstan/FZeroSNESRecomp',
                'fork': 'https://github.com/d-b-c-e/dbce-mods-fzero-snes',
                'repositoryId': 1380896600,
                'sourceBranch': 'codex/unified-product-20261001',
                'entrypoint': 'FZeroSNESRecomp.exe', 'setupArgument': '--launcher'}
    if any(product.get(k) != v for k, v in required.items()):
        raise ValueError('Canonical unified product identity differs')
    if type(product.get('repositoryId')) is not int:
        raise ValueError('Numeric stable repository ID required')
    release = product.get('release', {})
    if not isinstance(release, dict):
        raise ValueError('Release metadata must be an object')
    if release.get('artifactStem') != 'DBCE-FZeroSNES-unified-preview' or release.get('versionPolicy') != 'upstream-base-with-source-revision':
        raise ValueError('Explicit fork preview version policy required')
    if any(release.get(k) != v for k, v in {
            'tool': 'tools/package_unified.py', 'channel': 'candidate',
            'platform': 'windows-x64', 'distribution': 'stock-only-rom-free'}.items()) or release.get('preserveUserSettings') is not True or product.get('versionFile') != 'VERSION':
        raise ValueError('Canonical preview release contract differs')
    if product.get('settings') != ['config.ini', 'fzero-video.ini', 'keybinds.ini', 'rom.cfg']:
        raise ValueError('Existing settings identity must be preserved')
    if not isinstance(version, str) or not re.fullmatch(r'\d+\.\d+\.\d+', version) or product.get('upstreamBaseVersion') != version:
        raise ValueError('Numeric VERSION must match declared upstream base')
    if not isinstance(revision, str) or not re.fullmatch(r'[0-9a-f]{40}', revision):
        raise ValueError('Exact source revision required for preview identity')
    docs = {'product': 'docs/UNIFIED-PRODUCT.md', 'setup': 'docs/SETUP.md',
            'identity': 'docs/PRODUCT-IDENTITY.md', 'distribution': 'docs/DISTRIBUTION-AUDIT.md'}
    if product.get('documentation') != docs:
        raise ValueError('Canonical documentation links differ')
    return (f"{product['name']} {version}+g{revision[:12]}",
            f"{release['artifactStem']}-{version}-g{revision[:12]}-windows-x64.zip")


def relative(name):
    path = PurePosixPath(name)
    if not name or "\\" in name or ":" in name or path.is_absolute() or any(
            p in ("", ".", "..") for p in name.split("/")):
        raise ValueError(f"Unsafe payload path: {name}")
    return path


def approved(name, private_deluxe=False):
    path = relative(name)
    low = name.lower()
    if private_deluxe and (name in {"FZeroSNESRecomp-wheel-launcher.exe", "install-receipt.json", "private-inputs/backup-state.json", "tools/Recording.ps1", "dbce_shim.dll"}
                          or name.startswith(("controls-setup/", "shared-input/"))):
        return True
    if low == "dbce_shim.dll":
        raise ValueError("Private shared-input runtime excluded from public package")
    if low in ("fzerosnesrecomp.exe", "fzerosnesrecompheadless.exe") or (len(path.parts) == 1 and low.endswith(".dll")):
        return True
    if path.parts[0] == "licenses" and path.suffix.lower() in (".txt", ".md"):
        return True
    if name in UI_ASSETS:
        return True
    if low.startswith("assets/shaders/") and path.suffix.lower() in (".glsl", ".glslp") and "crt-geom" not in low:
        return True
    raise ValueError(f"Unapproved payload (private/user data forbidden): {name}")


def pe_imports(data):
    """Read only normal PE imports, rejecting malformed/non-x64 images."""
    try:
        if data[:2] != b"MZ":
            raise ValueError("Not a PE image")
        offset = struct.unpack_from("<I", data, 60)[0]
        if data[offset:offset + 4] != b"PE\0\0":
            raise ValueError("Invalid PE signature")
        machine, sections = struct.unpack_from("<HH", data, offset + 4)
        opt_size = struct.unpack_from("<H", data, offset + 20)[0]
        opt = offset + 24
        if machine != 0x8664 or struct.unpack_from("<H", data, opt)[0] != 0x20b:
            raise ValueError("Expected Windows x64 PE32+")
        section_table = opt + opt_size

        def rva(value):
            for i in range(sections):
                pos = section_table + i * 40
                size, va, raw_size, raw = struct.unpack_from("<IIII", data, pos + 8)
                if va <= value < va + max(size, raw_size):
                    result = raw + value - va
                    if result >= len(data):
                        break
                    return result
            raise ValueError("PE import RVA outside sections")

        if struct.unpack_from("<I", data, opt + 108)[0] < 14:
            raise ValueError("Incomplete PE data directories")
        # Delay imports need a separate audited resolver; do not silently omit.
        if struct.unpack_from("<I", data, opt + 112 + 13 * 8)[0]:
            raise ValueError("Delay imports require explicit packaging support")
        table = struct.unpack_from("<I", data, opt + 120)[0]
        if not table:
            return set()
        pos, names = rva(table), set()
        for _ in range(4096):
            entry = struct.unpack_from("<IIIII", data, pos)
            if not any(entry):
                return names
            start = rva(entry[3])
            end = data.find(b"\0", start, start + 256)
            if end < 0:
                raise ValueError("Unterminated PE import")
            name = data[start:end].decode("ascii").lower()
            if "/" in name or "\\" in name or not name.endswith(".dll"):
                raise ValueError("Unsafe PE import name")
            names.add(name)
            pos += 20
        raise ValueError("Unterminated PE import table")
    except (struct.error, UnicodeError) as error:
        raise ValueError("Malformed PE import table") from error


def read_payload(payload, receipt, product_version, toolkit_hash, private_deluxe=False):
    if receipt.get("schema") != "dbce.fzero-build" or receipt.get("version") != 1:
        raise ValueError("Unsupported build receipt")
    if "privateSharedInput" in receipt and not private_deluxe:
        raise ValueError("Private shared-input receipt excluded from public package")
    if receipt.get("dirty") is not False or receipt.get("bsDeluxe") is not private_deluxe:
        raise ValueError("Requires clean Deluxe build in private mode" if private_deluxe else "Requires clean stock-only build; private BS module excluded")
    if receipt.get("productVersion") != product_version:
        raise ValueError("Build/product version mismatch")
    for key in ("sourceRevision", "sourceTree"):
        if not re.fullmatch(r"[0-9a-f]{40}", receipt.get(key, "")):
            raise ValueError(f"Invalid {key}")
    deps = receipt.get("dependencies", {})
    if set(deps) != {"snesrecomp", "recomp-ui"} or any(
            not re.fullmatch(r"[0-9a-f]{40}", v) for v in deps.values()):
        raise ValueError("Exact submodule identities required")
    files, result, seen = receipt.get("files", {}), {}, set()
    for name, expected in files.items():
        approved(name, private_deluxe)
        if name.lower() in seen:
            raise ValueError("Case-colliding payload")
        seen.add(name.lower())
        path = payload.joinpath(*relative(name).parts)
        if any(p.is_symlink() for p in (path, *path.parents)) or not path.resolve().is_relative_to(payload.resolve()):
            raise ValueError("Symlink/path escape in payload")
        data = path.read_bytes()
        if not isinstance(expected, str) or digest(data) != expected.lower():
            raise ValueError(f"Payload hash mismatch: {name}")
        # Core recognizes this format; validate_deluxe still binds its exact helper inventory.
        if b"BSDELX1" in data and not (private_deluxe and name in ("FZeroSNESRecomp-wheel-launcher.exe", "FZeroSNESRecompHeadless.exe", "controls-setup/Wheelkit.Core.dll")):
            raise ValueError("Embedded private BS payload excluded")
        result[name] = data
    required = {"FZeroSNESRecomp-wheel-launcher.exe" if private_deluxe else "FZeroSNESRecomp.exe", "FZeroSNESRecompHeadless.exe", "WheelFfb.dll",
                "licenses/snesrecomp.txt", "licenses/recomp-ui.txt", "licenses/imgui.txt",
                "licenses/wheel-toolkit.txt"} | set(UI_ASSETS) | set(IMAGE_NOTICES)
    required |= {"licenses/" + Path(n).stem + ".txt" for n in UI_ASSETS if n.endswith(".ttf")}
    if not required.issubset(result):
        raise ValueError(f"Missing required payload: {sorted(required - result.keys())}")
    for font, (_, expected) in OFL_FILES.items():
        notice = result["licenses/" + Path(font).stem + ".txt"]
        embedded, separator, full_license = notice.partition(OFL_SEPARATOR)
        if not embedded.strip() or not separator or digest(full_license) != expected:
            raise ValueError(f"Complete upstream OFL and embedded notices required: {font}")
    if digest(result['licenses/flag-font-OFL.txt']) != OFL_FILES['NotoSansSymbols2-Regular.ttf'][1]:
        raise ValueError('Complete pinned OFL required for flag font artwork')
    if digest(result["WheelFfb.dll"]) != toolkit_hash.lower():
        raise ValueError("WheelFfb override needs explicit source repin")
    if private_deluxe:
        validate_deluxe(payload, receipt, result)
    binaries = {name.lower(): data for name, data in result.items() if name.lower().endswith((".exe", ".dll")) and not name.startswith("controls-setup/")}
    for name, data in binaries.items():
        for imported in pe_imports(data):
            if imported not in binaries and imported not in SYSTEM_DLLS and not imported.startswith(("api-ms-", "ext-ms-")):
                raise ValueError(f"Unresolved runtime DLL: {name} -> {imported}")
    return result


def validate_deluxe(payload, receipt, files, setup_root=None):
    """Bind the existing installer's fresh scratch receipt to exact private inputs."""
    from setup_bundle import payload as setup_payload
    if "FZeroSNESRecomp.exe" in files or setup_root is None and (payload / "build-receipt.json").exists():
        raise ValueError("Stock/Deluxe receipt or consumer ambiguity")
    if setup_root is None and {p.relative_to(payload).as_posix() for p in payload.rglob("*") if p.is_file()} != set(files):
        raise ValueError("Unreceipted Deluxe payload member")
    required = {"install-receipt.json", "private-inputs/backup-state.json", "controls-setup/bundle-manifest.json",
                "FZeroSNESRecomp-wheel-launcher.exe", "WheelFfb.dll", "dbce_shim.dll", "shared-input/PIN.json"}
    if not required.issubset(files):
        raise ValueError("Complete private Deluxe inputs required")
    raw = files["install-receipt.json"]
    if len(raw) > 256 * 1024:
        raise ValueError("Oversized Deluxe install receipt")
    def unique(pairs):
        value = {}
        for key, item in pairs:
            if key in value:
                raise ValueError("Duplicate Deluxe receipt field")
            value[key] = item
        return value
    installed = json.loads(raw.decode("utf-8-sig"), object_pairs_hook=unique)
    launcher = "FZeroSNESRecomp-wheel-launcher.exe"
    native = {"schema": 2, "adapter": "fzero-raw-wheel-2", "consumer": launcher}
    actions = ["nativeX", "nativeY", "nativeL", "nativeR", "saveStateMenu", "rewind"]
    if (installed.get("schemaVersion") != 1 or installed.get("controlsProfileSchema") != 1
            or installed.get("controlsProfileAdapter") != "fzero-raw-wheel-1"
            or installed.get("launcher") != launcher or installed.get("nativeControls") != native
            or type(installed.get("nativeControls", {}).get("schema")) is not int
            or installed.get("controlsProfileActions") != actions
            or installed.get("sourceCommit") != receipt["sourceRevision"]):
        raise ValueError("Exact Deluxe source and Native2 receipt required")
    image = files[launcher]
    if (b"BSDELX1\0" + struct.pack("<I", 1024 * 1024) not in image
            or any((key + "\0").encode() not in image for key in actions + ["SteeringProfileBinding", "AcceleratorProfileBinding", "BrakeProfileBinding"])):
        raise ValueError("Linked Deluxe/Native2 consumer required")
    if (installed.get("launcherSha256", "").lower() != digest(image)
            or installed.get("wheelFfbSha256", "").lower() != digest(files["WheelFfb.dll"])):
        raise ValueError("Deluxe receipt payload differs")
    setup_files, setup = setup_payload(setup_root or payload / "controls-setup", image)
    proof = receipt.get("privateDeluxe", {})
    if not isinstance(proof, dict):
        raise ValueError("Private Deluxe provenance required")
    embedded = proof.get("embeddedData", {})
    sources = proof.get("generatedSources", {})
    if (not isinstance(embedded, dict) or type(embedded.get("offset")) is not int or type(embedded.get("bytes")) is not int
            or embedded["offset"] < 0 or embedded["bytes"] < 12 or embedded["offset"] + embedded["bytes"] > len(image)
            or image[embedded["offset"]:embedded["offset"] + 12] != b"BSDELX1\0" + struct.pack("<I", 1024 * 1024)
            or digest(image[embedded["offset"]:embedded["offset"] + embedded["bytes"]]) != embedded.get("sha256")
            or not isinstance(sources, dict) or "deluxe_namespace.h" not in sources or not any(n.endswith(".c") for n in sources)
            or any(relative(n).name != n or not re.fullmatch(r"[0-9a-f]{64}", h) for n, h in sources.items())):
        raise ValueError("Exact embedded Deluxe data and generated-source provenance required")
    if image[embedded["offset"]:embedded["offset"] + embedded["bytes"]] not in files["FZeroSNESRecompHeadless.exe"]:
        raise ValueError("Headless companion does not embed the same Deluxe data")
    installed_setup = {key: setup[key] for key in ("sourceCommit", "manifestSha256", "exeSha256", "folder", "game", "waitForProcessContract", "waitForProcessTimeoutMs")}
    if (proof.get("helperSource") != NATIVE2_HELPER_SOURCE
            or setup["sourceCommit"] != proof["helperSource"]
            or installed.get("privateControlsSetup") != installed_setup
            or receipt.get("privateControlsSetup") != setup
            or installed.get("privateSharedInput") != receipt.get("privateSharedInput")
            or proof.get("installReceiptSha256") != digest(raw)):
        raise ValueError("Exact Deluxe helper/receipt provenance required")
    shared_files = {"shared-input/" + name for name in ("dbce_shim.h", "dbce_shim_extras.h", "PIN.json", "PROVENANCE.md")}
    if {name for name in files if name.startswith("shared-input/")} != shared_files:
        raise ValueError("Exact private Shared provenance inventory required")
    pin = json.loads(files["shared-input/PIN.json"].decode(), object_pairs_hook=unique)
    shared = receipt.get("privateSharedInput", {})
    if not isinstance(shared, dict):
        raise ValueError("Private Shared input provenance required")
    controls = shared.get("controls", {})
    if (pin.get("schema") != "dbce.fzero-shared-input-pin" or pin.get("version") != 1 or pin.get("abi") != 2
            or pin.get("distribution") != "private-only" or pin.get("extrasAbi") != 1
            or pin.get("sourceRevision") != shared.get("sourceRevision")
            or set(pin.get("files", {})) != {"dbce_shim.dll", "dbce_shim.h", "dbce_shim_extras.h"}
            or shared.get("runtimeSha256") != digest(files["dbce_shim.dll"])
            or not isinstance(controls, dict) or controls.get("schema") != 1
            or controls.get("adapter") != "fzero-shim-ini-controls-1" or controls.get("consumer") != launcher
            or any(expected != digest(files[name if name.endswith(".dll") else "shared-input/" + name]) for name, expected in pin["files"].items())):
        raise ValueError("Private Shared input pins differ")
    owned = {launcher, "WheelFfb.dll", "tools/Recording.ps1", "dbce_shim.dll"} | shared_files | set(setup_files)
    rows = installed["files"]
    if (len(rows) != len(owned) or {row["name"] for row in rows} != owned
            or any(row["sha256"].lower() != digest(files[row["name"]]) for row in rows)):
        raise ValueError("Deluxe install inventory differs from build inputs")
    rollback_raw = files["private-inputs/backup-state.json"]
    rollback = json.loads(rollback_raw.decode("utf-8-sig"), object_pairs_hook=unique)
    rollback_owned = owned | {"install-receipt.json"}
    rows = rollback["files"]
    if (rollback.get("schemaVersion") != 1 or len(rows) != len(rollback_owned)
            or {row["name"] for row in rows} != rollback_owned
            or any(row.get("existed") is not False or row.get("sha256") is not None for row in rows)
            or proof.get("rollbackStateSha256") != digest(rollback_raw)):
        raise ValueError("Fresh scratch-install rollback provenance required")


def zip_bytes(files):
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data)
    return stream.getvalue()


def verify(path, private_deluxe=False):
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        if len(set(n.lower() for n in names)) != len(names):
            raise ValueError("Duplicate ZIP entries")
        for name in names:
            relative(name)
        manifest = json.loads(archive.read("manifest.json"))
        if manifest.get("schema") != "dbce.fzero-package" or manifest.get("schemaVersion") != 1:
            raise ValueError("Unsupported package manifest")
        expected = manifest["files"]
        if set(names) != set(expected) | {"manifest.json"}:
            raise ValueError("Package file set differs from manifest")
        for name, hash_value in expected.items():
            if digest(archive.read(name)) != hash_value:
                raise ValueError(f"Package hash mismatch: {name}")
        receipt = manifest["build"]
        payload_names = receipt["files"]
        extras = set(SOURCE_FILES) | {"Setup.cmd"} | {"docs/" + n for n in DOCS}
        if set(expected) != set(payload_names) | extras:
            raise ValueError("Unapproved package file set")
        distribution = "private-deluxe-native2" if private_deluxe else "stock-only-rom-free"
        if manifest.get("productId") != "fzero-snes-recomp" or manifest.get("channel") != "candidate" or manifest.get("distribution") != distribution:
            raise ValueError("Wrong product/distribution identity")
        product = json.loads(archive.read("game-product.json"))
        identity, _ = preview_identity(product, manifest['version'], receipt['sourceRevision'])
        if manifest.get('previewIdentity') != identity:
            raise ValueError('Explicit fork preview identity required')
        if product.get("id") != manifest["productId"] or archive.read("VERSION").decode().strip() != manifest["version"]:
            raise ValueError("Product version/identity differs from manifest")
        pin = archive.read("lib/toolkit/MANIFEST.txt").decode()
        match = re.search(r"(?mi)^([0-9a-f]{64})[ \t]+native/WheelFfb\.dll[ \t]*\r?$", pin)
        if not match or match.group(1).lower() != manifest["wheelRuntimeSha256"]:
            raise ValueError("Package runtime differs from retained toolkit pin")
        for name, hash_value in payload_names.items():
            if expected.get(name) != hash_value.lower():
                raise ValueError("Package/build receipt hashes differ")
        # Revalidate the receipt boundaries and binary identities without extracting.
        import tempfile
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            for name in payload_names:
                destination = root.joinpath(*relative(name).parts)
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(archive.read(name))
            read_payload(root, receipt, manifest["version"], manifest["wheelRuntimeSha256"], private_deluxe)
    return manifest


def package(payload, receipt, output, root=ROOT, private_deluxe=False):
    product = json.loads((root / "game-product.json").read_text())
    version = (root / "VERSION").read_text().strip()
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError("Invalid VERSION")
    identity, archive_name = preview_identity(product, version, receipt['sourceRevision'])
    pin = (root / "lib/toolkit/MANIFEST.txt").read_text()
    match = re.search(r"(?mi)^([0-9a-f]{64})[ \t]+native/WheelFfb\.dll[ \t]*\r?$", pin)
    if not match:
        raise ValueError("Missing native runtime toolkit pin")
    wheel_hash = match.group(1).lower()
    files = read_payload(payload, receipt, version, wheel_hash, private_deluxe)
    if private_deluxe:
        archive_name = archive_name.removesuffix(".zip") + "-private-deluxe-native2.zip"
    git = lambda *args: subprocess.check_output(["git", "-C", str(root), *args], text=True).strip()
    if git("rev-parse", receipt["sourceRevision"] + "^{tree}") != receipt["sourceTree"]:
        raise ValueError("Build receipt tree does not match source revision")
    if git("rev-parse", "HEAD") != receipt["sourceRevision"] or git("status", "--porcelain", "--untracked-files=no"):
        raise ValueError("Package from the exact clean build revision")
    links = dict((line.split()[3], line.split()[1]) for line in git("ls-files", "--stage", "snesrecomp", "recomp-ui").splitlines())
    if links != receipt["dependencies"]:
        raise ValueError("Build dependency pins differ from source gitlinks")
    # Only repository shader files are accepted, never user-imported presets.
    shaders = {n: (root / n).read_bytes() for n in git("ls-files", "assets/shaders").splitlines()}
    for name, data in files.items():
        if name.startswith("assets/shaders/") and shaders.get(name) != data:
            raise ValueError("Unreviewed or altered shader in payload")
    if not set(shaders).issubset(files):
        raise ValueError("Bundled shader set incomplete")
    for name in SOURCE_FILES:
        files[name] = (root / name).read_bytes()
    for name in DOCS:
        files["docs/" + name] = (root / "docs" / name).read_bytes()
    files["Setup.cmd"] = SETUP.replace(b"FZeroSNESRecomp.exe", b"FZeroSNESRecomp-wheel-launcher.exe") if private_deluxe else SETUP
    manifest = {"schema": "dbce.fzero-package", "schemaVersion": 1,
                "productId": product["id"], "version": version, "channel": "candidate",
                "previewIdentity": identity,
                "distribution": "private-deluxe-native2" if private_deluxe else "stock-only-rom-free", "build": receipt,
                "wheelRuntimeSha256": wheel_hash,
                "files": {name: digest(data) for name, data in sorted(files.items())}}
    files["manifest.json"] = (json.dumps(manifest, indent=2, sort_keys=True) + "\n").encode()
    data = zip_bytes(files)
    verify(io.BytesIO(data), private_deluxe)
    output.mkdir(parents=True, exist_ok=False)
    archive = output / archive_name
    archive.write_bytes(data)
    archive.with_suffix(".zip.sha256").write_text(f"{digest(data)}  {archive.name}\n", encoding="ascii")
    verify(archive, private_deluxe)
    return archive


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--payload", type=Path)
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--verify", type=Path)
    parser.add_argument("--private-deluxe", action="store_true", help="Local-only Native2 Deluxe package; requires the exact installer/helper/rollback inputs")
    args = parser.parse_args()
    try:
        if args.verify:
            print(json.dumps(verify(args.verify, args.private_deluxe), indent=2))
        elif args.payload and args.receipt and args.output:
            print(package(args.payload, json.loads(args.receipt.read_text()), args.output, private_deluxe=args.private_deluxe))
        else:
            parser.error("Specify --verify or --payload, --receipt and --output")
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error))
