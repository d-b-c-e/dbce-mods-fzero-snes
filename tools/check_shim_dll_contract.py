"""Check a private x64 shim artifact without opening input or force providers."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path


def check(dll_path, receipt_path, source):
    if os.name != "nt" or ctypes.sizeof(ctypes.c_void_p) != 8:
        raise ValueError("This check requires a 64-bit Windows Python process")
    dll_path = dll_path.resolve(strict=True)
    receipt = json.loads(receipt_path.read_text(encoding="utf-8-sig"))
    digest = hashlib.sha256(dll_path.read_bytes()).hexdigest()
    if (receipt.get("arch") != "x64" or receipt.get("ordinaryOnly") is not False or
            receipt.get("crt") != "MT" or receipt.get("sourceChanged") is not False or
            receipt.get("changedFiles") != [] or receipt.get("toolkitCommit") != source or
            receipt.get("sha256", "").lower() != digest):
        raise ValueError("Artifact does not match the clean x64/MT source receipt")

    # These are the consumer's required base/extension calls. Opening, ticking,
    # recording and stopping providers are deliberately never invoked here.
    required = (
        "dbce_shim_abi_version", "dbce_shim_frame_size", "dbce_shim_open_args_size",
        "dbce_shim_action_name", "dbce_shim_open2", "dbce_shim_tick",
        "dbce_shim_record", "dbce_shim_stop", "dbce_shim_profile_info",
        "dbce_shim_extras_abi_version", "dbce_shim_extras_frame_size",
        "dbce_shim_open_extras", "dbce_shim_tick_extras", "dbce_shim_extras_info",
    )
    if not set(required).issubset(receipt.get("exports", [])):
        raise ValueError("Receipt is missing a consumer-required export")
    # The DLL directory and Windows default search directories only.
    library = ctypes.CDLL(str(dll_path), winmode=0x00000100 | 0x00001000)
    for name in required:
        getattr(library, name)
    sizes = {}
    expected = {"dbce_shim_abi_version": 2, "dbce_shim_frame_size": 296,
                "dbce_shim_open_args_size": 128, "dbce_shim_extras_abi_version": 1,
                "dbce_shim_extras_frame_size": 296}
    for name, value in expected.items():
        call = getattr(library, name)
        call.argtypes = []
        call.restype = ctypes.c_int32
        sizes[name] = call()
        if sizes[name] != value:
            raise ValueError(f"{name} differs: {sizes[name]} != {value}")
    action_name = library.dbce_shim_action_name
    action_name.argtypes = [ctypes.c_int32]
    action_name.restype = ctypes.c_char_p
    actions = "steer throttle brake clutch handbrake shiftUp shiftDown neutral reverse confirm back start select navUp navDown navLeft navRight camera lookBack reset horn modMenu muteFfb gear1 gear2 gear3 gear4 gear5 gear6 gear7 gear8".split()
    for index, name in enumerate(actions):
        if action_name(index) != name.encode("ascii"):
            raise ValueError(f"Common action order differs at {index}: {name}")
    if action_name(-1) is not None or action_name(len(actions)) is not None:
        raise ValueError("Common action table is not bounded to 31 actions")
    return {"pass": True, "dll": str(dll_path), "sha256": digest,
            "source": source, "requiredExports": list(required), "layouts": sizes,
            "commonActions": actions, "providerCalls": 0,
            "scope": "pure ABI/layout/action-name checks; no input/force provider opened"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dll", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--source", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("Output must be a new path")
    result = check(args.dll, args.receipt, args.source)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print("PASS private F-Zero shim artifact contract; 14 exports, 5 layouts, 31 actions; no provider calls")


if __name__ == "__main__":
    main()
