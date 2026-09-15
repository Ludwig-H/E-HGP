#!/usr/bin/env python3
"""Reçu : croissance q3 sur deux rangées parallèles, par l'oracle i128 (auditeur B, 15 septembre 2026).

Deux rangées de m sites (pas δ = 4, plan z = 100) à distance D ∈ {40, 200} (D/δ = 10 et 50), m ∈ {25, 50, 100},
Kmax ∈ {5, 10} : nombre de triangles aigus (candidats q3 de toute propriété), nombre de présentations q3
retenues (p < h3), part de celles dont l'arête maximale est croisée, et nombre de triangles à coquille
excédentaire. Vérifie numériquement l'argument corrigé du dialogue : supports vivants linéaires en m et
indépendants de D, candidats cubiques en m quand D domine. Sans assert. Écrit ROWS_Q3_GROWTH_CHECKS.json.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True)
    parser.add_argument("--output", default=str(HERE / "ROWS_Q3_GROWTH_CHECKS.json"))
    args = parser.parse_args()
    binary = Path(args.binary).resolve()
    delta = 4
    runs = []
    for m in (25, 50, 100):
        for D in (40, 200):
            pts = [(100, 100 + i * delta, 100) for i in range(m)] + [(100 + D, 100 + i * delta, 100) for i in range(m)]
            with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as f:
                f.write("\n".join(f"{x} {y} {z}" for x, y, z in pts) + "\n"); name = f.name
            for k in (5, 10):
                done = subprocess.run([str(binary), name, str(k), "q3only"], capture_output=True, text=True)
                if done.returncode != 0:
                    print("ECHEC :", done.stderr, file=sys.stderr); return 1
                summary = {}
                cross = 0; kept = 0
                for line in done.stdout.splitlines():
                    if line.startswith("SUMMARY"):
                        summary = dict(kv.split("=") for kv in line.split()[1:]); continue
                    if not line.startswith("q3 "):
                        continue
                    kept += 1
                    ids = [int(i) for i in line.split()[1].split(",")]
                    P = [pts[i] for i in ids]
                    edges = [((P[i][0] - P[j][0]) ** 2 + (P[i][1] - P[j][1]) ** 2, i, j) for i, j in ((0, 1), (0, 2), (1, 2))]
                    _, i, j = max(edges)
                    if P[i][0] != P[j][0]:
                        cross += 1
                runs.append({"m": m, "D": D, "D_over_delta": D // delta, "kmax": k, "acute_triangles": int(summary["q3_positive"]),
                             "alive_q3": kept, "alive_cross_longest_edge": cross, "extra_shell": int(summary["q3_extra_shell"])})
                print(f"m={m} D={D} K={k}: acute={summary['q3_positive']} alive={kept} cross={cross} extra_shell={summary['q3_extra_shell']}", flush=True)
            Path(name).unlink()
    receipt = {"title": "Croissance q3 sur deux rangées parallèles (oracle i128) : candidats, supports vivants, coquilles excédentaires",
               "date": "2026-09-15", "author_role": "auditeur indépendant B",
               "git_head": subprocess.run(["git", "rev-parse", "HEAD"], cwd=str(ROOT), capture_output=True, text=True).stdout.strip(),
               "pins_sha256": {"oracle_q3q4.cpp": hashlib.sha256((HERE / "oracle_q3q4.cpp").read_bytes()).hexdigest()},
               "runs": runs, "scope": "Mesure d'audit bornée (n ≤ 200) ; aucun statut public ; les rangées sont hors domaine générique pour q3."}
    Path(args.output).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OK : {len(runs)} mesures, reçu {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
