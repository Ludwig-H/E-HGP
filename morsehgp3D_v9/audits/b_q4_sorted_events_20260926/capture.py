#!/usr/bin/env python3
"""Capture a fresh standalone CPU experiment; never starts a cloud resource."""
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


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    out = HERE / "results"
    out.mkdir(exist_ok=False)
    build = Path(tempfile.mkdtemp(prefix="mhgp9-q4-sorted-events-"))
    source = HERE / "sorted_probe.cpp"
    dep = subprocess.run(["g++", "-std=c++20", "-MM", str(source)], cwd=ROOT,
                         check=True, text=True, capture_output=True)
    dependencies = [Path(p).resolve() for p in shlex.split(dep.stdout.split(":", 1)[1].replace("\\\n", " "))]
    dependencies.append(Path(__file__).resolve())
    hashes = {str(p.relative_to(ROOT)): sha(p) for p in dependencies}
    m = {"schema": "audit_q4_sorted_events_v1", "scope": "one_synthetic_family_not_product_T1",
         "base_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
         "build": str(build), "source_hashes_before": hashes,
         "machine": list(os.uname()), "gcp_used": False, "commands": []}

    def run(label, argv, expected=0):
        started = time.monotonic()
        p = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True)
        (out / (label + ".stdout")).write_text(p.stdout)
        (out / (label + ".stderr")).write_text(p.stderr)
        m["commands"].append({"label": label, "argv": argv, "expected": expected,
                              "returncode": p.returncode, "wall_seconds": time.monotonic() - started,
                              "stdout_sha256": sha(out / (label + ".stdout")),
                              "stderr_sha256": sha(out / (label + ".stderr"))})
        (out / "MANIFEST.json").write_text(json.dumps(m, indent=2) + "\n")
        if p.returncode != expected:
            raise RuntimeError(label + " failed")
        return p.stdout

    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    release = str(build / "sorted_release")
    sanitize = str(build / "sorted_sanitize")
    run("gxx", ["g++", "--version"])
    run("clangxx", ["clang++", "--version"])
    run("compile_release", ["g++", *flags, "-O3", "-DNDEBUG", str(source), "-o", release])
    run("compile_sanitize", ["clang++", *flags, "-O1", "-g", "-fsanitize=address,undefined",
                             "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", str(source), "-o", sanitize])
    normal = json.loads(run("selftest_release", [release, "--selftest"]))
    san = json.loads(run("selftest_sanitize", [sanitize, "--selftest"]))
    if normal != san or normal.get("status") != "pass":
        raise RuntimeError("selftests differ")
    run("bad_option", [release, "--unknown"], 2)
    run("bad_size", [release, "--bench", "12"], 1)
    for n in (8000, 16000, 32000):
        run("bench_" + str(n), [release, "--bench", str(n)])
    m["source_hashes_after"] = {str(p.relative_to(ROOT)): sha(p) for p in dependencies}
    if m["source_hashes_after"] != hashes:
        raise RuntimeError("sources changed during capture")
    m["binary_hashes"] = {release: sha(Path(release)), sanitize: sha(Path(sanitize))}
    m["status"] = "completed"
    (out / "MANIFEST.json").write_text(json.dumps(m, indent=2) + "\n")
    print(json.dumps({"status": "completed", "commands": len(m["commands"]), "selftest": normal}))


if __name__ == "__main__":
    main()
