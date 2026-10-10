#!/usr/bin/env python3
"""Prélecture figée : JSON de métadonnées seulement, aucun payload ni moteur."""
import argparse
import hashlib
import json
from pathlib import Path


def require(condition, message):
    if not condition:
        raise ValueError(message)


def raw_json(path):
    data = path.read_bytes()
    return json.loads(data), {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def flags(command):
    argv = command["argv"]
    require(argv[:2] == ["python3", "{src}/morsehgp3D_v12/microbancs/mes_b_scenes/pilote_b.py"], "pilote")
    require(len(argv[2:]) % 2 == 0, "arguments")
    result = dict(zip(argv[2::2], argv[3::2]))
    require(len(result) * 2 == len(argv[2:]), "options dupliquées")
    return result


def cases(value):
    result = []
    for item in value.split(","):
        scene, k, backend, passes = item.split(":")
        result.append({"scene": scene, "k": int(k), "backend": backend, "passes": int(passes)})
    return result


def analyse(current, reference):
    primary = {}
    cp, primary["current_preflight"] = raw_json(current / "preflight.json")
    rp, primary["reference_preflight"] = raw_json(reference / "preflight.json")
    cl, primary["current_plan"] = raw_json(current / "package/plan.json")
    rl, primary["reference_plan"] = raw_json(reference / "package/plan.json")
    # Unique accès à data_dir : le manifeste JSON. Ne jamais ouvrir les noms des payloads.
    manifest, primary["bundle_manifest"] = raw_json(Path(cp["data_dir"]) / "bundle_manifest.json")
    require(cp["commit"] == "ae8f8107cc5a13356da89addf90808b5aaad9d67", "source courante")
    require(rp["commit"] == "caf9585e4a89ba101ba86d2d46e77cb340be15bf", "source référence")
    for preflight, key in [(cp, "current_plan"), (rp, "reference_plan")]:
        require(preflight["source_kind"] == "commit", "source non commit")
        require(preflight["evidence_grade"] == "pushed_commit", "grade")
        require(preflight["public_status"] == "not_claimed", "statut")
        require(preflight["plan_sha256"] == primary[key]["sha256"], "plan différent du préflight")
    require(cp["data_manifest_sha256"] == rp["data_manifest_sha256"], "empreinte déclaration données")
    maps = []
    for preflight in (cp, rp):
        files = {f["name"]: {"bytes": f["size"], "sha256": f["sha256"]} for f in preflight["data_files"]}
        require(len(files) == len(preflight["data_files"]) == 45, "45 noms uniques requis")
        require(all(Path(name).name == name for name in files), "noms non locaux")
        maps.append(files)
    require(maps[0] == maps[1], "métadonnées des entrées différentes")
    require(maps[0]["bundle_manifest.json"] == primary["bundle_manifest"], "manifeste non épinglé")
    require(manifest["schema"] == "mhgp12.benchmark_inputs.v1", "schéma manifeste")
    scenes = {}
    for case in manifest["cases"]:
        name = case["name"]
        require(name not in scenes and case["profile"] == "u21", "scène/profil")
        entry = case["distinct"] if case.get("bundled") == "distinct" else case
        n = entry.get("count", case.get("count"))
        require(type(n) is int and n > 0, "nombre de sites")
        for field, sha_field, width in [("coordinates", "sha256", 12), ("point_ids", "ids_sha256", 4)]:
            require(maps[0][entry[field]] == {"bytes": width * n, "sha256": entry[sha_field]}, "description de payload incohérente")
        scenes[name] = n
    require(len(scenes) == 15, "15 scènes")
    require(cl["schema"] == rl["schema"] == "ehgp.v12.session_plan.v1", "schéma plan")
    require(cl["default_build"] is False and rl["default_build"] is False, "build implicite")
    require(len(cl["commands"]) == len(rl["commands"]) == 2, "deux commandes")
    commands = []
    for new, old in zip(cl["commands"], rl["commands"]):
        nf, of = flags(new), flags(old)
        require(new["name"] == old["name"] and new["timeout_seconds"] == old["timeout_seconds"], "ordre/délai des commandes")
        require(nf == of, "options pilote différentes")
        cohort = cases(nf["--cas"])
        require(all(c["scene"] in scenes for c in cohort), "scène absente")
        require(nf["--fils"] == "48" and nf["--budget-gio"] == "160" and nf["--empreinte-max-sites"] == "1600000", "régime")
        commands.append({"name": new["name"], "timeout_seconds": new["timeout_seconds"], "pilot_deadline_seconds": int(nf["--delai-global"]), "device_budget_gib": int(nf["--budget-appareil-gio"]), "cases": cohort})
    identity, main = [c["cases"] for c in commands]
    require(identity == [{"scene": "boreas_202011261358_f4500_n10_sans_sol", "k": 5, "backend": "appareil", "passes": 2}, {"scene": "boreas_202011261358_f4500_n10_sans_sol", "k": 5, "backend": "cpu", "passes": 1}], "cohorte identité")
    require(len(main) == 17 and len({c["scene"] for c in main}) == 15, "cohorte principale")
    require(all(c["backend"] == "appareil" for c in main), "principal GPU")
    require([(c["k"], c["passes"]) for c in main] == [(5, 2)] * 11 + [(10, 1)] * 2 + [(5, 1)] * 4, "répartition passes")
    require(commands[0]["device_budget_gib"] == 8 and commands[1]["device_budget_gib"] == 88, "budgets GPU")
    require(cp["budget"]["worker_window_seconds"] == 2200 and cp["budget"]["command_timeouts_sum_seconds"] == 3800, "fenêtre courante")
    require(cp["budget"]["oversubscribed"] is True, "sursouscription")
    cohort = identity + main
    result = {
        "schema": "audit.mesb1o_prelecture.v1", "status": "protocol_only_no_results",
        "current_commit": cp["commit"], "reference_commit": rp["commit"],
        "metadata_files_equal": 45, "scene_sites": scenes, "commands": commands,
        "planned_processes": len(cohort), "planned_full_passes": sum(c["passes"] for c in cohort),
        "planned_first_passes": len(cohort), "planned_warm_observations": sum(c["passes"] - 1 for c in cohort),
        "main_cases": 17, "main_scenes": 15, "main_full_passes": 28,
        "digest_max_sites": 1600000, "threads": 48, "host_budget_gib": 160,
        "current_budget": cp["budget"], "reference_budget": rp["budget"],
        "same_pilot_options_and_order": True, "payload_bytes_read": 0,
    }
    capture = {"schema": "audit.mesb1o_prelecture.capture.v1", "primary_metadata": primary, "declared_data_manifest_sha256": cp["data_manifest_sha256"], "declared_files_only_not_payload_rehashed": maps[0]}
    return capture, result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--current", type=Path, required=True)
    parser.add_argument("--reference", type=Path, required=True)
    args = parser.parse_args()
    capture, result = analyse(args.current, args.reference)
    here = Path(__file__).resolve().parent
    require(capture == json.loads((here / "capture.json").read_bytes()), "capture modifiée")
    require(result == json.loads((here / "results.json").read_bytes()), "résultats de prélecture modifiés")
    print(json.dumps({"status": "ok", "primary_metadata": len(capture["primary_metadata"]), "declarations": 45, "cases": 17, "scenes": 15, "planned_full_passes": 31, "payload_bytes_read": 0}, sort_keys=True))


if __name__ == "__main__":
    main()
