#!/usr/bin/env python3
"""Contrôle des sources du protocole ; n'exécute ni ne simule le moteur."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def require(value, message):
    if not value:
        raise ValueError(message)


def blob(repo, pin, path):
    return subprocess.check_output(["git", "-C", str(repo), "show", f"{pin}:{path}"])


def main():
    here = Path(__file__).resolve().parent
    capture = json.loads((here / "capture.json").read_text())
    repo = Path(sys.argv[1])
    bodies = {}
    for path, wanted in capture["sources"].items():
        data = blob(repo, capture["pin"], path)
        require(hashlib.sha256(data).hexdigest() == wanted, path)
        bodies[path] = data.decode()
    path = "morsehgp3D_v12/src/tower/pipeline_run.cpp"
    require(blob(repo, capture["pin"], path) == blob(repo, capture["initial_pin"], path), "corps initial")
    body = bodies[path].split("Outcome run_region(", 1)[1]
    sites = ["p.in_flight.fetch_add(1, std::memory_order_acq_rel)",
             "p.in_flight.fetch_sub(1, std::memory_order_acq_rel);\n    const u64 seen",
             "if (p.in_flight.load(std::memory_order_acquire) == 0 && !find_job(p, false, job)) return {};",
             "while (p.epoch.load(std::memory_order_acquire) == seen && p.in_flight.load(std::memory_order_acquire) != 0)"]
    positions = [body.index(s) for s in sites]
    require(positions == sorted(positions), "sites du protocole")
    require("add_library(mhgp12 STATIC)" in bodies["morsehgp3D_v12/CMakeLists.txt"], "archive statique")
    result = {"source_pins": len(bodies), "corps_identique_86d": True,
              "sites_ordonnes": len(sites), "gate_native_executee": False,
              "qualification_terminaison_native": False}
    expected = here / "results.json"
    if expected.exists():
        require(json.loads(expected.read_text()) == result, "résultat attendu")
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))


if __name__ == "__main__":
    main()
