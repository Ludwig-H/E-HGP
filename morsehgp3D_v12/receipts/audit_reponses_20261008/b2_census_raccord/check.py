#!/usr/bin/env python3
"""Épingles et corps conservés du prototype ; aucun moteur ni donnée d'entrée."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def require(ok, why):
    if not ok:
        raise SystemExit(why)


def body(text, marker):
    start = text.index("{", text.index(marker))
    depth = 0
    for end in range(start, len(text)):
        depth += (text[end] == "{") - (text[end] == "}")
        if depth == 0:
            return text[start:end + 1]
    raise SystemExit("corps non fermé")


def main():
    require(len(sys.argv) == 3, "usage: check.py DEPOT CAPTURE_EXTERNE")
    c = json.loads(Path(__file__).with_name("capture.json").read_text())
    repo, snapshot = sys.argv[1], Path(sys.argv[2])
    require(hashlib.sha256((snapshot / "change.patch").read_bytes()).hexdigest() == c["patch"], "patch différent")
    candidate = {}
    for path, sha in c["sources"].items():
        raw = (snapshot / path).read_bytes()
        require(hashlib.sha256(raw).hexdigest() == sha, "candidat différent: " + path)
        candidate[path] = raw.decode()
    base = {}
    for path, sha in c["base_sources"].items():
        raw = subprocess.check_output(["git", "show", c["base_commit"] + ":morsehgp3D_v12/" + path], cwd=repo)
        require(hashlib.sha256(raw).hexdigest() == sha, "base différente: " + path)
        base[path] = raw.decode()
    checked = {}
    for name, marker, before, after in [
        ("workspace_run_body_unchanged", "Outcome CensusWorkspace::run(", "src/index/census_workspace.cpp", "src/index/census_workspace.cpp"),
        ("side_offset_body_unchanged", "GuardedSphere::side_offset(", "src/num/guard.cpp", "src/num/guard.hpp"),
        ("wide_sign_body_unchanged", "GuardedSphere::wide_sign(", "src/num/guard.cpp", "src/num/guard.cpp"),
    ]:
        checked[name] = body(base[before], marker) == body(candidate[after], marker)
        require(checked[name], "corps changé: " + name)
    combined = "\n".join(candidate.values())
    for excerpt in c["proof_excerpts"].values():
        require(excerpt in combined, "extrait absent")
    result = {"candidate_files": len(candidate), "base_files": len(base), **checked, "native_run": False}
    require(result == c["result"], "résultat différent")
    print(json.dumps(result, sort_keys=True, ensure_ascii=False))


if __name__ == "__main__":
    main()
