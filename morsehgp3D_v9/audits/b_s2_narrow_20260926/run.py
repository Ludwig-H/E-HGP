#!/usr/bin/env python3
"""Bounded CPU-only S2 arithmetic experiment and live readback; no GPU claim."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check(data: dict) -> None:
    if data.get("schema") != "mhgp9_audit_b_s2_narrow_v1":
        raise ValueError("schema")
    if data.get("status") != "completed" or not data.get("commands"):
        raise ValueError("capture incomplete")
    for entry in data["files"]:
        if sha(Path(entry["path"])) != entry["sha256"]:
            raise ValueError("live source or binary changed: " + entry["path"])
    gates = 0
    open_sum = rejected_sum = fallback_sum = 0
    for command in data["commands"]:
        if command["returncode"] != 0 or command["stderr"]:
            raise ValueError("command failure")
        row = json.loads(command["stdout"])
        if row["status"] != "pass":
            raise ValueError("nonpass")
        if "numeric_checks" in row:
            gates += 1
            if row["guard_checks"] != 19 or row["numeric_checks"] != 116049:
                raise ValueError("gate work changed")
            if (row["narrow"], row["wide"], row["h_rejected"]) != (58098, 57930, 83972):
                raise ValueError("gate populations changed")
        else:
            if row["queries"] != (row["pair_mass"] + row["stride"] - 1) // row["stride"]:
                raise ValueError("sample mass")
            if row["open"] + row["rejected"] != row["queries"]:
                raise ValueError("sample outputs")
            if row["visits"] != row["eligible"] + row["fallback"] + row["h_rejected"]:
                raise ValueError("sample node partition")
            if min(row[x] for x in ("queries", "eligible", "h_rejected")) <= 0:
                raise ValueError("sample nonvacuity")
            open_sum += row["open"]
            rejected_sum += row["rejected"]
            fallback_sum += row["fallback"]
            if len(row["baseline_s"]) != 3 or len(row["candidate_s"]) != 3:
                raise ValueError("sample repeats")
            if any(t <= 0 for t in row["baseline_s"] + row["candidate_s"]):
                raise ValueError("sample time")
    if gates != 2:
        raise ValueError("Release and sanitizer gate required")
    if min(open_sum, rejected_sum, fallback_sum) <= 0:
        raise ValueError("campaign nonvacuity")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--read", type=Path)
    parser.add_argument("--capture", type=Path)
    parser.add_argument("--sample", type=Path)
    parser.add_argument("--gate", type=Path)
    parser.add_argument("--san-gate", type=Path)
    parser.add_argument("--lib", type=Path)
    parser.add_argument("--input", action="append", type=Path, default=[])
    parser.add_argument("--families", nargs="+", choices=("uniform", "terrain", "clusters"), default=["uniform", "terrain", "clusters"])
    args = parser.parse_args()
    if args.read:
        data = json.loads(args.read.read_text())
        check(data)
        print(json.dumps({"status": "pass", "commands": len(data["commands"]), "files": len(data["files"])}))
        return
    if not all((args.capture, args.sample, args.gate, args.san_gate, args.lib)):
        parser.error("capture, sample, gate, san-gate, lib required")
    if args.capture.exists():
        raise ValueError("refuse existing capture")
    files = sorted((ROOT / "morsehgp3D_v9/src/gen").rglob("*.hpp"))
    files += sorted((ROOT / "morsehgp3D_v9/src/gen").rglob("*.cpp"))
    files += [ROOT / "morsehgp3D_v9/src/gpu/witness_filter.hpp", ROOT / "morsehgp3D_v9/src/gpu/flat_index.hpp",
              ROOT / "morsehgp3D_v9/tests/gen/front_fixtures.hpp", ROOT / "morsehgp3D_v9/CMakeLists.txt"]
    files += [HERE / x for x in ("narrow_pair.hpp", "gate.cpp", "sample.cpp", "run.py")]
    files += [args.sample, args.gate, args.san_gate, args.lib] + args.input
    files = [x.resolve() for x in files]
    data = {"schema": "mhgp9_audit_b_s2_narrow_v1", "status": "running", "contract_certified": False,
            "gcp_used": False, "platform": platform.platform(), "commands": [],
            "git_head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
            "files": [{"path": str(p), "sha256": sha(p)} for p in files],
            "earlier_setup_failures": ["CMake and gate compile initially lacked Boost include path; no execution",
                "sample initial -Werror compile refused unsigned-to-signed list narrowing and unused nodiscard; no execution",
                "capture.json preserved failed: per-case nonvacuity required both arithmetic branches and both mask outcomes even when one absent from sample; numeric code unchanged by correction"]}
    commands = [[str(args.gate)], [str(args.san_gate)]]
    commands += [[str(args.sample), "synthetic", family, str(n), "1024", "5"]
                 for family in args.families for n in (8000, 16000, 32000)]
    commands += [[str(args.sample), "file", str(path), "1024", str(k)] for path in args.input for k in (5, 10)]
    args.capture.parent.mkdir(parents=True, exist_ok=True)
    try:
        for command in commands:
            started = time.monotonic()
            p = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, check=False)
            data["commands"].append({"argv": command, "returncode": p.returncode,
                                     "wall_s": time.monotonic()-started, "stdout": p.stdout, "stderr": p.stderr})
            args.capture.write_text(json.dumps(data, indent=2) + "\n")
            print(json.dumps({"argv": command, "returncode": p.returncode, "stdout": p.stdout}), flush=True)
            if p.returncode:
                raise ValueError("command failed")
        data["status"] = "completed"
        check(data)
    except BaseException:
        data["status"] = "failed"
        raise
    finally:
        args.capture.write_text(json.dumps(data, indent=2) + "\n")


if __name__ == "__main__":
    main()
