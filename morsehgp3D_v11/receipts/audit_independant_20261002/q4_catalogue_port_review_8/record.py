#!/usr/bin/env python3
"""Observation de fermeture, sans lancement de code produit."""
from datetime import datetime, timezone
from hashlib import sha256
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
DEV = Path("/workspaces/E-HGP/build/v11-development-20261002")
before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
commit = before["commit"]


def git(*args):
    return subprocess.check_output(["git", "-C", str(DEV), *args])


def digest(data):
    return sha256(data).hexdigest()


files = []
for item in before["files"]:
    path = item["path"]
    copied = (HERE / "source" / path).read_bytes()
    frozen = git("show", commit + ":" + path)
    working = (DEV / path).read_bytes()
    if not (digest(copied) == digest(frozen) == digest(working) == item["sha256"]):
        raise RuntimeError("source a changé : " + path)
    files.append({"path": path, "copy_sha256": digest(copied),
                  "git_sha256": digest(frozen), "working_sha256": digest(working)})

pins = json.loads((HERE / "source/morsehgp3D_v11/tests/num/source_pins.json").read_text())["q4_candidate_revision"]
pin_files = []
for item in pins["baseline_sources"]:
    path = "morsehgp3D_v11/" + item["path"]
    data = git("show", pins["baseline_commit"] + ":" + path)
    if digest(data) != item["sha256"]:
        raise RuntimeError("pin historique incorrect : " + path)
    target = HERE / "baseline" / path
    if target.exists():
        raise RuntimeError("capture baseline déjà présente : " + path)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    pin_files.append({"path": path, "declared_sha256": item["sha256"],
                      "observed_sha256": digest(data)})

for name, payload in (("SOURCE_AFTER.json", {
        "utc": datetime.now(timezone.utc).isoformat(),
        "frozen_commit": commit, "observed_head": git("rev-parse", "HEAD").decode().strip(),
        "observed_status": git("status", "--short").decode(), "files": files}),
        ("BASELINE_PINS.json", {"commit": pins["baseline_commit"], "files": pin_files})):
    target = HERE / name
    if target.exists():
        raise RuntimeError("fermeture déjà présente : " + name)
    target.write_text(json.dumps(payload, indent=2) + "\n")
