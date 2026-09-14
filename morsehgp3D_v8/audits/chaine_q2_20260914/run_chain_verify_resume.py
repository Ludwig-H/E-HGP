#!/usr/bin/env python3
"""Reçu : vérification des continuations de census q2 (auditeur B, 14 septembre 2026).

Compile chain_verify_resume.cpp contre une extraction épinglée (--src-root) et sa bibliothèque (--lib),
puis, pour des couples (ancre, nœud B) tirés de l'index de chaque nuage, trois réglages d'options
(Global ; frère + Complement ; Complement) et des budgets de transitions 1, 3 (avec transfert de fil
à chaque pas) et 1000 : la continuation avancée jusqu'à Done doit émettre exactement le multiensemble
de supports de la référence série, elle-même égale à la force brute ; compteurs discrets finaux égaux ;
aucun pas supplémentaire après Done n'émet ; les pauses après crédit, en phase différée et pendant
l'émission doivent être exercées. Écrit CHAINE_Q2_RESUME_CHECKS.json. Sans assert.
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
    parser.add_argument("--output", default=str(HERE / "CHAINE_Q2_RESUME_CHECKS.json"))
    parser.add_argument("--quick", action="store_true", help="familles adversariales seulement")
    args = parser.parse_args()
    lib = Path(args.lib).resolve(); src = Path(args.src_root).resolve(); build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    pinned = [src / "src/pipeline/q2_census_resume.hpp", src / "src/pipeline/q2_census.cpp", src / "src/pipeline/q2_census.hpp",
              src / "src/pipeline/wspd_q2_census.hpp", src / "bench/front_fixtures.hpp", HERE / "chain_verify_resume.cpp", lib]
    for p in pinned:
        if not p.is_file():
            return fail(f"fichier absent : {p}")
    pins = {str(p.relative_to(ROOT)) if str(p).startswith(str(ROOT)) else str(p): sha256(p) for p in pinned}
    binary = build / "chain_verify_resume"
    compile_cmd = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", f"-I{src / 'src'}", f"-I{src / 'bench'}",
                   str(HERE / "chain_verify_resume.cpp"), str(lib), "-pthread", "-o", str(binary)]
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
                plan.append((fam, 800, k, 8, 3))
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
            return fail(f"échec {' '.join(cmd)} (code {done.returncode}) :\n{done.stdout[-3000:]}\n{done.stderr[-2000:]}")
        for key in ("ref_mismatch", "mismatch", "counter_breaks", "after_done_emissions"):
            if summary.get(key) != 0:
                return fail(f"désaccord {key} : {' '.join(cmd)}\n{done.stdout}")
        runs.append({"command": " ".join(cmd), "wall_seconds": round(elapsed, 3), "summary": summary, "raw_stdout": done.stdout})
        print(f"ok {' '.join(cmd[1:])} ({elapsed:.1f} s)", flush=True)
    keys = ("pairs", "continuations", "transfers", "alive", "ref_mismatch", "mismatch", "counter_breaks", "after_done_emissions",
            "pauses", "pauses_after_credit", "pauses_inside_deferred", "pauses_during_emission", "mid_range_pauses")
    totals = {k: sum(r["summary"].get(k, 0) for r in runs) for k in keys}
    totals["runs"] = len(runs); totals["max_pending"] = max(r["summary"].get("max_pending", 0) for r in runs)
    if min(totals["pauses_after_credit"], totals["pauses_inside_deferred"], totals["pauses_during_emission"]) == 0:
        return fail("un type de pause n'a pas été exercé : " + json.dumps(totals))
    receipt = {
        "title": "Continuations de census q2 : force brute, référence série, budgets 1/3/1000, transfert de fil, pauses classées",
        "date": "2026-09-14", "author_role": "auditeur indépendant B",
        "git_head": run(["git", "rev-parse", "HEAD"], ROOT).stdout.strip(), "src_root": str(src),
        "compile_command": " ".join(compile_cmd), "pins_sha256": pins, "totals": totals, "runs": runs,
        "stable_digest_without_times": hashlib.sha256(json.dumps([r["summary"] for r in runs], sort_keys=True).encode()).hexdigest(),
        "scope": "Exactitude et reprise des continuations à ancre unique ; aucun temps produit, aucune tour FULL, aucun raccord Pool/joint.",
    }
    Path(args.output).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OK : {len(runs)} exécutions, {totals['continuations']} continuations ({totals['transfers']} avec transfert de fil), mismatch {totals['mismatch']}, pauses après crédit {totals['pauses_after_credit']}, en phase différée {totals['pauses_inside_deferred']}, pendant émission {totals['pauses_during_emission']}, reçu {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
