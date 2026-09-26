#!/usr/bin/env python3
"""Capture this bounded CPU consumer experiment; never starts GCP."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import time


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    output = HERE / "results"
    output.mkdir(exist_ok=False)
    build = Path(tempfile.mkdtemp(prefix="mhgp9-census-payload-capture-"))
    source = HERE / "payload_probe.cpp"
    dep = subprocess.run(["g++", "-std=c++20", "-MM", str(source)], check=True,
                         text=True, capture_output=True, cwd=ROOT)
    dependencies = [Path(p).resolve() for p in shlex.split(dep.stdout.split(":", 1)[1].replace("\\\n", " "))]
    dependencies.append(Path(__file__).resolve())
    before = {str(p.relative_to(ROOT)): digest(p) for p in dependencies}
    manifest = {"base_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                "build": str(build), "source_sha256": before,
                "hostname": os.uname().nodename, "uname": list(os.uname()),
                "scope": "consumer_only_synthetic_reference_packet_cpu_one_thread",
                "gcp_used": False, "commands": []}

    def run(label, argv, expected=0):
        started = time.time()
        p = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True)
        (output / (label + ".stdout")).write_text(p.stdout)
        (output / (label + ".stderr")).write_text(p.stderr)
        manifest["commands"].append({"label": label, "argv": argv, "returncode": p.returncode,
                                     "expected": expected, "wall_seconds": time.time() - started})
        (output / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
        if p.returncode != expected:
            raise RuntimeError(label + " returned " + str(p.returncode))
        return p.stdout

    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    release = str(build / "payload_probe")
    sanitize = str(build / "payload_probe_sanitize")
    run("compiler_release", ["g++", "--version"])
    run("compiler_sanitize", ["clang++", "--version"])
    run("compile_release", ["g++", *flags, "-O3", "-DNDEBUG", str(source), "-o", release])
    run("compile_sanitize", ["clang++", *flags, "-O1", "-g", "-fsanitize=address,undefined",
                             "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", str(source), "-o", sanitize])
    normal = json.loads(run("selftest_release", [release, "--selftest"]))
    san = json.loads(run("selftest_sanitize", [sanitize, "--selftest"]))
    if normal != san or normal.get("status") != "pass":
        raise RuntimeError("selftests differ")
    run("bad_argument", [release, "--unknown"], expected=2)
    run("bad_size", [release, "--benchmark", "1", "1"], expected=1)
    rows = []
    for n in (8000, 16000, 32000):
        rows.extend(json.loads(line) for line in run("bench_" + str(n), [release, "--benchmark", str(n), "5"]).splitlines())
    after = {str(p.relative_to(ROOT)): digest(p) for p in dependencies}
    if before != after:
        raise RuntimeError("source_changed_during_capture")
    manifest["source_after_sha256"] = after
    manifest["binaries_sha256"] = {release: digest(Path(release)), sanitize: digest(Path(sanitize))}
    manifest["status"] = "completed"
    (output / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (output / "RESULTS.json").write_text(json.dumps({"selftest": normal, "measurements": rows}, indent=2) + "\n")
    print(json.dumps({"output": str(output), "selftest": normal, "measurements": len(rows)}))


if __name__ == "__main__":
    main()
