#!/usr/bin/env python3
"""Reçu : crédits locaux P0 sur les rectangles terminaux du premier front v8 (auditeur B, 14 sept. 2026).

Trois harnais compilés contre une extraction épinglée de da366f7f (--src-root) et la
bibliothèque produit (--lib) : (1) clusters_credits : Pool/DualBlocks/Tubes sur trois
produits inter-amas de la fixture clusters ; (2) tube_width : balayage de la largeur de
cellule des tubes (copie d'audit tube_audit) ; (3) front_pool : front réel MidpointSamples
puis crédits Pool/DualBlocks sur les rectangles terminaux au-dessus d'un seuil de taille.
Écrit CREDITS_TERMINAUX_CHECKS.json avec pins, commandes et sorties brutes. Sans assert.
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
FIELD_RE = re.compile(r"(\w+)=([-\w.]+)")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fail(message: str) -> int:
    print(f"ECHEC : {message}", file=sys.stderr)
    return 1


def run(cmd: list[str], cwd: Path, env: dict | None = None) -> subprocess.CompletedProcess:
    import os
    full = dict(os.environ)
    if env:
        full.update(env)
    return subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True, env=full)


def fields(line: str) -> dict:
    out = {}
    for key, value in FIELD_RE.findall(line):
        if key in out:
            continue
        try:
            out[key] = int(value)
        except ValueError:
            try:
                out[key] = float(value)
            except ValueError:
                out[key] = value
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lib", required=True)
    parser.add_argument("--src-root", required=True, help="extraction `git archive da366f7f morsehgp3D_v8/src morsehgp3D_v8/bench`")
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--output", default=str(HERE / "CREDITS_TERMINAUX_CHECKS.json"))
    parser.add_argument("--quick", action="store_true", help="8k seulement (rejeu rapide)")
    args = parser.parse_args()
    lib = Path(args.lib).resolve(); src = Path(args.src_root).resolve(); build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    sources = {
        "clusters_credits": HERE / "clusters_credits.cpp", "front_pool": HERE / "front_pool.cpp", "tube_width": HERE / "tube_width.cpp",
    }
    pinned = [src / "src/pipeline/tube_credits.hpp", src / "src/pipeline/local_credits.hpp", src / "src/pipeline/local_credits.cpp",
              src / "src/wspd/front.cpp", src / "bench/front_fixtures.hpp", HERE / "wide/pipeline/tube_credits.hpp", lib] + list(sources.values())
    for p in pinned:
        if not p.is_file():
            return fail(f"fichier absent : {p}")
    pins = {str(p.relative_to(ROOT)) if str(p).startswith(str(ROOT)) else str(p): sha256(p) for p in pinned}
    binaries = {}
    for name, cpp in sources.items():
        cmd = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra"]
        if name == "tube_width":
            cmd += [f"-I{HERE / 'wide/pipeline'}", f"-I{src / 'src/pipeline'}"]
        cmd += [f"-I{src / 'src'}", f"-I{src / 'bench'}", str(cpp), str(lib), "-o", str(build / name)]
        done = run(cmd, HERE)
        if done.returncode != 0:
            return fail(f"compilation refusée ({name}) :\n{done.stderr}")
        binaries[name] = (str(build / name), " ".join(cmd))
    sizes = [8000] if args.quick else [8000, 16000, 32000]
    plan = []
    for n in sizes:
        plan.append(("clusters_credits", [str(n)], {}))
    for mul in (1, 16, 64, 256, 1024):
        plan.append(("tube_width", ["8000" if args.quick else "32000", "0", "1"], {"MHGP8_TUBE_WIDTH_MUL": str(mul)}))
    for n in sizes:
        plan.append(("front_pool", [str(n), "clusters", "10", "8", "64", "pool", "samples"], {}))
        plan.append(("front_pool", [str(n), "clusters", "10", "8", "64", "dual", "samples"], {}))
    plan.append(("front_pool", ["8000", "clusters", "10", "8", "2", "pool", "samples"], {}))
    plan.append(("front_pool", ["8000", "uniform", "10", "8", "8", "pool", "samples"], {}))
    plan.append(("front_pool", ["8000", "uniform", "10", "8", "2", "pool", "samples"], {}))
    plan.append(("front_pool", ["8000", "terrain", "10", "8", "8", "pool", "samples"], {}))
    runs = []
    for name, argv, env in plan:
        cmd = [binaries[name][0]] + argv
        started = time.perf_counter()
        done = run(cmd, HERE, env)
        elapsed = time.perf_counter() - started
        if done.returncode != 0:
            return fail(f"échec {' '.join(cmd)} :\n{done.stdout}\n{done.stderr}")
        lines = [fields(l) for l in done.stdout.strip().splitlines() if "=" in l]
        runs.append({"harness": name, "command": " ".join(cmd), "env": env, "wall_seconds": round(elapsed, 3),
                     "parsed_lines": lines, "raw_stdout": done.stdout})
    receipt = {
        "title": "Crédits locaux P0 sur les rectangles terminaux du premier front v8 et largeur des tubes sur nuages irréguliers",
        "date": "2026-09-14", "author_role": "auditeur indépendant B",
        "git_head": run(["git", "rev-parse", "HEAD"], ROOT).stdout.strip(), "src_root": str(src),
        "compile_commands": {k: v[1] for k, v in binaries.items()}, "pins_sha256": pins, "runs": runs,
        "stable_digest_without_times": hashlib.sha256(json.dumps([[{k: v for k, v in l.items() if not k.endswith("_ms") and k != "ms"} for l in r["parsed_lines"]] for r in runs], sort_keys=True).encode()).hexdigest(),
        "scope": "Harnais d'audit hors produit, compilés contre da366f7f ; compteurs déterministes, temps indicatifs (machine partagée).",
    }
    Path(args.output).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OK : {len(runs)} exécutions, reçu {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
