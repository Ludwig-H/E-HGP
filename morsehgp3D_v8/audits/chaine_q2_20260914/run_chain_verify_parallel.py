#!/usr/bin/env python3
"""Reçu : vérification de la chaîne parallèle front + census q2 (auditeur B, 14 septembre 2026).

Compile chain_verify_parallel.cpp contre une extraction épinglée (--src-root, `git archive <commit>`)
et sa bibliothèque produit (--lib), puis :
- mode vérification (petits nuages) : pour cinq combinaisons d'options, W ∈ {1,2,3,4,8} fils et lots
  de 1 ou 16 produits, les supports réunis de tous les slots doivent être exactement ceux de la force
  brute (paires à moins de Kmax intérieurs stricts, une seule fois, intérieurs, coquille complète, clé),
  sans doublon entre slots, avec un condensé canonique identique d'un W à l'autre et des compteurs
  discrets (rectangles, candidates, admises, rejetées) égaux à ceux du chemin série ;
- mode échelle (8k/16k/32k, pas de force brute) : mêmes identités de condensé et de compteurs entre le
  chemin série et W ∈ {1,2,4,8}, temps englobants consignés (hôte partagé, un seul passage : pas un contrat).
Écrit CHAINE_Q2_PARALLEL_CHECKS.json. Sans assert.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
FIELD_RE = re.compile(r"(\w+)=([-\w.:/]+)")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fail(message: str) -> int:
    print(f"ECHEC : {message}", file=sys.stderr)
    return 1


def run(cmd: list[str], cwd: Path) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True)


def parse_summary(stdout: str) -> dict | None:
    for line in stdout.splitlines():
        if line.startswith("SUMMARY "):
            out = {}
            for k, v in FIELD_RE.findall(line):
                try:
                    out[k] = int(v)
                except ValueError:
                    out[k] = v
            return out
    return None


def parse_scale(stdout: str) -> list[dict]:
    rows = []
    for line in stdout.splitlines():
        if line.strip().startswith("W="):
            f = dict(FIELD_RE.findall(line))
            rows.append({"workers": int(f["W"]), "jobs_per_worker": int(f["J"]), "started": int(f["started"]),
                         "jobs": int(f["jobs"]), "total_ms": float(f["total_ms"]), "digest_eq": f["digest_eq"], "counters_eq": f["counters_eq"],
                         "worker_max_ms": float(f.get("worker_max_ms", 0)), "worker_mean_ms": float(f.get("worker_mean_ms", 0)),
                         "imbalance": float(f.get("imbalance", 0)), "max_visit_share": float(f.get("max_visit_share", 0)), "max_jobs": int(f.get("max_jobs", 0))})
        elif "serial:" in line:
            f = dict(FIELD_RE.findall(line))
            rows.append({"workers": 0, "serial_total_ms": float(f["serial_total_ms"]), "digest": f["digest"], "candidates": int(f["cand"]), "accepted": int(f["acc"])})
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lib", required=True)
    parser.add_argument("--src-root", required=True)
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--output", default=str(HERE / "CHAINE_Q2_PARALLEL_CHECKS.json"))
    parser.add_argument("--quick", action="store_true", help="familles adversariales seulement, pas d'échelle")
    parser.add_argument("--no-scale", action="store_true", help="pas de mesures 8k/16k/32k")
    parser.add_argument("--scale-only", action="store_true", help="seulement les mesures 8k/16k/32k (quatre familles, dont rangées)")
    parser.add_argument("--donate", action="store_true", help="redistribution dynamique : répète chaque appel pour Coarse, Donate{64,64} et Donate{1,1}")
    parser.add_argument("--coop", action="store_true", help="équipe coopérative front + census (tranche 17) : ajoute run_wspd_q2_census_cooperative pour W ∈ {1,2,3,4,8} × quatre réglages")
    parser.add_argument("--ranges", action="store_true", help="plages d'ancres et Pool partagé (tranche 18) : ajoute run_wspd_q2_census_ranges pour W ∈ {1,2,3,4,8} × quatre réglages")
    parser.add_argument("--batched", action="store_true", help="petits census entrelacés (tranche 19) : ajoute run_wspd_q2_census_batched pour W ∈ {1,2,3,4,8} × quatre réglages")
    args = parser.parse_args()
    lib = Path(args.lib).resolve(); src = Path(args.src_root).resolve(); build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    pinned = ([src / "src/pipeline/wspd_q2_batched.hpp"] if args.batched else []) + \
             ([src / "src/pipeline/wspd_q2_ranges.hpp", src / "src/pipeline/q2_node_pool.hpp"] if args.ranges else []) + \
             ([src / "src/pipeline/wspd_q2_cooperative.hpp", src / "src/pipeline/q2_census_resume.hpp", src / "src/pipeline/q2_census_parallel.hpp"] if args.coop else []) + \
             [src / "src/pipeline/wspd_q2_parallel.hpp", src / "src/pipeline/wspd_q2_census.hpp", src / "src/pipeline/q2_census.cpp",
              src / "src/pipeline/q2_node_pool.hpp", src / "src/parallel/joined_workers.hpp", src / "src/parallel/work_reduction.hpp",
              src / "src/wspd/front.cpp", src / "bench/front_fixtures.hpp", HERE / "chain_verify_parallel.cpp", lib]
    for p in pinned:
        if not p.is_file():
            return fail(f"fichier absent : {p}")
    pins = {str(p.relative_to(ROOT)) if str(p).startswith(str(ROOT)) else str(p): sha256(p) for p in pinned}
    binary = build / "chain_verify_parallel"
    compile_cmd = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", f"-I{src / 'src'}", f"-I{src / 'bench'}"]
    if args.donate:
        compile_cmd.append("-DMHGP8_AUDIT_DONATE")
    if args.coop:
        compile_cmd.append("-DMHGP8_AUDIT_COOP")
    if args.ranges:
        compile_cmd.append("-DMHGP8_AUDIT_RANGES")
    if args.batched:
        compile_cmd.append("-DMHGP8_AUDIT_BATCHED")
    compile_cmd += [str(HERE / "chain_verify_parallel.cpp"), str(lib), "-pthread", "-o", str(binary)]
    done = run(compile_cmd, HERE)
    if done.returncode != 0:
        return fail("compilation refusée :\n" + done.stderr)
    plan = []
    if args.scale_only:
        for fam in ("uniform", "clusters", "terrain", "rows"):
            for n in (8000, 16000, 32000):
                plan.append((fam, n, 10, 8, 3, True))
    for adv in (() if args.scale_only else ("grid5", "cospherical", "collinear", "cube_corners", "extremes", "halfint", "dense_ball")):
        for k in (1, 2, 5, 10):
            for s in (8, 12):
                plan.append((f"adv:{adv}", 0, k, s, 3, False))
    if not args.quick and not args.scale_only:
        for fam in ("uniform", "terrain", "clusters", "rows"):
            for k in (1, 3, 10):
                for s in (8, 12):
                    plan.append((fam, 800, k, s, 3, False))
            plan.append((fam, 800, 10, 8, 11, False))
        plan.append(("uniform", 2000, 10, 8, 3, False))
        plan.append(("clusters", 2000, 10, 8, 3, False))
        if not args.no_scale:
            for fam in ("uniform", "clusters", "terrain", "rows"):
                for n in (8000, 16000, 32000):
                    plan.append((fam, n, 10, 8, 3, True))
    runs = []
    for fam, n, k, s, seed, scale in plan:
        cmd = [str(binary), fam, str(n), str(k), str(s), str(seed)] + (["scale"] if scale else [])
        started = time.perf_counter()
        done = run(cmd, HERE)
        elapsed = time.perf_counter() - started
        summary = parse_summary(done.stdout)
        if summary is None or done.returncode != 0:
            return fail(f"échec {' '.join(cmd)} (code {done.returncode}) :\n{done.stdout[-3000:]}\n{done.stderr[-2000:]}")
        for key in ("mismatch", "cross_slot_dups", "digest_breaks", "counter_breaks", "coop_mismatch", "coop_digest_breaks", "coop_counter_breaks", "coop_ident_breaks", "rng_mismatch", "rng_digest_breaks", "rng_counter_breaks", "rng_ident_breaks", "bat_mismatch", "bat_digest_breaks", "bat_counter_breaks", "bat_ident_breaks"):
            if summary.get(key, 0) != 0:
                return fail(f"désaccord {key} : {' '.join(cmd)}\n{done.stdout}")
        if args.batched and summary.get("bat_liveness_runs", 0) != summary.get("bat_liveness_exceptions", 0):
            return fail(f"vivacité lots : exception non propagée : {' '.join(cmd)}\n{done.stdout}")
        if args.ranges and summary.get("rng_liveness_runs", 0) != summary.get("rng_liveness_exceptions", 0):
            return fail(f"vivacité plages : exception non propagée : {' '.join(cmd)}\n{done.stdout}")
        if args.coop and summary.get("coop_liveness_runs", 0) != summary.get("coop_liveness_exceptions", 0):
            return fail(f"vivacité coopérative : exception non propagée : {' '.join(cmd)}\n{done.stdout}")
        entry = {"command": " ".join(cmd), "wall_seconds": round(elapsed, 3), "summary": summary, "raw_stdout": done.stdout}
        if scale:
            entry["scale_rows"] = parse_scale(done.stdout)
        runs.append(entry)
        print(f"ok {' '.join(cmd[1:])} ({elapsed:.1f} s)", flush=True)
    verify_runs = [r for r in runs if "scale" not in r["command"]]
    totals = {"runs": len(runs), "verify_runs": len(verify_runs), "scale_runs": len(runs) - len(verify_runs),
              "parallel_runs": sum(r["summary"]["parallel_runs"] for r in runs),
              "pairs_checked": sum(r["summary"]["checked"] for r in verify_runs),
              "alive_pairs": sum(r["summary"]["alive"] for r in verify_runs),
              "mismatch": sum(r["summary"]["mismatch"] for r in runs),
              "cross_slot_dups": sum(r["summary"]["cross_slot_dups"] for r in runs),
              "digest_breaks": sum(r["summary"]["digest_breaks"] for r in runs),
              "counter_breaks": sum(r["summary"]["counter_breaks"] for r in runs),
              "donations": sum(r["summary"].get("donations", 0) for r in runs),
              "stolen": sum(r["summary"].get("stolen", 0) for r in runs)}
    for key in ("coop_runs", "coop_mismatch", "coop_digest_breaks", "coop_counter_breaks", "coop_ident_breaks", "coop_continued", "coop_donations", "coop_liveness_runs", "coop_liveness_exceptions", "rng_runs", "rng_mismatch", "rng_digest_breaks", "rng_counter_breaks", "rng_ident_breaks", "rng_donations", "rng_liveness_runs", "rng_liveness_exceptions", "bat_runs", "bat_mismatch", "bat_digest_breaks", "bat_counter_breaks", "bat_ident_breaks", "bat_enqueued", "bat_liveness_runs", "bat_liveness_exceptions"):
        totals[key] = sum(r["summary"].get(key, 0) for r in runs)
    receipt = {
        "title": "Chaîne parallèle front + census q2 : force brute, identité des condensés et des compteurs selon le nombre de fils",
        "date": "2026-09-14", "author_role": "auditeur indépendant B",
        "git_head": run(["git", "rev-parse", "HEAD"], ROOT).stdout.strip(), "src_root": str(src),
        "compile_command": " ".join(compile_cmd), "pins_sha256": pins, "totals": totals, "runs": runs, "donate_schedules": bool(args.donate), "cooperative": bool(args.coop), "anchor_ranges": bool(args.ranges), "batched": bool(args.batched),
        "stable_digest_without_times": hashlib.sha256(json.dumps([r["summary"] for r in runs], sort_keys=True).encode()).hexdigest(),
        "scope": "Exactitude, absence de doublon entre slots, identité bit à bit des supports et des compteurs discrets selon W ; les temps d'échelle sont indicatifs (hôte partagé), jamais un contrat ; aucune tour FULL.",
    }
    Path(args.output).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OK : {len(runs)} exécutions ({totals['parallel_runs']} appels parallèles), {totals['pairs_checked']} paires contrôlées, mismatch {totals['mismatch']}, doublons inter-slots {totals['cross_slot_dups']}, ruptures de condensé {totals['digest_breaks']}, reçu {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
