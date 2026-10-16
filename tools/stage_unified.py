"""Create a fresh explicit stock-only staging folder and build receipt.

Run immediately after a clean source build. Reads only that build and its
verified dependency roots; never launches binaries, copies configs or ROMs,
or modifies a source/install folder. Use package_unified.py afterward for normal
staging. --private-shared-input adds the provisional DLL and a private receipt;
the public packager deliberately refuses it. Neither route admits Deluxe data.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess

from package_unified import ROOT, UI_ASSETS, SYSTEM_DLLS, OFL_FILES, OFL_SEPARATOR, digest, pe_imports
from setup_bundle import payload as setup_payload


def git(root, *args):
    return subprocess.check_output(["git", "-c", "safe.directory=" + root.as_posix(),
                                   "-C", str(root), *args], text=True).strip()


def font_notice(data, name):
    records = []
    for i in range(struct.unpack_from(">H", data, 4)[0]):
        tag, _, offset, _ = struct.unpack_from(">4sIII", data, 12 + 16 * i)
        if tag != b"name":
            continue
        _, count, strings = struct.unpack_from(">HHH", data, offset)
        for j in range(count):
            platform, _, _, name_id, length, location = struct.unpack_from(">6H", data, offset + 6 + 12 * j)
            if name_id not in (0, 7, 8, 9, 13, 14):
                continue
            raw = data[offset + strings + location:offset + strings + location + length]
            text = raw.decode("utf-16-be" if platform in (0, 3) else "mac_roman")
            if text not in records:
                records.append(text)
    if not records:
        raise ValueError("Font has no embedded copyright/license records")
    notice = ("\n\n".join(records) + "\n").encode()
    if name in OFL_FILES:
        path, expected = OFL_FILES[name]
        license_text = (ROOT / path).read_bytes()
        if digest(license_text) != expected:
            raise ValueError("Upstream full font license has changed; review and repin")
        notice += OFL_SEPARATOR + license_text
    return notice


def shared_controls_contract():
    """The existing ordinary consumer, not a launcher/session capability.

    tests/test_unified_package.py compares this partition with the C mapper.
    """
    unsupported = {
        "clutch": "F-Zero has no clutch", "handbrake": "F-Zero has no handbrake",
        "shiftUp": "F-Zero has no gearbox", "shiftDown": "F-Zero has no gearbox",
        "neutral": "F-Zero has no gearbox", "reverse": "F-Zero has no reverse gear",
        "camera": "no generic F-Zero camera action", "lookBack": "no F-Zero look-back action",
        "reset": "guest reset remains a native host hotkey", "horn": "F-Zero has no horn",
        "modMenu": "the F-Zero launcher is not an in-game mod menu",
        "muteFfb": "shared input does not own the native force lifecycle",
        **{f"gear{i}": "F-Zero has no gearbox" for i in range(1, 9)},
    }
    return {"schema": 1, "adapter": "fzero-shim-ini-controls-1",
            "consumer": "FZeroSNESRecomp.exe", "profile": "config.ini", "settings": "config.ini",
            "shimAbi": 2, "optIn": {"section": "Input", "key": "Backend", "value": "Shared",
                                    "default": "Native", "appliesAfterRestart": True},
            "delivered": ["steer", "throttle", "brake", "confirm", "back", "start", "select",
                          "navUp", "navDown", "navLeft", "navRight"],
            "unsupported": unsupported, "requires": {"directRom": True, "forceFeedbackDisabled": True}}


def shared_extras_contract():
    declaration = ("game fzero-snes\n"
                   "fzero-snes.x digital\nfzero-snes.y digital\n"
                   "fzero-snes.l digital\nfzero-snes.r digital\n")
    return {"abi": 1, "acceptedSchema": 1, "declaration": declaration,
            "declarationSha256": digest(declaration.encode("ascii")),
            "presentation": [{"id": "fzero-snes." + name, "label": "SNES " + name.upper(),
                              "group": "SNES buttons", "effects": "Guest button in menus and gameplay; keyboard retained.",
                              "limits": "Requires explicit SharedExtras=1; no physical force qualification."}
                             for name in ("x", "y", "l", "r")]}


def private_shared_files(build, root=ROOT):
    """Explicit private add-on only; never infer permission from a nearby DLL."""
    vendor = root / "vendor/dbce-shim"
    pin_bytes = (vendor / "PIN.json").read_bytes()
    pin = json.loads(pin_bytes)
    base_files = {"dbce_shim.dll", "dbce_shim.h"}
    extras = "dbce_shim_extras.h" in pin.get("files", {})
    if pin.get("schema") != "dbce.fzero-shared-input-pin" or pin.get("version") != 1 or \
            pin.get("abi") != 2 or pin.get("distribution") != "private-only" or \
            not re.fullmatch(r"[0-9a-f]{40}", pin.get("sourceRevision", "")) or \
            set(pin.get("files", {})) != base_files | ({"dbce_shim_extras.h"} if extras else set()) or \
            (pin.get("extrasAbi") != 1 if extras else "extrasAbi" in pin):
        raise ValueError("Invalid private shared-input source pin")
    files = {"dbce_shim.dll": (build / "dbce_shim.dll").read_bytes(),
             "shared-input/dbce_shim.h": (vendor / "dbce_shim.h").read_bytes(),
             "shared-input/PIN.json": pin_bytes,
             "shared-input/PROVENANCE.md": (vendor / "PROVENANCE.md").read_bytes()}
    if extras:
        files["shared-input/dbce_shim_extras.h"] = (vendor / "dbce_shim_extras.h").read_bytes()
    for name, expected in pin["files"].items():
        key = name if name.endswith(".dll") else "shared-input/" + name
        if not isinstance(expected, str) or not re.fullmatch(r"[0-9a-f]{64}", expected) or digest(files[key]) != expected:
            raise ValueError(f"Private shared-input bytes differ from source pin: {name}")
    return files, {"abi": 2, "distribution": "private-only", **({"extrasAbi": 1, "extras": shared_extras_contract()} if extras else {}),
                   "sourceRevision": pin["sourceRevision"],
                   "runtimeSha256": pin["files"]["dbce_shim.dll"],
                   "controls": shared_controls_contract()}


def write_receipts(output, receipt):
    """Same private receipt beside its consumer, never in its own hash inventory."""
    data = json.dumps(receipt, indent=2) + "\n"
    (output / "build-receipt.json").write_text(data)
    if "privateSharedInput" in receipt:
        (output / "payload/build-receipt.json").write_text(data)


def validate_shared_consumer(image, description, extras=False):
    if description.encode() + b"\0" not in image or b"[fzero-shim]" not in image:
        raise ValueError("Private shared consumer lacks the clean linked source stamp or shim adapter")
    if extras and (shared_extras_contract()["declaration"].encode("ascii") + b"\0" not in image or b"SharedExtras\0" not in image):
        raise ValueError("Private shared extras consumer lacks the linked declaration or opt-in flag")


def stage(build, output, private_shared_input=False, private_controls_setup=False):
    if private_controls_setup and not private_shared_input:
        raise ValueError("Private controls setup requires the owned private Shared consumer")
    cache = {}
    for line in (build / "CMakeCache.txt").read_text().splitlines():
        match = re.match(r"([^:#]+):[^=]+=(.*)", line)
        if match:
            cache[match[1]] = match[2]
    if cache.get("FZERO_DELUXE_GEN_DIR") or cache.get("FZERO_DELUXE_DATA_FILE"):
        raise ValueError("Stock-only build required; BS-generated module excluded")
    revision, tree = git(ROOT, "rev-parse", "HEAD"), git(ROOT, "rev-parse", "HEAD^{tree}")
    if git(ROOT, "status", "--porcelain", "--untracked-files=no"):
        raise ValueError("Commit tested source before the release build")
    header = (build / "generated/fzero_build.h").read_text()
    description = git(ROOT, "describe", "--always", "--dirty", "--abbrev=12")
    if f'#define FZERO_SOURCE_REVISION "{description}"' not in header:
        raise ValueError("Build source stamp is stale or dirty; reconfigure/rebuild")
    version = (ROOT / "VERSION").read_text().strip()
    files = {name: (build / name).read_bytes() for name in
             ("FZeroSNESRecomp.exe", "FZeroSNESRecompHeadless.exe")}
    if version.encode() not in files["FZeroSNESRecomp.exe"]:
        raise ValueError("Build does not contain product version")
    if b"BSDELX1" in files["FZeroSNESRecomp.exe"]:
        raise ValueError("Embedded private BS module excluded")
    links = dict((line.split()[3], line.split()[1]) for line in git(ROOT, "ls-files", "--stage", "snesrecomp", "recomp-ui").splitlines())
    dependencies = {}
    for name, key in (("snesrecomp", "SNESRECOMP_ROOT"), ("recomp-ui", "RECOMP_UI_ROOT")):
        root = Path(cache[key]).resolve()
        if git(root, "rev-parse", "HEAD") != links[name] or git(root, "status", "--porcelain", "--untracked-files=no"):
            raise ValueError(f"Dependency {name} does not match clean gitlink")
        dependencies[name] = root
        files[f"licenses/{name}.txt"] = (root / "LICENSE").read_bytes()
    files["licenses/imgui.txt"] = (dependencies["recomp-ui"] / "src/third_party/imgui/LICENSE.txt").read_bytes()
    files['licenses/launcher-images.txt'] = (dependencies['recomp-ui'] / 'assets/common/img/NOTICE.md').read_bytes()
    files['licenses/launcher-fonts.txt'] = (dependencies['recomp-ui'] / 'assets/common/fonts/NOTICE.md').read_bytes()
    files['licenses/flag-font-OFL.txt'] = (ROOT / OFL_FILES['NotoSansSymbols2-Regular.ttf'][0]).read_bytes()
    files["licenses/wheel-toolkit.txt"] = (ROOT / "lib/toolkit/LICENSE.txt").read_bytes()
    files["WheelFfb.dll"] = (ROOT / "lib/toolkit/native/WheelFfb.dll").read_bytes()
    for name in UI_ASSETS:
        files[name] = (build / name).read_bytes()
        if name.endswith(".ttf"):
            source = dependencies["recomp-ui"] / "assets/common/fonts" / Path(name).name
            if files[name] != source.read_bytes():
                raise ValueError("Unpinned launcher font")
            files["licenses/" + Path(name).stem + ".txt"] = font_notice(files[name], Path(name).name)
        else:
            relative = "assets/consoles/snes/img/pad.tga" if name.endswith("/pad.tga") else "assets/common/img/" + Path(name).name
            if files[name] != (dependencies["recomp-ui"] / relative).read_bytes():
                raise ValueError("Unpinned launcher image")
    for name in git(ROOT, "ls-files", "assets/shaders").splitlines():
        files[name] = (ROOT / name).read_bytes()
    shared = None
    if private_shared_input:
        extra, shared = private_shared_files(build)
        validate_shared_consumer(files["FZeroSNESRecomp.exe"], description, shared.get("extrasAbi") == 1)
        files.update(extra)
    pending = [n for n in files if n.lower().endswith((".exe", ".dll"))]
    while pending:
        name = pending.pop()
        for imported in pe_imports(files[name]):
            if imported in SYSTEM_DLLS or imported.startswith(("api-ms-", "ext-ms-")):
                continue
            existing = next((n for n in files if n.lower() == imported), None)
            if existing:
                continue
            source = next((p for p in build.glob("*.dll") if p.name.lower() == imported), None)
            if source is None:
                raise ValueError(f"Runtime import absent from build: {imported}")
            # Extra third-party runtimes require reviewed notices; don't infer terms.
            notice = build / "licenses" / (source.stem + ".txt")
            files["licenses/" + source.stem + ".txt"] = notice.read_bytes()
            files[source.name] = source.read_bytes()
            pending.append(source.name)
    setup = None
    if private_controls_setup:
        extra, setup = setup_payload(build / "controls-setup", files["FZeroSNESRecomp.exe"])
        files.update(extra) # Existing portable bundle already owns its runtime/notices.
    receipt = {"schema": "dbce.fzero-build", "version": 1, "productVersion": version,
               "sourceRevision": revision, "sourceTree": tree, "dirty": False,
               "bsDeluxe": False, "dependencies": links,
               "files": {n: digest(d) for n, d in sorted(files.items())}}
    if shared is not None:
        receipt["privateSharedInput"] = shared
    if setup is not None:
        receipt["privateControlsSetup"] = setup
    output.mkdir(parents=True, exist_ok=False)
    for name, data in files.items():
        path = output / "payload" / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    write_receipts(output, receipt)
    return output / "build-receipt.json"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--private-shared-input", action="store_true",
                        help="Stage pinned private-only input DLL; public packager refuses this receipt")
    parser.add_argument("--private-controls-setup", action="store_true",
                        help="Stage the exact qualified portable setup bundle with the private Shared consumer")
    args = parser.parse_args()
    try:
        print(stage(args.build.resolve(), args.output.resolve(), args.private_shared_input, args.private_controls_setup))
    except (ValueError, OSError, KeyError, struct.error, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error))
