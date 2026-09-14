#!/usr/bin/env python3
"""Reçu : vérité terrain sur les survivantes des crédits terminaux (auditeur B, 14 septembre 2026).

Compile front_pool_truth.cpp contre l'extraction épinglée de da366f7f (--src-root) et sa bibliothèque
(--lib), puis, pour la famille clusters à 8k/16k/32k (Kmax 10, s 8, seuil 64), Pool et DualBlocks :
compte exact des sites strictement intérieurs (force brute arrêtée à Kmax) pour chaque paire q2
survivante des gros rectangles terminaux, et sûreté par échantillon déterministe des paires rejetées.
Écrit SURVIVANTS_CHECKS.json. Sans assert.
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


def parse(stdout: str) -> dict | None:
    lines = stdout.splitlines()
    if not lines or not lines[0].startswith("family="):
        return None
    out = {}
    for k, v in FIELD_RE.findall(lines[0]):
        try:
            out[k] = int(v)
        except ValueError:
            try:
                out[k] = float(v)
            except ValueError:
                out[k] = v
    for line in lines[1:]:
        if line.strip().startswith("hist_interior:"):
            out["hist_interior"] = {int(a): int(b) for a, b in re.findall(r"(\d+):(\d+)", line)}
        elif line.strip().startswith("alive_per_big_rect"):
            out["alive_per_big_rect"] = [int(x) for x in re.findall(r"\b(\d+)\b", line.split(":", 1)[1])]
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lib", required=True)
    parser.add_argument("--src-root", required=True)
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--output", default=str(HERE / "SURVIVANTS_CHECKS.json"))
    parser.add_argument("--sizes", default="8000,16000,32000")
    parser.add_argument("--quick", action="store_true", help="8k seulement, Pool seulement")
    args = parser.parse_args()
    lib = Path(args.lib).resolve(); src = Path(args.src_root).resolve(); build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    pinned = [src / "src/pipeline/local_credits.cpp", src / "src/pipeline/local_credits.hpp", src / "src/pipeline/q2_census.cpp",
              src / "src/wspd/front.cpp", src / "bench/front_fixtures.hpp", HERE / "front_pool_truth.cpp", lib]
    for p in pinned:
        if not p.is_file():
            return fail(f"fichier absent : {p}")
    pins = {str(p.relative_to(ROOT)) if str(p).startswith(str(ROOT)) else str(p): sha256(p) for p in pinned}
    binary = build / "front_pool_truth"
    compile_cmd = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", f"-I{src / 'src'}", f"-I{src / 'bench'}",
                   str(HERE / "front_pool_truth.cpp"), str(lib), "-o", str(binary)]
    done = run(compile_cmd, HERE)
    if done.returncode != 0:
        return fail("compilation refusée :\n" + done.stderr)
    sizes = [8000] if args.quick else [int(x) for x in args.sizes.split(",")]
    strategies = ["pool"] if args.quick else ["pool", "dual"]
    runs = []
    for n in sizes:
        for strat in strategies:
            cmd = [str(binary), str(n), "clusters", "10", "8", "64", strat, "20000"]
            started = time.perf_counter()
            done = run(cmd, HERE)
            elapsed = time.perf_counter() - started
            summary = parse(done.stdout)
            if summary is None or done.returncode != 0:
                return fail(f"échec {' '.join(cmd)} (code {done.returncode}) :\n{done.stdout}\n{done.stderr}")
            if summary.get("rejected_unsound") != 0:
                return fail(f"crédit non sûr détecté : {' '.join(cmd)}\n{done.stdout}")
            runs.append({"command": " ".join(cmd), "wall_seconds": round(elapsed, 3), "summary": summary, "raw_stdout": done.stdout})
    totals = {"runs": len(runs), "survivors": sum(r["summary"]["survivors"] for r in runs),
              "alive": sum(r["summary"]["alive"] for r in runs), "rejected_sampled": sum(r["summary"]["rejected_sampled"] for r in runs),
              "rejected_unsound": sum(r["summary"]["rejected_unsound"] for r in runs)}
    receipt = {
        "title": "Survivantes des crédits terminaux (da366f7f, amas 8k/16k/32k) : vérité terrain par force brute bornée",
        "date": "2026-09-14", "author_role": "auditeur indépendant B",
        "git_head": run(["git", "rev-parse", "HEAD"], ROOT).stdout.strip(), "src_root": str(src),
        "compile_command": " ".join(compile_cmd), "pins_sha256": pins, "totals": totals, "runs": runs,
        "stable_digest_without_times": hashlib.sha256(json.dumps([{k: v for k, v in r["summary"].items() if not k.endswith("_ms")} for r in runs], sort_keys=True).encode()).hexdigest(),
        "scope": "Mesure d'audit sur la famille clusters ; aucun temps produit, aucune tour FULL ; la sûreté des rejets n'est contrôlée que par échantillon.",
    }
    Path(args.output).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OK : {len(runs)} exécutions, {totals['survivors']} survivantes dont {totals['alive']} vivantes, {totals['rejected_sampled']} rejetées échantillonnées, {totals['rejected_unsound']} non sûres, reçu {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
