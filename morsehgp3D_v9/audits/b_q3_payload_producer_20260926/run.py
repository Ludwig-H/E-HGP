#!/usr/bin/env python3
"""CPU audit capture of q3 interior-ID production; no product edit or GCP."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BOOST = Path("/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--data", type=Path, required=True)
    args = parser.parse_args()
    build = args.build.resolve()
    build.mkdir(exist_ok=True)
    output = args.output.resolve()
    output.mkdir(exist_ok=False)
    includes = ["-I", str(ROOT / "morsehgp3D_v9/src/gen"), "-I", str(BOOST)]
    source = HERE / "probe.cpp"
    exact = ROOT / "morsehgp3D_v9/src/gen/lanes/exact_ball.cpp"
    dep = subprocess.run(["g++", "-std=c++20", *includes, "-MM", str(source), str(exact)],
                         cwd=ROOT, text=True, capture_output=True, check=True)
    # -MM gives one target per translation unit; each is newline continued.
    dependencies = set()
    for line in dep.stdout.replace("\\\n", " ").splitlines():
        for item in shlex.split(line.split(":", 1)[1]):
            dependencies.add(Path(item).resolve())
    dependencies.update((HERE / name).resolve() for name in ("producer.hpp", "probe.cpp", "run.py", "readback.py"))

    def name(path):
        return str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)

    before = {name(path): sha(path) for path in sorted(dependencies)}
    manifest = {"schema": "audit_q3_payload_producer_v1", "scope": "cpu_portable_q3_range_not_L15_not_full",
                "gcp_used": False, "base_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                "source_before": before, "build": str(build), "commands": [], "input_files": {}, "status": "running"}

    def save():
        (output / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")

    def run(label, command, expected=0):
        record = {"label": label, "argv": command, "expected": expected, "status": "running"}
        manifest["commands"].append(record)
        save()
        started = time.monotonic()
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, env=env)
        (output / (label + ".stdout")).write_text(result.stdout)
        (output / (label + ".stderr")).write_text(result.stderr)
        record.update(status="completed", exit_code=result.returncode, wall_seconds=time.monotonic()-started)
        save()
        if result.returncode != expected:
            raise RuntimeError(f"{label}: unexpected return code {result.returncode}")
        return result

    try:
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", *includes]
        run("compiler_release", ["g++", "--version"])
        run("compiler_sanitize", ["clang++", "--version"])
        release = str(build / "producer_release")
        sanitize = str(build / "producer_sanitize")
        run("compile_release", ["g++", *flags, "-O3", "-DNDEBUG", str(source), str(exact), "-o", release])
        run("compile_sanitize", ["clang++", *flags, "-O1", "-g", "-fsanitize=address,undefined",
                                 "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", str(source), str(exact), "-o", sanitize])
        first = json.loads(run("selftest_release", [release, "--selftest"]).stdout)
        second = json.loads(run("selftest_sanitize", [sanitize, "--selftest"]).stdout)
        if first != second or first["status"] != "pass":
            raise RuntimeError("selftests differ")
        mutants = []
        for label, macro in (("omit_last", "AUDIT_Q3_PAYLOAD_MUTANT_OMIT_LAST"), ("rank_as_id", "AUDIT_Q3_PAYLOAD_MUTANT_RANK_AS_ID")):
            binary = str(build / ("mutant_" + label))
            run("compile_" + label, ["g++", *flags, "-O3", "-D" + macro, str(source), str(exact), "-o", binary])
            bad = run("kill_" + label, [binary, "--selftest"], 1)
            if bad.stderr.strip() != "cause=interior_ids_differ":
                raise RuntimeError("mutant did not die causally")
            mutants.append({"name": label, "binary": binary, "sha256": sha(Path(binary)), "cause": bad.stderr.strip()})
        run("bad_arguments", [release, "--unknown"], 2)
        run("bad_k", [release, "--file", "absent.u32le", "11"], 1)
        measurements = []
        for family in ("uniform", "terrain", "clusters"):
            for n in (8000, 16000, 32000):
                result = run(f"synthetic_{family}_{n}", [release, "--synthetic", str(n), family])
                measurements.extend(json.loads(line) for line in result.stdout.splitlines())
        for scene, k in (("00", 5), ("01", 5), ("02", 5), ("00", 10)):
            path = (args.data / ("scene_" + scene + "_grid/full.u32le")).resolve(strict=True)
            manifest["input_files"][str(path)] = sha(path)
            result = run(f"lidar_{scene}_K{k}", [release, "--file", str(path), str(k)])
            for line in result.stdout.splitlines():
                row = json.loads(line)
                row.update(scene=scene, input_sha256=manifest["input_files"][str(path)])
                measurements.append(row)
        manifest["source_after"] = {name(path): sha(path) for path in sorted(dependencies)}
        if before != manifest["source_after"]:
            raise RuntimeError("source changed during capture")
        for path, digest in manifest["input_files"].items():
            if sha(Path(path)) != digest:
                raise RuntimeError("input changed during capture")
        manifest.update(status="completed", binaries={release: sha(Path(release)), sanitize: sha(Path(sanitize))}, mutants=mutants)
        (output / "RESULTS.json").write_text(json.dumps({"selftest": first, "measurements": measurements}, indent=2) + "\n")
        manifest["outputs"] = {path.name: sha(path) for path in sorted(output.iterdir()) if path.name != "MANIFEST.json"}
    except BaseException as error:
        manifest.update(status="failed", error=repr(error))
        raise
    finally:
        save()
    print(json.dumps({"status": "completed", "selftest": first, "measurements": len(measurements)}))


if __name__ == "__main__":
    main()
