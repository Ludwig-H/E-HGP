#!/usr/bin/env python3
"""Create-only terminal export / host stub / SAN / strict NVCC captures.

NVCC mode compiles and links only. No device, GCP or network execution here.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parent
BASE = ROOT.parent
REPO = BASE.parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def common_closure(pins):
    """Conservative full shared source tree, not only currently used headers."""
    return {name: digest for name, digest in pins.items() if name.startswith("source/") or
        (len(Path(name).parts) == 1 and
         (Path(name).suffix in (".cpp", ".cu", ".cuh", ".hpp") or name == "nvcc_strict_host.py"))}


def fixture_binding(pins):
    result = common_closure(pins)
    for name, digest in pins.items():
        if name == "cuda_trial/export.cpp" or (name.startswith("cuda_trial/") and
                Path(name).suffix in (".hpp", ".cuh")):
            result[name] = digest
    for required in ("cuda_trial/export.cpp", "cuda_trial/reference.hpp", "cuda_trial/fixture_types.hpp"):
        if required not in result:
            raise RuntimeError("fixture authority source missing: " + required)
    return result


def verify_snapshot(root, pins):
    for name, digest in pins.items():
        relative = Path(name)
        if relative.is_absolute() or ".." in relative.parts:
            raise RuntimeError("invalid snapshot path")
        path = root / relative
        if path.is_symlink() or sha(path) != digest:
            raise RuntimeError("snapshot drift: " + name)


def closed_capture(data):
    return data.get("status") == "passed" and data.get("sources_stable") is True and \
        isinstance(data.get("sources_before"), dict) and data["sources_before"] == data.get("sources_after")


def compatible_prerequisite(data, shared):
    return closed_capture(data) and common_closure(data["sources_before"]) == shared


def compatible_export(data, binding, fixture_hash):
    return closed_capture(data) and data.get("mode") == "export" and \
        data.get("snapshot_stable") is True and data.get("fixture_sha256") == fixture_hash and \
        data.get("fixture_binding") == binding and fixture_binding(data["sources_before"]) == binding


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True)
    parser.add_argument("--mode", required=True, choices=("export", "stub", "san", "nvcc"))
    parser.add_argument("--fixtures", type=Path)
    args = parser.parse_args()
    out = ROOT / args.out
    out.mkdir(exist_ok=False)
    sources = sorted(path for path in (BASE / "source").rglob("*") if path.is_file())
    sources += sorted(path for path in BASE.iterdir() if path.is_file() and
        (path.suffix in (".cpp", ".cu", ".cuh", ".hpp", ".py") or
         path.name in ("baseline_pins.json", "ownerfix_import.json")))
    sources += sorted(path for path in ROOT.iterdir() if path.is_file())
    before = {str(path.relative_to(BASE)): sha(path) for path in sources}
    snapshot = out / "source_snapshot"
    for path in sources:
        target = snapshot / path.relative_to(BASE)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    receipt = {"status": "running", "mode": args.mode, "sources_before": before,
        "commands": [], "device_executed": False, "gcp_used": False,
        "base_commit": "c03f6be8488453486b112811071827a96303ec86"}
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    receipt["sanitizer_environment"] = {key: env[key] for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS")}

    def save():
        (out / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")

    def run(name, argv, expected=0):
        start = time.time_ns()
        with (out / (name + ".stdout")).open("wb") as stdout, (out / (name + ".stderr")).open("wb") as stderr:
            done = subprocess.run(argv, cwd=REPO, env=env, stdout=stdout, stderr=stderr)
        receipt["commands"].append({"name": name, "argv": argv, "exit_code": done.returncode,
            "expected_exit_code": expected, "started_ns": start, "ended_ns": time.time_ns(),
            "stdout_sha256": sha(out / (name + ".stdout")), "stderr_sha256": sha(out / (name + ".stderr"))})
        save()
        print(name, done.returncode, flush=True)
        if done.returncode != expected:
            raise RuntimeError(name + " unexpected exit")

    try:
        shared = common_closure(before)
        binding = fixture_binding(before)
        verify_snapshot(snapshot, before)
        receipt["shared_source_closure"] = shared
        receipt["fixture_binding"] = binding
        if args.mode == "export":
            if args.fixtures:
                raise RuntimeError("export does not accept imported fixtures")
            prerequisites = {}
            for relative in ("o2_r1/receipt.json", "san_root_r1/receipt.json",
                "t2_trial/o2_r1/receipt.json", "t2_trial/san_root_r1/receipt.json",
                "guards_trial/o2_r1/receipt.json", "guards_trial/san_root_r1/receipt.json"):
                path = BASE / relative
                data = json.loads(path.read_text())
                if not compatible_prerequisite(data, shared):
                    raise RuntimeError("ownerfix prerequisite not closed: " + relative)
                verify_snapshot(path.parent / "source_snapshot", shared)
                prerequisites[relative] = sha(path)
            receipt["ownerfix_prerequisites"] = prerequisites
            run("provenance_normal", ["python3", "-B", str(snapshot / "cuda_trial/provenance_gate.py")])
            run("provenance_optimized", ["python3", "-B", "-O", str(snapshot / "cuda_trial/provenance_gate.py")])
            run("compiler", ["g++", "--version"])
            run("compile", ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
                "-isystem", str(REPO / "build/v7_boost_gate/extracted/usr/include"), "-MMD", "-MF", str(out / "gate.d"),
                str(snapshot / "cuda_trial/export.cpp"), "-o", str(out / "export_gate")])
            receipt["binary_sha256"] = sha(out / "export_gate")
            run("export", [str(out / "export_gate"), "--export"])
            summary = json.loads((out / "export.stderr").read_text())
            if summary.get("status") != "passed" or summary.get("clouds") != 14 or \
                    summary.get("all_declared_facets_nominal") is not True or \
                    summary.get("silently_filtered_failures") != 0 or \
                    summary.get("large_ordinals") != summary.get("requests") or \
                    any(type(summary.get(key)) is not int or summary[key] <= 0 for key in
                        ("requests", "trace_rows", "Gram_checked_rows", "q2", "q3", "q4", "extra_shells",
                         "strict_steps", "same_radius_steps", "intruder_queries")) or \
                    summary["trace_rows"] != summary["Gram_checked_rows"]:
                raise RuntimeError("export nonvacuity or accounting failed")
            shutil.copy2(out / "export.stdout", out / "terminal_fixtures.inc")
            receipt["fixture_sha256"] = sha(out / "terminal_fixtures.inc")
            receipt["export_summary"] = summary
            run("unknown", [str(out / "export_gate"), "--unknown"], 2)
            run("missing_arg", [str(out / "export_gate")], 2)
        else:
            if args.fixtures is None:
                raise RuntimeError("a closed export directory is required via --fixtures")
            exported = args.fixtures.resolve()
            provenance = json.loads((exported / "receipt.json").read_text())
            fixture = exported / "terminal_fixtures.inc"
            if not compatible_export(provenance, binding, sha(fixture)):
                raise RuntimeError("unqualified or changed fixture export")
            verify_snapshot(exported / "source_snapshot", binding)
            shutil.copy2(fixture, snapshot / "cuda_trial/terminal_fixtures.inc")
            receipt["fixture_provenance"] = {"path": str(exported), "receipt_sha256": sha(exported / "receipt.json"),
                "fixture_sha256": sha(fixture), "summary": provenance["export_summary"]}
            gate = snapshot / "cuda_trial/device_gate.cu"
            if args.mode == "nvcc":
                toolkit = REPO / "build/v7_nvcc_pedantic_20260910/toolkit"
                run("compiler", [str(toolkit / "bin/nvcc"), "--version"])
                run("compile_link", [str(toolkit / "bin/nvcc"), "-std=c++20", "-O2", "--gpu-architecture=sm_120",
                    "-fmad=false", "--expt-relaxed-constexpr", "-Xcompiler=-pthread,-Wall,-Wextra,-Wpedantic,-Werror",
                    "-ccbin", str(snapshot / "nvcc_strict_host.py"), "-L" + str(toolkit / "lib"),
                    "-MMD", "-MF", str(out / "gate.d"), str(gate), "-o", str(out / "device_gate")])
                receipt["binary_sha256"] = sha(out / "device_gate")
            else:
                flags = ["-O2"] if args.mode == "stub" else ["-O1", "-g", "-fno-omit-frame-pointer",
                    "-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
                run("compiler", ["g++", "--version"])
                run("compile", ["g++", "-x", "c++", "-DMHGP7_FAKE_DEVICE", "-std=c++20", *flags,
                    "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-MMD", "-MF", str(out / "gate.d"),
                    str(gate), "-o", str(out / "stub_gate")])
                receipt["binary_sha256"] = sha(out / "stub_gate")
                run("selftest", [str(out / "stub_gate"), "--selftest"])
                run("unknown", [str(out / "stub_gate"), "--unknown"], 2)
                run("missing_arg", [str(out / "stub_gate")], 2)
        receipt["status"] = "passed"
    except Exception as error:
        receipt["status"] = "failed"
        receipt["error"] = str(error)
    finally:
        receipt["sources_after"] = {str(path.relative_to(BASE)): sha(path) for path in sources}
        receipt["sources_stable"] = before == receipt["sources_after"]
        if not receipt["sources_stable"]:
            receipt["status"] = "failed"
        try:
            verify_snapshot(snapshot, before)
            if args.mode != "export" and "fixture_provenance" in receipt:
                if sha(snapshot / "cuda_trial/terminal_fixtures.inc") != receipt["fixture_provenance"]["fixture_sha256"]:
                    raise RuntimeError("consumed fixture drift")
            receipt["snapshot_stable"] = True
        except Exception as error:
            receipt["snapshot_stable"] = False
            receipt["status"] = "failed"
            receipt["snapshot_error"] = str(error)
        save()
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
