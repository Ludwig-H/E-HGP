#!/usr/bin/env python3
"""Reçu : vérification exécutée de la chaîne front + census q2 (auditeur B, 14 septembre 2026).

Compile chain_verify.cpp contre une extraction épinglée de e3af11a7 (--src-root, `git archive
e3af11a7 morsehgp3D_v8`) et sa bibliothèque produit (--lib), puis exécute, pour huit
combinaisons de modes (Pure/MidpointSamples × Pairwise/SharedBlocks × frère Disabled/Saturating ×
ordre GlobalDfs/ComplementFirst), une force brute exacte : toute paire non ordonnée à moins de
Kmax intérieurs stricts doit être émise exactement une fois avec ses IDs intérieurs, toute sa
coquille et sa clé ; aucune autre paire ne doit l'être. Écrit CHAINE_Q2_CHECKS.json. Sans assert.
Avec --joint (onzième tranche, `git archive b2106c3c`), dix combinaisons Q2AnchorMode SharedProduct/SharedAnchors
s'ajoutent et le reçu attendu est CHAINE_Q2_JOINT_CHECKS.json (passer --output).
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lib", required=True)
    parser.add_argument("--src-root", required=True)
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--output", default=str(HERE / "CHAINE_Q2_CHECKS.json"))
    parser.add_argument("--quick", action="store_true", help="familles adversariales seulement")
    parser.add_argument("--joint", action="store_true",
                        help="onzième tranche : ajoute les dix combinaisons Q2AnchorMode (SharedProduct/SharedAnchors)")
    args = parser.parse_args()
    lib = Path(args.lib).resolve(); src = Path(args.src_root).resolve(); build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    pinned = [src / "src/pipeline/wspd_q2_census.hpp", src / "src/pipeline/q2_census.cpp", src / "src/pipeline/q2_census.hpp",
              src / "src/wspd/front.cpp", src / "bench/front_fixtures.hpp", HERE / "chain_verify.cpp", lib]
    if args.joint:
        pinned.insert(3, src / "src/pipeline/q2_joint_bounds.hpp")
    for p in pinned:
        if not p.is_file():
            return fail(f"fichier absent : {p}")
    pins = {str(p.relative_to(ROOT)) if str(p).startswith(str(ROOT)) else str(p): sha256(p) for p in pinned}
    binary = build / "chain_verify"
    compile_cmd = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", f"-I{src / 'src'}", f"-I{src / 'bench'}"]
    if args.joint:
        compile_cmd.append("-DMHGP8_AUDIT_JOINT")
    compile_cmd += [str(HERE / "chain_verify.cpp"), str(lib), "-o", str(binary)]
    done = run(compile_cmd, HERE)
    if done.returncode != 0:
        return fail("compilation refusée :\n" + done.stderr)
    plan = []
    for adv in ("grid5", "cospherical", "collinear", "cube_corners", "extremes", "halfint", "dense_ball"):
        for k in (1, 2, 5, 10):
            for s in (8, 12):
                plan.append((f"adv:{adv}", 0, k, s, 3))
    if not args.quick:
        for fam in ("uniform", "terrain", "clusters", "rows"):
            for k in (1, 3, 10):
                for s in (8, 12):
                    plan.append((fam, 800, k, s, 3))
            plan.append((fam, 800, 10, 8, 11))
        plan.append(("uniform", 2000, 10, 8, 3))
        plan.append(("clusters", 2000, 10, 8, 3))
    runs = []
    for fam, n, k, s, seed in plan:
        cmd = [str(binary), fam, str(n), str(k), str(s), str(seed)]
        started = time.perf_counter()
        done = run(cmd, HERE)
        elapsed = time.perf_counter() - started
        summary = parse_summary(done.stdout)
        if summary is None or done.returncode != 0:
            return fail(f"échec {' '.join(cmd)} :\n{done.stdout}\n{done.stderr}")
        if summary.get("mismatch") != 0:
            return fail(f"désaccord détecté : {' '.join(cmd)}\n{done.stdout}")
        runs.append({"command": " ".join(cmd), "wall_seconds": round(elapsed, 3), "summary": summary, "raw_stdout": done.stdout})
    combos_per_run = sorted({r["summary"].get("combos", 8) for r in runs})
    totals = {"runs": len(runs), "combos_per_run": combos_per_run[0] if len(combos_per_run) == 1 else combos_per_run,
              "pairs_checked": sum(r["summary"]["checked"] for r in runs),
              "alive_pairs": sum(r["summary"]["alive"] for r in runs), "emitted": sum(r["summary"]["emitted"] for r in runs),
              "mismatch": sum(r["summary"]["mismatch"] for r in runs)}
    if args.joint:
        for key in ("joint_rejected", "joint_accepted", "joint_handoffs", "joint_handoffs_after_credit"):
            totals[key] = sum(r["summary"].get(key, 0) for r in runs)
    receipt = {
        "title": ("Chaîne front + census q2 conjoint (onzième tranche, Q2AnchorMode) : force brute exacte sur dix-huit combinaisons"
                  if args.joint else "Chaîne front + census q2 (e3af11a7) : force brute exacte sur toutes les combinaisons de modes"),
        "joint_modes": bool(args.joint),
        "date": "2026-09-14", "author_role": "auditeur indépendant B",
        "git_head": run(["git", "rev-parse", "HEAD"], ROOT).stdout.strip(), "src_root": str(src),
        "compile_command": " ".join(compile_cmd), "pins_sha256": pins, "totals": totals, "runs": runs,
        "stable_digest_without_times": hashlib.sha256(json.dumps([r["summary"] for r in runs], sort_keys=True).encode()).hexdigest(),
        "scope": "Vérification d'exactitude et de complétude du flux de supports q2 sur petits nuages ; aucun temps produit, aucune tour FULL.",
    }
    Path(args.output).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OK : {len(runs)} exécutions × {totals['combos_per_run']} combinaisons, {totals['pairs_checked']} paires contrôlées, mismatch {totals['mismatch']}, reçu {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
