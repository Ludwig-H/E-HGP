#!/usr/bin/env python3
"""Reçu : balayage de la séparation s du front v8 avec le census q2 (auditeur B, 14 septembre 2026).

Exécute la sonde produit mhgp8_wspd_q2_census_probe (--probe, construite depuis --src-root, sources
épinglées par hachés) sur les familles synthétiques à 8k (s ∈ {8,10,12}, Pool 0 et 64) et à 32k
(s ∈ {8,10,12}, Pool 64), toujours Samples / Shared / frère / Complement / ancres individuelles, Kmax 10,
graine 3. Jamais de s < 8 : une séparation inférieure n'a pas de sens pour l'objet (directive du
14 septembre 2026). Vérifie que le condensé canonique des supports ne dépend ni de s ni du filtre Pool, et
consigne rectangles, candidates, visites, sites de facteurs relus et temps englobants.
Écrit SEPARATION_CHECKS.json. Sans assert. Aucun temps n'est un contrat : hôte partagé, un fil.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fail(message: str) -> int:
    print(f"ECHEC : {message}", file=sys.stderr)
    return 1


def pick(d: dict, *keys):
    cur = d
    for k in keys:
        if not isinstance(cur, dict) or k not in cur:
            return None
        cur = cur[k]
    return cur


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--src-root", required=True)
    parser.add_argument("--output", default=str(HERE / "SEPARATION_CHECKS.json"))
    parser.add_argument("--quick", action="store_true", help="8k amas et uniforme, s 8/12, Pool 64 seulement")
    args = parser.parse_args()
    probe = Path(args.probe).resolve(); src = Path(args.src_root).resolve()
    pinned = [probe, src / "src/wspd/front.cpp", src / "src/wspd/front.hpp", src / "src/pipeline/q2_census.cpp",
              src / "src/pipeline/q2_node_pool.hpp", src / "src/pipeline/wspd_q2_census.hpp", src / "bench/wspd_q2_census_probe.cpp",
              src / "bench/front_fixtures.hpp"]
    for p in pinned:
        if not p.is_file():
            return fail(f"fichier absent : {p}")
    pins = {str(p.relative_to(ROOT)) if str(p).startswith(str(ROOT)) else str(p): sha256(p) for p in pinned}
    separations = (8, 10, 12)  # jamais en dessous de 8
    if args.quick:
        plan = [(8000, fam, s, 64) for fam in ("clusters", "uniform") for s in (8, 12)]
    else:
        plan = [(8000, fam, s, pool) for fam in ("uniform", "clusters", "terrain", "rows") for s in separations for pool in (0, 64)]
        plan += [(32000, fam, s, 64) for fam in ("uniform", "clusters") for s in separations]
    if any(s < 8 for _, _, s, _ in plan):
        return fail("une séparation inférieure à 8 n'a pas de sens ; plan refusé")
    runs = []
    digests: dict[tuple, set] = {}
    for n, fam, s, pool in plan:
        cmd = [str(probe), str(n), fam, "10", str(s), "3", "samples", "shared", "sibling", "complement", "anchors", str(pool)]
        started = time.perf_counter()
        done = subprocess.run(cmd, capture_output=True, text=True)
        elapsed = time.perf_counter() - started
        if done.returncode != 0:
            return fail(f"échec {' '.join(cmd)} (code {done.returncode}) :\n{done.stdout[-2000:]}\n{done.stderr[-2000:]}")
        try:
            d = json.loads(done.stdout)
        except json.JSONDecodeError:
            return fail(f"sortie non JSON : {' '.join(cmd)}\n{done.stdout[-1000:]}")
        digest = pick(d, "digest")
        key = (n, fam)
        digests.setdefault(key, set()).add(json.dumps(digest, sort_keys=True))
        fw = d.get("front_work") or {}
        pw = d.get("pool_work") or {}
        summary = {
            "n": n, "family": fam, "s": s, "pool_min_factor": pool,
            "input_rectangles": d.get("input_rectangles"), "candidate_pairs": d.get("candidate_pairs"),
            "accepted_pairs": d.get("accepted_pairs"), "rejected_pairs": d.get("rejected_pairs"),
            "product_visits": fw.get("product_visits"), "witness_searches": fw.get("witness_searches"),
            "witness_descent_steps": fw.get("witness_descent_steps"), "emitted_factor_sites": fw.get("emitted_factor_sites"),
            "max_factor_size": fw.get("max_factor_size"), "size_class_rectangles": fw.get("size_class_rectangles"),
            "census_work": d.get("census_work"),
            "pool_selected_rectangles": pw.get("selected_rectangles"), "pool_filtered_pairs": pw.get("filtered_pairs"),
            "pool_passthrough_rectangles": pw.get("passthrough_rectangles"), "pool_pair_roots": pw.get("pair_roots"),
            "pool_factor_sites": pw.get("factor_sites"),
            "pipeline_total_ms": pick(d, "timings", "pipeline_total_ms"), "total_ms": pick(d, "timings", "total_ms"),
            "front_and_count_ms": pick(d, "timings", "front_and_count_ms"), "payload_ms": pick(d, "timings", "payload_ms"),
            "pool_preparation_ms": pw.get("preparation_ms"), "pool_selected_total_ms": pw.get("selected_total_ms"),
            "digest": digest, "wall_seconds": round(elapsed, 3),
        }
        runs.append({"command": " ".join(cmd), "summary": summary, "raw_stdout": done.stdout})
        print(f"{fam} n={n} s={s} pool={pool}: rect={summary['input_rectangles']} cand={summary['candidate_pairs']} total_ms={summary['pipeline_total_ms']}", flush=True)
    digest_ok = all(len(v) == 1 for v in digests.values())
    if not digest_ok:
        return fail("le condensé des supports dépend de s ou du filtre Pool : " + json.dumps({str(k): len(v) for k, v in digests.items()}))
    receipt = {
        "title": "Balayage de la séparation s du front v8 (census q2, filtre Pool terminal) : condensés identiques, coûts mesurés",
        "date": "2026-09-14", "author_role": "auditeur indépendant B",
        "git_head": subprocess.run(["git", "rev-parse", "HEAD"], cwd=str(ROOT), capture_output=True, text=True).stdout.strip(),
        "src_root": str(src), "pins_sha256": pins, "runs": runs,
        "digests_per_input": {f"{k[1]}_{k[0]}": sorted(v)[0] for k, v in digests.items()},
        "stable_digest_without_times": hashlib.sha256(json.dumps([{k: v for k, v in r["summary"].items() if not k.endswith("_ms") and k != "wall_seconds"} for r in runs], sort_keys=True).encode()).hexdigest(),
        "scope": "Mesure d'audit ; aucun temps n'est un contrat ; aucune tour FULL ; s n'est pas un paramètre d'exactitude.",
    }
    Path(args.output).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OK : {len(runs)} exécutions, condensés identiques par entrée : {digest_ok}, reçu {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
