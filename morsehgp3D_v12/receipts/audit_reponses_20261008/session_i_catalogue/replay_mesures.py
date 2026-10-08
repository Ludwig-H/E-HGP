#!/usr/bin/env python3
"""Relecture des 36 journaux de temps déjà archivés ; aucun moteur lancé."""
import argparse
import hashlib
import json
import re
import statistics
from pathlib import Path


def require(test, message):
    if not test:
        raise ValueError(message)


def strict_pairs(pairs):
    out = {}
    for key, value in pairs:
        require(key not in out, "cle dupliquee")
        out[key] = value
    return out


def rows(path):
    lines = path.read_text(encoding="utf-8").splitlines()
    require(lines[0].startswith("$ "), "commande absente")
    split = lines.index("--- stderr ---")
    require(not any(line.strip() for line in lines[split + 1:]), "stderr non vide")
    parsed = [json.loads(line, object_pairs_hook=strict_pairs) for line in lines[1:split] if line.strip()]
    require(all(isinstance(row, dict) for row in parsed), "ligne non objet")
    require(parsed[-1] == {"phase": "exit", "status": "ok", "reason": "none"}, "sortie non conforme")
    return [row for row in parsed if row.get("phase") == "catalogue"]


def stats(values):
    return {"median_ns": statistics.median(values), "max_ns": max(values)}


def stages(row):
    diag, dev = row["diagnostics"], row["device"]
    return {
        "parcours": diag["traversal_ns"], "feuilles": diag["count_ns"], "emission": diag["fill_ns"],
        "fin": sum(diag[key] for key in ("levels_ns", "sort_ns", "assemble_ns", "table_ns")),
        "transferts": dev["transfer_ns"], "publication": dev["publish_ns"],
    }


def summary(runs):
    warm = [row for run in runs for row in run[1:]]
    cold = [run[0] for run in runs]
    out = {"processes": len(runs), "warm_values": len(warm), **stats([row["wall_ns"] for row in warm]),
           "max_process_median_ns": max(statistics.median(row["wall_ns"] for row in run[1:]) for run in runs),
           "first_pass": stats([row["wall_ns"] for row in cold])}
    out["stages"] = {key: stats([stages(row)[key] for row in warm]) for key in stages(warm[0])}
    out["non_ventile"] = stats([row["wall_ns"] - sum(stages(row).values()) for row in warm])
    out["warm_allocations"] = sorted({row["device"]["allocations"] for row in warm})
    if warm[0]["path"] == "device":
        require(out["warm_allocations"] == [0], "allocation appareil a chaud")
        # Six tableaux rapatries par finish_stage, ABI cible : CatalogueBall32, LevelWords48 (u21).
        # L inclut le rang zero, qui est construit sur l'hote et n'est pas transporte.
        traffic = []
        for row in warm:
            c, inc, lev, n = (row[key] for key in ("balls", "incidences", "levels", "sites"))
            parts = {"boules": 32*c, "offsets_population": 8*(c+1), "incidences": 4*inc,
                     "niveaux": 48*(lev-1), "offsets_supports": 8*(n+1), "valeurs_supports": 4*c}
            received = row["device"]["transfer_d2h_bytes"]
            chains, elements = (row["diagnostics"][key] for key in ("chains_repaired", "chain_elements"))
            repair = (16*c + 64*elements) if chains else 0
            require(sum(parts.values()) <= received, "modele des sorties depasse D2H")
            require(sum(parts.values()) + repair <= received, "modele du repli depasse D2H")
            traffic.append({"six_sorties": parts, "sorties_total": sum(parts.values()), "D2H": received,
                            "D2H_hors_six_sorties": received-sum(parts.values()),
                            "chains_repaired": chains, "chain_elements": elements,
                            "D2H_repli": repair, "D2H_repli_tableaux_complets": 16*c if chains else 0,
                            "D2H_controle_restant": received-sum(parts.values())-repair,
                            "H2D": row["device"]["transfer_h2d_bytes"], "ops": row["device"]["transfer_ops"],
                            "device_bytes": row["device"]["device_bytes"],
                            "pinned_bytes": row["device"]["pinned_bytes"], "budget_peak": row["diagnostics"]["peak_bytes"]})
        require(all(entry == traffic[0] for entry in traffic), "volumes instables")
        out["traffic_and_capacity"] = traffic[0]
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    v12 = args.repo / "morsehgp3D_v12"
    root = v12 / "receipts/g4_t1bi_20261008/resultats/cmd/000_t1b_catalogue_appareil/files/t1b"
    cpu, device, pins, passes = {}, {}, {}, 0
    for path in sorted((root/"logs").glob("*.log")):
        cm = re.fullmatch(r"cpu_(\w+)_k(5|10)_f(16|24)\.log", path.name)
        dm = re.fullmatch(r"dev_k(5|10)_(ng0[012])_p([0-4])\.log", path.name)
        if not cm and not dm:
            continue
        run = rows(path)
        kind, k, leaf, frame = ("cpu", int(cm[2]), int(cm[3]), cm[1]) if cm else ("device", int(dm[1]), 24, dm[2])
        count = 5 if kind == "device" and k == 10 else 10
        require(len(run) == count, "nombre de passes")
        for p, row in enumerate(run):
            require([row[key] for key in ("status", "reason", "path", "coord_bits", "kmax", "leaf", "threads", "pass")]
                    == ["ok", "none", kind, 21, k, leaf, 48, p], "configuration")
            require(type(row["wall_ns"]) is int and row["wall_ns"] > 0, "temps non entier positif")
            require(all(type(t) is int and t >= 0 for t in stages(row).values()), "temps elementaire")
            require(sum(stages(row).values()) <= row["wall_ns"], "temps non disjoints")
        pins[path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
        passes += len(run)
        if cm:
            cpu[f"{frame}:K{k}:L{leaf}"] = summary([run])
        else:
            device.setdefault(f"{frame}:K{k}", {})[int(dm[3])] = run
    require(len(cpu) == 12 and len(device) == 6 and len(pins) == 36 and passes == 315, "cohorte")
    for key, runs in device.items():
        expected = 3 if key.endswith("K10") else 5
        require(sorted(runs) == list(range(expected)), "processus absents")
        device[key] = summary([runs[i] for i in range(expected)])
    sources = {}
    for name in ("bench/catalogue_probe.cpp", "src/catalogue/device_pipeline.hpp", "src/catalogue/device_cuda.cu",
                 "src/catalogue/finish_driver.hpp", "src/catalogue/finish_level.hpp", "src/catalogue/catalogue.hpp",
                 "src/catalogue/assemble.cpp"):
        sources[name] = hashlib.sha256((v12/name).read_bytes()).hexdigest()
    result = {"schema": "audit_session_i_mesures_v1", "logs": pins, "sources": sources, "passes_lues": passes,
              "cpu": cpu, "device": device,
              "portee": "Catalogue C seul, u21, W48 ; aucune somme C+G ni temps FULL, aucun natif relance."}
    if args.check:
        require(result == json.loads(Path(__file__).with_name("mesures.json").read_text()), "capture differente")
        print(json.dumps({"conforme": True, "journaux": len(pins), "passes": passes, "cellules_cpu": 12, "cellules_gpu": 6}))
    else:
        print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
