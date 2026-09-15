#!/usr/bin/env python3
"""Reçu : oracle q3/q4 indépendant (i128) confronté au catalogue rationnel de reference/ (auditeur B, 15 sept. 2026).

Sur des petits nuages entiers (n ≤ 12, coordonnées serrées pour provoquer angles droits, cosphéricités et
coquilles excédentaires) et sur les deux contre-fixtures du constructeur, compare l'ensemble des présentations
positives de cardinal 3 et 4 (support, intérieurs, coquille) retenues sous p + q ≤ Kmax + 1 par oracle_q3q4.cpp
avec les événements de build_critical_catalog (reference.morsehgp3d_oracle), ainsi que les coquilles
excédentaires avec ses candidats dégénérés. Sans assert. Écrit ORACLE_Q3Q4_CHECKS.json.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import random
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT))
from reference.morsehgp3d_oracle.catalog import build_critical_catalog  # noqa: E402


def fail(message: str) -> int:
    print(f"ECHEC : {message}", file=sys.stderr)
    return 1


def run_oracle(binary: Path, points: list[tuple[int, int, int]], kmax: int):
    with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as f:
        for p in points:
            f.write(f"{p[0]} {p[1]} {p[2]}\n")
        name = f.name
    done = subprocess.run([str(binary), name, str(kmax)], capture_output=True, text=True)
    Path(name).unlink()
    if done.returncode != 0:
        raise RuntimeError(done.stderr)
    kept, extra = set(), set()
    summary = {}
    for line in done.stdout.splitlines():
        if line.startswith("SUMMARY"):
            summary = dict(kv.split("=") for kv in line.split()[1:])
            continue
        parts = line.split()
        support = tuple(int(x) for x in parts[1].split(","))
        fields = dict(kv.split("=") for kv in parts[2:])
        ids = lambda s: tuple() if s == "-" else tuple(int(x) for x in s.split(","))
        entry = (support, ids(fields["interior"]), ids(fields["shell"]))
        (extra if fields["extra"] == "1" else kept).add(entry)
    return kept, extra, summary


def reference_sets(points, kmax):
    cat = build_critical_catalog(points, kmax)
    if tuple(cat.points) != tuple(points):
        raise RuntimeError("le catalogue a réordonné les points")
    kept = {(tuple(e.minimal_support_ids), tuple(e.interior_ids), tuple(e.shell_ids)) for e in cat.events if len(e.minimal_support_ids) >= 3}
    # la référence enregistre UNE dégénérescence par boule, sous le support de plus petite clé (souvent q1/q2) :
    # on compare donc par boule (intérieurs, coquille), tous supports confondus
    degenerate = {(tuple(d.ball.support_ids), tuple(d.interior_ids), tuple(d.shell_ids)) for d in cat.degenerate_candidates if d.reason == "extra_shell"}
    return kept, degenerate


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True)
    parser.add_argument("--clouds", type=int, default=300)
    parser.add_argument("--output", default=str(HERE / "ORACLE_Q3Q4_CHECKS.json"))
    args = parser.parse_args()
    binary = Path(args.binary).resolve()
    rng = random.Random(20260915)
    fixtures = {
        "constructeur_q3": [(900, 1000, 1000), (1100, 1000, 1000), (1000, 1120, 1000)] + [(1000 + j, 910, 1000) for j in range(5)],
        "constructeur_q4": [(900, 1000, 1000), (1100, 1000, 1000), (1000, 1120, 1040), (1000, 1120, 960)] + [(1000 + j, 910, 1000) for j in range(5)],
        "tetraedre_regulier_entier": [(0, 0, 0), (1, 1, 0), (1, 0, 1), (0, 1, 1), (2, 2, 2)],
        "triangle_droit": [(0, 0, 0), (4, 0, 0), (0, 3, 0), (7, 7, 7)],
        "cube_cospherique": [(0, 0, 0), (0, 0, 2), (0, 2, 0), (0, 2, 2), (2, 0, 0), (2, 0, 2), (2, 2, 0), (2, 2, 2), (1, 1, 1)],
    }
    plan = [(name, sorted(set(pts)), k) for name, pts in fixtures.items() for k in (2, 5, 10)]
    for i in range(args.clouds):
        n = rng.randint(4, 12); box = rng.choice((3, 4, 6, 10, 40, 65535))
        pts = set()
        while len(pts) < n:
            pts.add((rng.randint(0, box), rng.randint(0, box), rng.randint(0, box)))
        plan.append((f"random_{i}", sorted(pts), rng.choice((2, 3, 5, 10))))
    runs = []; totals = {"clouds": 0, "kept_q3": 0, "kept_q4": 0, "extra_shell": 0, "mismatch": 0}
    for name, pts, k in plan:
        if len(pts) < 3:
            continue
        kept_o, extra_o, summary = run_oracle(binary, pts, k)
        kept_r, degenerate_r = reference_sets(pts, k)
        # dans reference, les candidats dégénérés ne sont conservés que si p + |support| <= s_max ; même filtre ici
        s_max = min(min(k, len(pts)) + 1, len(pts))
        # reference conserve UN candidat dégénéré par boule (identité centre, rayon, intérieurs, coquille) ; l'oracle liste
        # chaque présentation : comparer les coquilles excédentaires par boule, c'est-à-dire par (intérieurs, coquille)
        extra_o_f = {(e[1], e[2]) for e in extra_o if len(e[1]) + len(e[0]) <= s_max}
        degenerate_r = {(d[1], d[2]) for d in degenerate_r}
        # reference garde dans kept les événements dont closed_rank <= s_max : p + |support| <= s_max ; l'oracle filtre p < h_q = k + 2 - q
        kept_o_f = {e for e in kept_o if len(e[1]) + len(e[0]) <= s_max}
        # inclusion : toute coquille excédentaire d'une présentation q3/q4 positive doit être une dégénérescence de la référence
        # (la réciproque n'est pas attendue : une boule dégénérée peut n'avoir aucune présentation positive de cardinal 3 ou 4)
        ok = kept_o_f == kept_r and extra_o_f <= degenerate_r
        if not ok:
            totals["mismatch"] += 1
            print(f"MISMATCH {name} k={k} n={len(pts)} oracle-only={sorted(kept_o_f - kept_r)[:3]} ref-only={sorted(kept_r - kept_o_f)[:3]} extra-only={sorted(extra_o_f - degenerate_r)[:3]} deg-only={sorted(degenerate_r - extra_o_f)[:3]}")
        totals["clouds"] += 1; totals["kept_q3"] += sum(1 for e in kept_o_f if len(e[0]) == 3); totals["kept_q4"] += sum(1 for e in kept_o_f if len(e[0]) == 4); totals["extra_shell"] += len(extra_o_f)
        runs.append({"name": name, "n": len(pts), "kmax": k, "ok": ok, "kept": len(kept_o_f), "extra_shell": len(extra_o_f), "summary": summary})
    receipt = {"title": "Oracle q3/q4 i128 contre le catalogue rationnel de référence (n ≤ 12)", "date": "2026-09-15", "author_role": "auditeur indépendant B",
               "git_head": subprocess.run(["git", "rev-parse", "HEAD"], cwd=str(ROOT), capture_output=True, text=True).stdout.strip(),
               "pins_sha256": {"oracle_q3q4.cpp": hashlib.sha256((HERE / "oracle_q3q4.cpp").read_bytes()).hexdigest(), "reference/morsehgp3d_oracle/catalog.py": hashlib.sha256((ROOT / "reference/morsehgp3d_oracle/catalog.py").read_bytes()).hexdigest()},
               "totals": totals, "runs": runs, "scope": "Vérité terrain bornée pour les présentations q3/q4 ; aucun statut public, aucun backend."}
    Path(args.output).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OK : {totals['clouds']} nuages, {totals['kept_q3']} présentations q3 et {totals['kept_q4']} q4 retenues, {totals['extra_shell']} coquilles excédentaires, mismatch {totals['mismatch']}, reçu {args.output}")
    return 0 if totals["mismatch"] == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
