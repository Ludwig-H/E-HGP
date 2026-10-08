#!/usr/bin/env python3
"""Relecture du patch sur le test sauvé, sans compilation ni moteur."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile

HERE = Path(__file__).resolve().parent

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", type=Path, required=True)
    args = parser.parse_args()
    pins = json.loads((HERE / "pins.json").read_text())
    sha = lambda b: hashlib.sha256(b).hexdigest()
    archive_bytes = args.archive.read_bytes()
    if sha(archive_bytes) != pins["archive"]["sha256"]:
        raise SystemExit("archive hash mismatch")
    spec = pins["test_patch"]
    with tarfile.open(args.archive, "r:gz") as archive:
        before = archive.extractfile(spec["archive_member"]).read()
    if sha(before) != spec["before_sha256"]:
        raise SystemExit("test source hash mismatch")
    with tempfile.TemporaryDirectory(prefix="audit-a-counter-") as folder:
        root = Path(folder)
        target = root / spec["path"]
        target.parent.mkdir(parents=True)
        target.write_bytes(before)
        patch = HERE / "compte_tour_seule.patch"
        for options in (("--check",), ()):
            subprocess.run(["git", "apply", *options, str(patch)], cwd=root,
                           check=True, capture_output=True)
        after = target.read_bytes()
        if sha(after) != spec["after_sha256"]:
            raise SystemExit("patched source hash mismatch")
        subprocess.run(["git", "apply", "--reverse", "--check", str(patch)],
                       cwd=root, check=True, capture_output=True)
    print(json.dumps({"apply_check": True, "after_sha256": sha(after),
                      "reverse_check": True, "native_test": False}, sort_keys=True))

if __name__ == "__main__":
    main()
