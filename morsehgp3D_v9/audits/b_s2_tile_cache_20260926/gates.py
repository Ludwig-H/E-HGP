#!/usr/bin/env python3
"""Capture causal cache tests separately from the unchanged sample capture."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--library", type=Path, required=True)
    args = p.parse_args()
    library = args.library.resolve(strict=True)
    out = HERE / "gates"
    out.mkdir(exist_ok=False)
    build = Path(tempfile.mkdtemp(prefix="mhgp9-tile-cache-gates-"))
    source = HERE / "gate.cpp"
    manifest = {"source_sha256": sha(source), "runner_sha256": sha(Path(__file__)),
                "library_sha256": sha(library), "commands": [], "status": "running"}

    def run(name, argv, expected, cause=None):
        completed = subprocess.run(argv, cwd=ROOT, capture_output=True, text=True)
        (out / (name + ".stdout")).write_text(completed.stdout)
        (out / (name + ".stderr")).write_text(completed.stderr)
        manifest["commands"].append(dict(name=name, argv=argv, returncode=completed.returncode, expected=expected))
        (out / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
        if completed.returncode != expected or (cause is not None and completed.stderr.strip() != cause):
            raise RuntimeError(name + " did not match causal outcome")

    for variant, define in (("normal", []), ("mutant", ["-DMHGP9_TILE_CACHE_MUTANT_NO_RETEST=1"])):
        binary = str(build / variant)
        run("compile_" + variant, ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                                  "-pthread", "-I", str(ROOT / "morsehgp3D_v9/src/gen"), *define, str(source),
                                  str(library), "-o", binary], 0)
        run(variant, [binary], 0 if variant == "normal" else 1,
            None if variant == "normal" else "cause=tile_cache.endpoint_retest")
    if sha(source) != manifest["source_sha256"] or sha(library) != manifest["library_sha256"]:
        raise RuntimeError("dependencies changed")
    manifest.update(status="completed", output_sha256={f.name: sha(f) for f in out.iterdir() if f.name != "MANIFEST.json"})
    (out / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("PASS causal tile-cache gates; mutant rejected by endpoint_retest")


if __name__ == "__main__":
    main()
