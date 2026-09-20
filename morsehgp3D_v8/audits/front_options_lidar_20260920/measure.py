#!/usr/bin/env python3
"""Paired public-API observations; fresh directory, all attempts retained."""
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def main():
    out = BASE / "capture"
    out.mkdir(exist_ok=False)
    binary = BASE / ".build/lidar_probe"
    prepared = ROOT / "morsehgp3D_v8/audits/lidar08_20260914/prepared"
    cases = [(0, 8000), (100, 8000), (200, 8000), (0, 16000),
             (0, 32000), (0, 50000), (100, 50000), (200, 50000)]
    # Only scan 0 at 50k has repetitions: report all other timings as single observations.
    cases += [(0, 50000), (0, 50000)]
    options = [(1, "all", 0), (2, "16", 0), (2, "16", 1), (4, "all", 1)]
    plans = []
    inputs = {}
    for group, (scan, n) in enumerate(cases):
        file = prepared / f"single_{scan:06d}/n{n}.u16le"
        metadata = file.parent / "METADATA.json"
        expected = next(s["sha256"] for s in json.loads(metadata.read_text())["samples"] if s["n"] == n)
        if sha(file) != expected:
            raise RuntimeError("input differs from original preparation")
        inputs[str(file.relative_to(ROOT))] = expected
        inputs[str(metadata.relative_to(ROOT))] = sha(metadata)
        # Alternate order, no warm-up removed from evidence.
        order = options if group % 2 == 0 else list(reversed(options))
        for factor, limit, inherit in order:
            command = [str(binary), str(n), str(file), "10", "8", "0", "0", "1", "64",
                       str(factor), limit, str(inherit)]
            plans.append({"group": group, "scan": scan, "n": n, "command": command})
    pins = {str(BASE.joinpath(name).relative_to(ROOT)): sha(BASE / name)
            for name in ["probe.cpp", "measure.py", "SOURCE_PINS.json", ".build/lidar_probe",
                         ".build/libmhgp8_p0.a"]}
    pins.update(inputs)
    original_affinity = sorted(os.sched_getaffinity(0))
    cpu = original_affinity[0]
    manifest = {"schema": "audit_lidar_front_options_v1", "status": "running",
                "source_commit": json.loads((BASE / "SOURCE_PINS.json").read_text())["commit"],
                "gcp_used": False, "public_status": "not_claimed", "pins": pins,
                "planned": plans, "affinity": [cpu], "available_affinity": original_affinity,
                "platform": platform.platform(), "started_unix": time.time(),
                "worktree": subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True)}
    write(out / "MANIFEST.json", manifest)
    os.sched_setaffinity(0, {cpu})
    completed = []
    try:
        for number, plan in enumerate(plans):
            record = dict(plan, status="started", started_unix=time.time())
            dest = out / f"record_{number:03d}.json"
            write(dest, record)
            stdout = out / f"record_{number:03d}.stdout"
            stderr = out / f"record_{number:03d}.stderr"
            with stdout.open("w") as sout, stderr.open("w") as serr:
                proc = subprocess.run(plan["command"], cwd=ROOT, stdout=sout, stderr=serr)
            record.update(returncode=proc.returncode, finished_unix=time.time(),
                          stdout_sha256=sha(stdout), stderr_sha256=sha(stderr))
            record["status"] = "completed" if proc.returncode == 0 else "failed"
            write(dest, record)
            completed.append(dest.name)
            print(number, plan["scan"], plan["n"], plan["command"][-3:], proc.returncode, flush=True)
            if proc.returncode:
                raise RuntimeError("probe failed; evidence retained")
        for name, value in pins.items():
            if sha(ROOT / name) != value:
                raise RuntimeError("capture dependency changed: " + name)
        snapshots = json.loads((BASE / "SOURCE_PINS.json").read_text())["source_sha256"]
        for name, value in snapshots.items():
            if sha(BASE / ".snapshot" / Path(name).relative_to("morsehgp3D_v8")) != value:
                raise RuntimeError("snapshot changed")
        write(out / "COMPLETION.json", {"status": "completed", "records": completed,
              "finished_unix": time.time(), "closure": "all inputs, binary, scripts and snapshot unchanged",
              "sha256": {p.name: sha(p) for p in out.iterdir() if p.is_file()}})
    except BaseException as error:
        write(out / "FAILURE.json", {"type": type(error).__name__, "message": str(error),
                                   "completed": completed, "time": time.time()})
        raise
    finally:
        os.sched_setaffinity(0, original_affinity)


if __name__ == "__main__":
    main()
