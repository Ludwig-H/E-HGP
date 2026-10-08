#!/usr/bin/env python3
"""Freeze FULLN source/plan/protocol metadata, never run its commands or data."""
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tarfile


def need(ok, text):
    if not ok:
        raise RuntimeError(text)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    need(len(sys.argv) == 4, "usage: check.py DEPOT_GIT DOSSIER_SESSION SNAPSHOT_PROTOCOLE")
    repo, session, snap = [Path(a).resolve() for a in sys.argv[1:]]
    here = Path(__file__).resolve().parent
    cap = json.loads((here / "capture.json").read_text())
    plan_raw = (snap / "plan.json").read_bytes()
    need(digest(plan_raw) == cap["plan_sha256"] and len(plan_raw) == cap["plan_bytes"], "frozen plan")
    need((session / "package/plan.json").read_bytes() == plan_raw, "session plan")
    state_raw = (snap / "etat_local.json").read_bytes()
    need(digest(state_raw) == cap["etat_local_sha256"] and json.loads(state_raw) == cap["etat_local"], "frozen state")
    plan = json.loads(plan_raw)
    allowed = {"--fils", "--processus", "--passes", "--jobs", "--delai", "--k", "--voies", "--tours",
               "--tours-difficiles", "--fils-difficiles", "--delai-global", "--delai-cas"}
    commands = []
    for command in plan["commands"]:
        args = command["argv"]
        opts = {a: args[i + 1] for i, a in enumerate(args[:-1]) if a in allowed}
        commands.append(dict(name=command["name"], timeout_seconds=command["timeout_seconds"], options_publiques=opts))
        need(not any(a in args for a in ["--essai", "--sequentiel", "--cache", "--budget-gio"]), "default regime")
    need(commands == cap["commands"], "command option projection")
    need(plan["default_build"] == cap["default_build"] and plan["build_timeout_seconds"] == cap["build_timeout_seconds"],
         "default build")
    prefixes = ("morsehgp3D_v12/src/", "morsehgp3D_v12/tests/", "morsehgp3D_v12/bench/",
                "morsehgp3D_v12/microbancs/", "morsehgp3D_v12/cmake/")

    def selected(path):
        return path.startswith(prefixes) or path in ["morsehgp3D_v12/CMakeLists.txt", "gcp-migration/v12_worker.sh"]

    archive = (session / "package/package.tar.gz").read_bytes()
    need(digest(archive) == cap["package"]["sha256"] and len(archive) == cap["package"]["bytes"], "source package")
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:gz") as tar:
        source = {m.name: tar.extractfile(m).read() for m in tar.getmembers() if m.isfile() and selected(m.name)}
    git_archive = subprocess.check_output(["git", "archive", cap["source_git"], "morsehgp3D_v12", "gcp-migration/v12_worker.sh"], cwd=repo)
    with tarfile.open(fileobj=io.BytesIO(git_archive)) as tar:
        git_source = {m.name: tar.extractfile(m).read() for m in tar.getmembers() if m.isfile() and selected(m.name)}
    need(source == git_source and len(source) == cap["source_files"] == 488, "Git package equality")
    inventory = "".join(digest(data) + "  " + path + "\n" for path, data in sorted(source.items()))
    need(digest(inventory.encode()) == cap["source_inventory_sha256"], "inventory")
    for path, sha in cap["source_pins"].items():
        need(digest(source[path]) == sha, "source pin " + path)
    probe = source["morsehgp3D_v12/bench/full_probe.cpp"].decode()
    need("cache = u64{8} << 30" in probe and "budget = MemoryBudget::kUnlimited" in probe, "probe defaults")
    full = source["morsehgp3D_v12/microbancs/mes_full/pilote_full.py"].decode()
    need("small = (min(3, args.processus), min(5, args.passes))" in full, "small cohorts")
    need("FRAMES, 5, small[0], small[1], args.fils, False" in full, "CPU K5 cohort")
    small = source["morsehgp3D_v12/microbancs/mes_c_petits/pilote_c.py"].decode()
    need("default=64.0" in small and "MAIN_REGIME_NS_PER_SITE = 241.3e6 / 64740" in small, "MES-C references")
    result = {
        "schema": "fulln-protocole-result-v1", "source_git": cap["source_git"], "source_files_equal": 488,
        "plan_sha256": cap["plan_sha256"], "b3": False, "cache_octets": 8 << 30,
        "full": {
            "ng_gpu_k5": {"processes": 15, "passes": 150, "hot": 135},
            "ng_gpu_k10": {"processes": 9, "passes": 45, "hot": 36},
            "ng_cpu_k5": {"processes": 9, "passes": 45, "hot": 36},
            "v12set_gpu_k5": {"processes": 5, "passes": 370, "hot": 185},
            "threads": 48, "cpu_ng_k10": False, "cpu_v12set": False,
            "total_processes": 38, "total_passes": 610, "total_hot": 392},
        "small_expected": {"real": 132, "sound": 15, "hard": 12, "k": [5, 10], "ways": ["cpu", "appareil"],
                           "threads": [4, 48], "cycles": 3, "hard_threads": 48, "hard_passes": 2,
                           "processes_if_complete": 56, "passes_if_all_succeed": 3624,
                           "input_manifest_newly_qualified": False},
        "etat_local_capture": cap["etat_local"], "resultats_admis_par_ce_recu": False,
        "natif_execute_par_auditeur": False}
    rendered = json.dumps(result, ensure_ascii=False, indent=2) + "\n"
    expected = here / "results.json"
    if expected.exists():
        need(expected.read_text() == rendered, "results mismatch")
    print(rendered, end="")


if __name__ == "__main__":
    main()
