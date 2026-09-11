"""Compile and capture an audit gate in a fresh directory under audits only."""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess


PACKET = Path(__file__).resolve().parent
AUDITS = PACKET.parent
ROOT = AUDITS.parents[1]
DEPENDENCY = ROOT / "morsehgp3D_v7/receipts/atlas_graph_full_20260911"
DEPENDENCY_SHA = "bb4a482385a7f75537c38d41397d4410d02c04f2a899c6377df79515caf0f9bc"
PARENT_SHA = "341c8c228a9d008084010db7b010adac77beefba123fa8a59d1d7c9023652013"


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def require(value: bool, why: str) -> None:
    if not value:
        raise RuntimeError(why)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--san", action="store_true")
    args = parser.parse_args()
    out = args.out.resolve()
    require(out.is_relative_to(AUDITS) and not out.exists(), "output must be fresh under audits")
    require(sha((DEPENDENCY / "MANIFEST.json").read_bytes()) == DEPENDENCY_SHA,
            "dependency manifest differs")
    require(sha((DEPENDENCY.parent / "rank_atlas_20260911/manifest.json").read_bytes()) == PARENT_SHA,
            "dependency parent differs")
    out.mkdir(parents=True)
    (out / "tmp").mkdir()
    environment = os.environ.copy()
    environment["TMPDIR"] = str(out / "tmp")
    if args.san:
        environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
        environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    gate = PACKET / "historical_export_gate.cpp"
    own = [gate, PACKET / "record.py"]
    receipt = {
        "status": "preparing", "recorded_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "mode": "SAN" if args.san else "O2", "out": str(out), "cwd": str(ROOT),
        "dependency_manifest": DEPENDENCY_SHA, "dependency_parent_manifest": PARENT_SHA,
        "own_sources_before": {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in own},
        "environment": {k: environment[k] for k in ["TMPDIR", "ASAN_OPTIONS", "UBSAN_OPTIONS"]
                        if k in environment},
        "commands": [], "gcp_used": False, "performance_claim": False,
        "scope": "Audit C++ gate; borrowed sealed private extraction/history and actual Builder reference. No active product change or parallel-export speed claim.",
    }

    def save() -> None:
        (out / "receipt.json").write_text(json.dumps(receipt, ensure_ascii=False, indent=2) + "\n")

    def run(name: str, argv: list[str], expected: int = 0) -> subprocess.CompletedProcess[str]:
        result = subprocess.run(argv, cwd=ROOT, env=environment, capture_output=True, text=True,
                                timeout=300)
        receipt["commands"].append({"name": name, "argv": argv, "expected_code": expected,
                                    "exit_code": result.returncode,
                                    "stdout": result.stdout, "stderr": result.stderr})
        save()
        print(json.dumps({"command": name, "exit_code": result.returncode}), flush=True)
        require(result.returncode == expected, name + " unexpected exit code")
        return result

    try:
        dep = out / "dependency"
        run("extract_dependency", ["python3", "-B", str(DEPENDENCY / "verify.py"), "--extract", str(dep)])
        cpp_root = dep / "build/v7_atlas_graph_20260911"
        graph_header = dep / "build/v7_graph_full_20260911/graph_full.hpp"
        compiler_name = shutil.which("g++")
        require(compiler_name is not None, "g++ unavailable")
        compiler = str(Path(compiler_name).resolve())
        receipt["compiler_sha256_before"] = sha(Path(compiler).read_bytes())
        run("compiler", [compiler, "--version"])
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread"]
        flags += (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                   "-fno-pie", "-no-pie"] if args.san else ["-O2"])
        flags += ["-I", str(cpp_root), "-I", str(cpp_root / "source/morsehgp3D_v7"),
                  '-DMHGP7_AUDIT_GRAPH_FULL_HEADER="' + str(graph_header) + '"']
        command = [compiler, *flags, str(gate)]

        def dependencies(name: str) -> dict[str, str]:
            result = run(name, [*command, "-MM", "-MT", "audit_gate"])
            require(result.stderr == "" and ":" in result.stdout, "dependency diagnostics")
            paths = shlex.split(result.stdout.replace("\\\n", " ").split(":", 1)[1])
            pins = {}
            for raw in paths:
                path = Path(raw).resolve()
                if path == gate:
                    logical = "audit/historical_export_gate.cpp"
                else:
                    require(path.is_relative_to(dep), "project dependency outside sealed extraction")
                    logical = path.relative_to(dep).as_posix()
                pins[logical] = sha(path.read_bytes())
            require(len(pins) > 10, "empty project dependency closure")
            return pins

        receipt["project_sources_before"] = dependencies("dependencies_before")
        executable = out / "historical_export_gate"
        result = run("compile", [*command, "-o", str(executable)])
        require(result.stdout == result.stderr == "", "compile diagnostics")
        receipt["binary_sha256_before"] = sha(executable.read_bytes())
        run("selftest", [str(executable), "--selftest"])
        run("unknown", [str(executable), "--unknown"], 2)
        run("missing", [str(executable)], 2)
        receipt["project_sources_after"] = dependencies("dependencies_after")
        receipt["own_sources_after"] = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in own}
        receipt["binary_sha256_after"] = sha(executable.read_bytes())
        receipt["compiler_sha256_after"] = sha(Path(compiler).read_bytes())
        require(receipt["project_sources_before"] == receipt["project_sources_after"], "project source changed")
        require(receipt["own_sources_before"] == receipt["own_sources_after"], "audit source changed")
        require(receipt["binary_sha256_before"] == receipt["binary_sha256_after"], "binary changed")
        require(receipt["compiler_sha256_before"] == receipt["compiler_sha256_after"], "compiler changed")
        receipt["status"] = "completed"
    except Exception as error:
        receipt["status"] = "failed"
        receipt["failure"] = str(error)
        raise
    finally:
        save()


if __name__ == "__main__":
    main()
