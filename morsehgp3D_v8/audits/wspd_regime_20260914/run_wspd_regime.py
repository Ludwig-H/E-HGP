#!/usr/bin/env python3
"""Reçu de régime WSPD (auditeur v8, 14 septembre 2026).

Compile le harnais wspd_factor_hist.cpp contre les en-têtes v4 épinglés,
exécute une matrice de fronts WSPD purs (s=8 ; familles du plan de test),
puis écrit WSPD_REGIME_CHECKS.json : pins SHA-256, commandes, sorties brutes,
champs analysés et rapports par doublement. Aucune assertion Python : le
script se comporte de la même façon sous ``python3 -O``. Il n'écrit que dans
son propre dossier et dans le répertoire de build passé en argument.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
HARNESS = HERE / "wspd_factor_hist.cpp"
PINNED = [
    HARNESS,
    ROOT / "morsehgp3D_v4/src/cloud/families.hpp",
    ROOT / "morsehgp3D_v4/src/tree/radix_tree.hpp",
    ROOT / "morsehgp3D_v4/src/wspd/wavefront.hpp",
]
MATRIX = [
    ("uniform", 8000, 8, "diam"), ("uniform", 16000, 8, "diam"), ("uniform", 32000, 8, "diam"),
    ("uniform", 64000, 8, "diam"), ("uniform", 128000, 8, "diam"), ("uniform", 256000, 8, "diam"),
    ("terrain", 8000, 8, "diam"), ("terrain", 16000, 8, "diam"), ("terrain", 32000, 8, "diam"),
    ("eight_clusters", 8000, 8, "diam"), ("eight_clusters", 16000, 8, "diam"), ("eight_clusters", 32000, 8, "diam"),
    ("uniform", 32000, 10, "diam"), ("uniform", 32000, 12, "diam"), ("uniform", 32000, 14, "diam"),
    ("uniform", 32000, 16, "diam"), ("uniform", 32000, 18, "diam"),
    ("uniform", 8000, 8, "level"), ("uniform", 16000, 8, "level"), ("uniform", 32000, 8, "level"), ("uniform", 64000, 8, "level"),
]
HEAD_RE = re.compile(
    r"split=(\w+) famille=(\w+) n=(\d+) s=(\d+) seed=(\d+) uniques=(\d+) rectangles=(\d+) sum_factors=(\d+) "
    r"max_factor=(\d+) both_ge8=(\d+) both_ge64=(\d+) masse=(\d+) masse_attendue=(\d+)")
BUCKET_RE = re.compile(r"^\s*(\S+)\s+rect=\s*(\d+) \(\s*([0-9.]+)%\)\s+masse_paires=(\d+) \(\s*([0-9.]+)%\)")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fail(message: str) -> int:
    print(f"ECHEC : {message}", file=sys.stderr)
    return 1


def run(cmd: list[str], cwd: Path) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True)


def parse(stdout: str) -> dict | None:
    lines = stdout.strip().splitlines()
    if not lines:
        return None
    head = HEAD_RE.match(lines[0])
    if head is None:
        return None
    buckets = []
    for line in lines[1:]:
        m = BUCKET_RE.match(line)
        if m is None:
            return None
        buckets.append({"class": m.group(1), "rectangles": int(m.group(2)), "rect_pct": float(m.group(3)),
                        "pair_mass": int(m.group(4)), "mass_pct": float(m.group(5))})
    if len(buckets) != 5:
        return None
    return {
        "split": head.group(1), "family": head.group(2), "n": int(head.group(3)), "s": int(head.group(4)),
        "seed": int(head.group(5)), "uniques": int(head.group(6)), "rectangles": int(head.group(7)),
        "sum_factors": int(head.group(8)), "max_factor": int(head.group(9)), "both_ge8": int(head.group(10)),
        "both_ge64": int(head.group(11)), "pair_mass": int(head.group(12)), "expected_pair_mass": int(head.group(13)),
        "buckets": buckets,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, help="répertoire neuf pour le binaire (hors dépôt de préférence)")
    parser.add_argument("--output", default=str(HERE / "WSPD_REGIME_CHECKS.json"))
    parser.add_argument("--skip-large", action="store_true", help="omettre n>=128000 (rejeu rapide)")
    args = parser.parse_args()
    build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    for path in PINNED:
        if not path.is_file():
            return fail(f"source épinglée absente : {path}")
    pins = {str(p.relative_to(ROOT)): sha256(p) for p in PINNED}
    head = run(["git", "rev-parse", "HEAD"], ROOT)
    gxx = run(["g++", "--version"], ROOT)
    binary = build / "wspd_factor_hist"
    compile_cmd = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                   "-o", str(binary), str(HARNESS)]
    compiled = run(compile_cmd, HERE)
    if compiled.returncode != 0:
        return fail("compilation refusée :\n" + compiled.stderr)
    runs = []
    for family, n, s, split in MATRIX:
        if args.skip_large and n >= 128000:
            continue
        cmd = [str(binary), f"--family={family}", f"--n={n}", f"--s={s}", "--seed=3"]
        if split == "level":
            cmd.append("--split=level")
        started = time.perf_counter()
        done = run(cmd, HERE)
        elapsed = time.perf_counter() - started
        parsed = parse(done.stdout) if done.returncode == 0 else None
        if parsed is None:
            return fail(f"sortie inattendue pour {' '.join(cmd)} :\n{done.stdout}\n{done.stderr}")
        if parsed["family"] != family or parsed["n"] != n or parsed["s"] != s or parsed["split"] != split:
            return fail(f"la sortie ne correspond pas à la commande {' '.join(cmd)}")
        mass = sum(b["pair_mass"] for b in parsed["buckets"])
        if mass != parsed["pair_mass"] or mass != parsed["expected_pair_mass"]:
            return fail(f"ledger de masse violé ({mass}, {parsed['pair_mass']}, {parsed['expected_pair_mass']}) pour {' '.join(cmd)}")
        if parsed["uniques"] == n and mass != n * (n - 1) // 2:
            return fail(f"masse {mass} != C(n,2) pour {' '.join(cmd)}")
        parsed["command"] = " ".join(cmd)
        parsed["wall_seconds"] = round(elapsed, 3)
        parsed["raw_stdout"] = done.stdout
        runs.append(parsed)
    # rapports par doublement, par (famille, s, split)
    ratios = []
    by_key = {}
    for r in runs:
        by_key.setdefault((r["family"], r["s"], r["split"]), []).append(r)
    for key, series in by_key.items():
        series.sort(key=lambda r: r["n"])
        for prev, cur in zip(series, series[1:]):
            if cur["n"] == 2 * prev["n"]:
                ratios.append({"family": key[0], "s": key[1], "split": key[2], "from_n": prev["n"], "to_n": cur["n"],
                               "rectangles_ratio": round(cur["rectangles"] / prev["rectangles"], 3),
                               "sum_factors_ratio": round(cur["sum_factors"] / prev["sum_factors"], 3),
                               "rect_per_point_from": round(prev["rectangles"] / prev["n"], 1),
                               "rect_per_point_to": round(cur["rectangles"] / cur["n"], 1)})
    stable = hashlib.sha256(json.dumps([{k: v for k, v in r.items() if k not in ("wall_seconds",)} for r in runs],
                                       sort_keys=True).encode()).hexdigest()
    receipt = {
        "title": "Régime des rectangles du front WSPD pur v4 (s=8 ; familles du plan de test)",
        "date": "2026-09-14",
        "author_role": "auditeur indépendant v8",
        "git_head": head.stdout.strip(),
        "compiler": gxx.stdout.splitlines()[0] if gxx.stdout else "",
        "compile_command": " ".join(compile_cmd),
        "pins_sha256": pins,
        "matrix_size": len(runs),
        "runs": runs,
        "doubling_ratios": ratios,
        "stable_digest_without_times": stable,
        "scope": "Front WSPD pur (sans élimination par témoins), rectangles terminaux ; ce n'est ni le front fusionné v7 ni un moteur v8. Les temps muraux sont indicatifs (machine partagée).",
    }
    Path(args.output).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OK : {len(runs)} exécutions, digest stable {stable[:16]}…, reçu {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
