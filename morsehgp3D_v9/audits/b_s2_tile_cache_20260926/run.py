#!/usr/bin/env python3
"""Bounded diagnostic, no GCP. Capture raw-mass tile samples, not a contract."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DATA = ROOT / "morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    library = args.library.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(exist_ok=False)
    build = Path(tempfile.mkdtemp(prefix="mhgp9-tile-cache-"))
    sources = [HERE / "probe.cpp", Path(__file__), *sorted((ROOT / "morsehgp3D_v9/src/gen").rglob("*.cpp")),
               *sorted((ROOT / "morsehgp3D_v9/src/gen").rglob("*.hpp"))]
    pins = {str(p.relative_to(ROOT)): sha(p) for p in sources}
    manifest = {"scope": "stratified_raw_pair_mass_tiles_not_full", "gcp_used": False,
                "base_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                "source_before": pins, "library": str(library), "library_sha256": sha(library),
                "commands": [], "status": "running"}

    def save():
        (output / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")

    def run(label, command, expected=0):
        manifest["commands"].append({"label": label, "argv": command, "expected": expected, "status": "started"})
        save()
        started = time.monotonic()
        p = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (output / (label + ".stdout")).write_text(p.stdout)
        (output / (label + ".stderr")).write_text(p.stderr)
        manifest["commands"][-1].update(status="completed", exit_code=p.returncode,
                                        wall_seconds=time.monotonic()-started)
        save()
        if p.returncode != expected:
            raise RuntimeError(label + " unexpected exit " + str(p.returncode))
        return p.stdout

    try:
        probe = str(build / "probe")
        run("compiler", ["g++", "--version"])
        run("compile", ["g++", "-std=c++20", "-O3", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
                        "-I", str(ROOT / "morsehgp3D_v9/src/gen"), str(HERE / "probe.cpp"), str(library), "-o", probe])
        run("bad_arguments", [probe], 2)
        run("bad_separation", [probe, "absent.u32le", "5", "7", "10"], 2)
        run("bad_k", [probe, "absent.u32le", "11", "8", "10"], 2)
        rows = []
        for scene, k, s in (("00", 5, 8), ("01", 5, 8), ("02", 5, 8), ("00", 10, 8), ("00", 5, 10), ("00", 5, 12)):
            path = DATA / ("scene_" + scene + "_grid/full.u32le")
            label = "scene_" + scene + "_K" + str(k) + "_s" + str(s)
            row = json.loads(run(label, [probe, str(path), str(k), str(s), "4096"]))
            if row["status"] != "equal" or row["mismatches"] != 0:
                raise RuntimeError("tile masks differ")
            row.update(scene=scene, input_sha256=sha(path), input_path=str(path.relative_to(ROOT)))
            rows.append(row)
        manifest.update(status="completed", source_after={str(p.relative_to(ROOT)): sha(p) for p in sources},
                        library_after_sha256=sha(library), binary=probe, binary_sha256=sha(Path(probe)))
        if manifest["source_after"] != pins or manifest["library_after_sha256"] != manifest["library_sha256"]:
            raise RuntimeError("sources/library changed")
        (output / "RESULTS.json").write_text(json.dumps(rows, indent=2) + "\n")
        manifest["output_sha256"] = {p.name: sha(p) for p in sorted(output.iterdir()) if p.name != "MANIFEST.json"}
    except BaseException as error:
        manifest.update(status="failed", error=repr(error))
        raise
    finally:
        save()
    print(json.dumps({"status": "completed", "cases": len(rows), "output": str(output)}))


if __name__ == "__main__":
    main()
