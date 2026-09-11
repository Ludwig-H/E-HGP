#!/usr/bin/env python3
"""Portable read-only evidence gate, never a compiler or geometry invocation."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
PRIVATE_SHA = "fb0f7d3b4daddc650ee1a0ad85f749f22fd1c2e01e557650e17b9a79e02897f1"


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def digest(content):
    return hashlib.sha256(content).hexdigest()


def safe(name):
    return bool(name) and not Path(name).is_absolute() and ".." not in Path(name).parts


def main():
    need(len(sys.argv) == 1, "no arguments")
    manifest = read_json(ROOT / "manifest.json")
    need(manifest["GCP_used"] is False and manifest["public_status"] == "not_claimed" and
         manifest["integration_decision"] == "not_integrated", "public scope")
    actual = {path.relative_to(ROOT).as_posix() for path in ROOT.rglob("*") if path.is_file()}
    need(actual == set(manifest["files"]) | {"manifest.json"}, "unexpected or missing public files")
    for name, metadata in manifest["files"].items():
        need(safe(name), "unsafe public path")
        path = ROOT / name
        need(path.is_file() and not path.is_symlink(), "missing/symlink " + name)
        data = path.read_bytes(); data.decode("utf-8")
        need(not data.startswith(b"\x7fELF"), "ELF payload forbidden")
        need(len(data) == metadata["size"] and digest(data) == metadata["sha256"], "public hash " + name)
    original_bytes = (ROOT / "capture_manifest.json").read_bytes()
    need(digest(original_bytes) == PRIVATE_SHA, "private manifest changed")
    original = json.loads(original_bytes)
    need(original["GCP_used"] is False and original["no_active_source_change"] is True and
         original["public_status"] == "not_claimed", "original scope")
    mapping = read_json(ROOT / "storage_map.json")
    need(set(mapping) == set(original["files"]), "incomplete logical mapping")
    binary_pins = read_json(ROOT / "binaries.json")
    need(len(binary_pins) == 11 and not (set(binary_pins) & set(mapping)), "ELF metadata separation")
    with tempfile.TemporaryDirectory(prefix="mhgp7-incremental-full-evidence-") as temporary:
        restored = Path(temporary)
        for name, expected in original["files"].items():
            entry = mapping[name]
            need(safe(name) and safe(entry["storage"]), "unsafe logical path")
            need(entry["storage"] in manifest["files"] and entry["sha256"] == expected, "unsealed or changed mapping")
            data = (ROOT / entry["storage"]).read_bytes()
            need(digest(data) == expected and len(data) == entry["size"], "logical content mismatch " + name)
            target = restored / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        (restored / "MANIFEST.json").write_bytes(original_bytes)
        flags = ["-B"] + (["-O"] if sys.flags.optimize else [])
        result = subprocess.run([sys.executable, *flags, str(restored / "verify.py")],
                                check=False, capture_output=True, text=True)
        need(result.returncode == 0 and result.stderr == "", "private reader failed: " + result.stdout + result.stderr)
        report = json.loads(result.stdout)
        need(report["status"] == "verified_private_incremental_full" and report["qualification_commands"] == 24,
             "private qualification status")
        need(report["public_status"] == "not_claimed" and report["contract_qualified"] is False and
             report["GCP_used"] is False, "private claim scope")
        need(report["physical_totals_per_kind_per_build"] == dict(calls=216, complete=180, rejected=36,
             nodes=4932, parents=4208, contributions=3432), "physical nonvacuity")
        need(report["preserved_negative_attempts"] == ["san_r1", "failure_san_root_r2"], "negative attempts omitted")
        micro = report["micro"]
        need([row["n"] for row in micro] == [200, 400, 800], "micro sizes")
        for row in micro:
            need(row["speedup_claim"] is False and row["shared_host"] is True and row["all_matched"] is True,
                 "micro scope")
            need(int(row["candidate"]["new_calls"]) > int(row["baseline"]["new_calls"]) and
                 int(row["candidate"]["retained"]) > int(row["baseline"]["retained"]), "negative micro verdict lost")
        a, b = micro[-1]["baseline"], micro[-1]["candidate"]
        need(int(b["new_calls"]) - int(a["new_calls"]) == 366021 and
             int(b["retained"]) - int(a["retained"]) == 17340480 and
             int(b["requested_peak"]) - int(a["requested_peak"]) == -955584, "n800 negative receipt")
        need(int(micro[1]["candidate"]["requested_peak"]) > int(micro[1]["baseline"]["requested_peak"]), "n400 peak regression")
    print(json.dumps({"status": "passed", "logical_files": len(mapping), "public_files": len(manifest["files"]),
                      "paired_modes": ["cache", "no_cache", "static1", "static4"], "host_O2_SAN_verified": True,
                      "allocation_rejections": 1402, "micro_verdict": "not_integrated_copies_and_capacity",
                      "public_status": "not_claimed", "contract_qualified": False, "GCP_used": False,
                      "manifest_sha256": digest((ROOT / "manifest.json").read_bytes())}))


if __name__ == "__main__":
    main()
