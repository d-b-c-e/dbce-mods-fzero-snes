"""Admit the existing portable F-Zero setup bundle at build/package time.

No helper execution. Runtime receives only the two bounded identity pins;
the complete manifest inventory is verified here once, outside game startup.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re

CONTRACT = "fzero-exact-process-exit-1"
REPORT_CONTRACT = "fzero-reviewed-browser-report-1"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def no_links(path):
    for item in (path, *path.parents):
        if item.is_symlink() or (getattr(item.stat(), "st_file_attributes", 0) & 0x400):
            raise ValueError("Setup bundle cannot traverse links")


def admit(root):
    root = Path(root).absolute()
    no_links(root)
    manifest_path = root / "bundle-manifest.json"
    no_links(manifest_path)
    if not manifest_path.is_file() or manifest_path.stat().st_size > 1024 * 1024:
        raise ValueError("Missing or oversized setup bundle manifest")
    with manifest_path.open("rb") as stream:
        raw = stream.read(1024 * 1024 + 1)
    if len(raw) > 1024 * 1024:
        raise ValueError("Oversized setup bundle manifest")
    manifest = json.loads(raw.decode("utf-8-sig"))
    if (manifest.get("schema") != 1 or manifest.get("kind") != "private-standalone-helper"
            or manifest.get("rid") != "win-x64" or manifest.get("selfContained") is not True
            or not re.fullmatch(r"[0-9a-f]{40}", manifest.get("sourceCommit", ""))
            or not isinstance(manifest.get("supportedGames"), list)
            or "f-zero" not in manifest["supportedGames"]
            or manifest.get("waitForProcessContract") != CONTRACT
            or manifest.get("waitForProcessTimeoutMs") != 60000):
        raise ValueError("Setup bundle does not declare the F-Zero exact-exit contract")
    report = manifest.get("report")
    report_qualified = (isinstance(report, dict) and report.get("contract") == REPORT_CONTRACT
                        and report.get("games") == ["f-zero"] and report.get("browserDraftOnly") is True)
    # An absent capability is an old supported setup bundle; malformed declarations refuse.
    if report is not None and not report_qualified:
        raise ValueError("Unsupported F-Zero reviewed report capability")
    entries = manifest.get("files")
    if not isinstance(entries, list) or not 1 <= len(entries) <= 4096:
        raise ValueError("Setup bundle has no bounded file inventory")
    files = {}
    for entry in entries:
        name = entry.get("path", "")
        path = PurePosixPath(name)
        if (not name or "\\" in name or ":" in name or path.is_absolute()
                or any(part in ("", ".", "..") for part in name.split("/"))
                or name.casefold() in files or name.casefold() == "bundle-manifest.json"):
            raise ValueError("Invalid or duplicate setup bundle path")
        target = root.joinpath(*path.parts)
        no_links(target)
        if (not target.is_file() or type(entry.get("bytes")) is not int
                or target.stat().st_size != entry["bytes"]
                or not re.fullmatch(r"[0-9a-f]{64}", entry.get("sha256", ""))):
            raise ValueError("Setup bundle file identity differs: " + name)
        # Package-time streaming hash; no file bytes are embedded in the game.
        hasher = hashlib.sha256()
        with target.open("rb") as stream:
            for chunk in iter(lambda: stream.read(65536), b""):
                hasher.update(chunk)
        actual = hasher.hexdigest()
        if actual != entry["sha256"]:
            raise ValueError("Setup bundle file hash differs: " + name)
        files[name.casefold()] = (name, target, entry)
    actual_files = set()
    for target in root.rglob("*"):
        no_links(target)
        if target.is_file() and target != manifest_path:
            actual_files.add(target.relative_to(root).as_posix().casefold())
    if actual_files != set(files):
        raise ValueError("Setup bundle inventory is not exact")
    exe = files.get("dbcemodsetup.exe")
    if not exe or exe[0] != "DbceModSetup.exe" or not 1 <= exe[2]["bytes"] <= 8 * 1024 * 1024:
        raise ValueError("Setup GUI apphost missing or exceeds runtime identity bound")
    return {"reportQualified": report_qualified, "sourceCommit": manifest["sourceCommit"], "manifestSha256": digest(raw),
            "manifestBytes": len(raw), "exeSha256": exe[2]["sha256"],
            "exeBytes": exe[2]["bytes"], "files": [item[0] for item in files.values()],
            "fileHashes": {item[0]: item[2]["sha256"] for item in files.values()}}


def payload(root, consumer):
    """Existing private stager: exact portable bytes and linked host identity."""
    root = Path(root)
    info = admit(root)
    if any(value.encode("ascii") + b"\0" not in consumer
           for value in (info["manifestSha256"], info["exeSha256"])):
        raise ValueError("Game binary is not bound to this controls setup bundle")
    result = {}
    for name, expected in {**info["fileHashes"], "bundle-manifest.json": info["manifestSha256"]}.items():
        data = (root / name).read_bytes()
        if digest(data) != expected:
            raise ValueError("Setup bundle changed during staging")
        result["controls-setup/" + name] = data
    metadata = {key: info[key] for key in ("sourceCommit", "manifestSha256", "manifestBytes", "exeSha256", "exeBytes")}
    metadata.update({"folder": "controls-setup", "game": "f-zero", "waitForProcessContract": CONTRACT,
                     "waitForProcessTimeoutMs": 60000})
    return result, metadata


def header(info=None):
    info = info or {}
    return ("// Generated build-time setup identity; no payload or runtime inventory.\n#pragma once\n"
            f"#define FZERO_SETUP_QUALIFIED {1 if info else 0}\n"
            f"#define FZERO_SETUP_REPORT_QUALIFIED {1 if info.get('reportQualified') else 0}\n"
            f'#define FZERO_SETUP_SOURCE_COMMIT "{info.get("sourceCommit", "")}"\n'
            f'#define FZERO_SETUP_MANIFEST_SHA256 "{info.get("manifestSha256", "")}"\n'
            f'#define FZERO_SETUP_EXE_SHA256 "{info.get("exeSha256", "")}"\n'
            f'#define FZERO_SETUP_MANIFEST_BYTES {info.get("manifestBytes", 0)}\n'
            f'#define FZERO_SETUP_EXE_BYTES {info.get("exeBytes", 0)}\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path)
    parser.add_argument("--header", type=Path, required=True)
    args = parser.parse_args()
    info = admit(args.bundle) if args.bundle else None
    args.header.parent.mkdir(parents=True, exist_ok=True)
    args.header.write_bytes(header(info).encode("utf-8"))


if __name__ == "__main__":
    main()
