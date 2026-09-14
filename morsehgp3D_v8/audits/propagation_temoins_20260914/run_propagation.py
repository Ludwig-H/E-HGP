#!/usr/bin/env python3
"""Reçu du prototype de propagation des témoins (auditeur B, 14 septembre 2026).

Compile la copie instrumentée du front v8 contre la bibliothèque produit
indiquée, exécute la matrice (familles × tailles × modes) et les vérifications
par force brute, puis écrit PROPAGATION_CHECKS.json : pins SHA-256 (sources
constructeur d'origine, copie, bibliothèque), diff copie/source, commandes,
sorties brutes, champs analysés. Aucune assertion Python ; identique sous -O.
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
V8 = ROOT / "morsehgp3D_v8"
BOOST = ROOT / "build/v7_boost_gate/extracted/usr/include"
FIELD_RE = re.compile(r"(\w+)=([-\w.]+)")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fail(message: str) -> int:
    print(f"ECHEC : {message}", file=sys.stderr)
    return 1


def run(cmd: list[str], cwd: Path) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True)


def parse(stdout: str) -> dict | None:
    lines = stdout.strip().splitlines()
    if not lines or not lines[0].startswith("mode="):
        return None
    out = {}
    for line in lines:
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
    parser.add_argument("--lib", required=True, help="libmhgp8_p0.a du build produit (da366f7f)")
    parser.add_argument("--src-root", default=str(V8),
                        help="racine morsehgp3D_v8 des en-têtes à compiler : de préférence une extraction "
                             "`git archive da366f7f morsehgp3D_v8/src morsehgp3D_v8/bench` (le worktree partagé bouge)")
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--output", default=str(HERE / "PROPAGATION_CHECKS.json"))
    parser.add_argument("--sizes", default="8000,16000,32000")
    parser.add_argument("--skip-verify", action="store_true")
    args = parser.parse_args()
    lib = Path(args.lib).resolve()
    src_root = Path(args.src_root).resolve()
    build = Path(args.build_dir).resolve()
    build.mkdir(parents=True, exist_ok=True)
    origin = src_root / "src/wspd/front.cpp"
    for path in (lib, origin, src_root / "src/wspd/front.hpp", HERE / "lens_front.cpp", HERE / "main.cpp"):
        if not path.is_file():
            return fail(f"fichier absent : {path}")
    pins = {str(p.relative_to(ROOT)) if str(p).startswith(str(ROOT)) else str(p): sha256(p)
            for p in (origin, src_root / "src/wspd/front.hpp", HERE / "lens_front.cpp", HERE / "main.cpp", lib)}
    diff = run(["git", "diff", "--no-index", "--", str(origin), str(HERE / "lens_front.cpp")], ROOT)
    head = run(["git", "rev-parse", "HEAD"], ROOT).stdout.strip()
    binary = build / "propagation_probe"
    compile_cmd = ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", f"-I{src_root / 'src'}", f"-I{src_root / 'bench'}",
                   "-isystem", str(BOOST), str(HERE / "main.cpp"), str(HERE / "lens_front.cpp"), str(lib), "-o", str(binary)]
    compiled = run(compile_cmd, HERE)
    if compiled.returncode != 0:
        return fail("compilation refusée :\n" + compiled.stderr)
    sizes = [int(x) for x in args.sizes.split(",")]
    matrix = [(fam, n, 10, 8, 3, mode) for fam in ("uniform", "terrain", "clusters", "rows")
              for n in sizes for mode in ("ref", "copy", "propagate")]
    verify = [] if args.skip_verify else (
        [(fam, 600, 10, 8, 3, mode) for fam in ("uniform", "terrain", "clusters", "rows") for mode in ("verify_ref", "verify_propagate")]
        + [(fam, 900, k, 8, 7, mode) for fam in ("uniform", "clusters") for k in (1, 2, 3, 5) for mode in ("verify_ref", "verify_propagate")])
    runs = []
    for fam, n, kmax, s, seed, mode in verify + matrix:
        cmd = [str(binary), str(n), fam, str(kmax), str(s), str(seed), mode]
        started = time.perf_counter()
        done = run(cmd, HERE)
        elapsed = time.perf_counter() - started
        parsed = parse(done.stdout) if done.returncode == 0 else None
        if parsed is None:
            return fail(f"sortie inattendue pour {' '.join(cmd)} :\n{done.stdout}\n{done.stderr}")
        if parsed.get("family") != fam or parsed.get("n") != n or parsed.get("mode") != mode:
            return fail(f"sortie incohérente pour {' '.join(cmd)}")
        if mode.startswith("verify") and parsed.get("UNSOUND") != 0:
            return fail(f"rejet non sûr détecté : {' '.join(cmd)}\n{done.stdout}")
        parsed["command"] = " ".join(cmd)
        parsed["wall_seconds"] = round(elapsed, 3)
        parsed["raw_stdout"] = done.stdout
        runs.append(parsed)
    # comparaisons ref/copy/propagate par (famille, n)
    comparisons = []
    by = {(r["family"], r["n"], r["mode"]): r for r in runs if r["mode"] in ("ref", "copy", "propagate")}
    for fam in ("uniform", "terrain", "clusters", "rows"):
        for n in sizes:
            ref, copy, prop = by.get((fam, n, "ref")), by.get((fam, n, "copy")), by.get((fam, n, "propagate"))
            if not (ref and copy and prop):
                continue
            if any(ref[k] != copy[k] for k in ("visits", "searches", "rejected_full", "emitted", "steps", "credits", "residual_q2", "residual_q3", "residual_q4")):
                return fail(f"la copie sans propagation ne reproduit pas la bibliothèque sur {fam} {n}")
            comparisons.append({"family": fam, "n": n,
                                "residual_ratio_q2": round(prop["residual_q2"] / max(1, ref["residual_q2"]), 4),
                                "residual_ratio_q3": round(prop["residual_q3"] / max(1, ref["residual_q3"]), 4),
                                "residual_ratio_q4": round(prop["residual_q4"] / max(1, ref["residual_q4"]), 4),
                                "emitted_ratio": round(prop["emitted"] / max(1, ref["emitted"]), 4),
                                "visits_ratio": round(prop["visits"] / max(1, ref["visits"]), 4),
                                "steps_ratio": round(prop["steps"] / max(1, ref["steps"]), 4),
                                "credits_ratio": round(prop["credits"] / max(1, ref["credits"]), 4),
                                "front_ms": {"ref": ref["front_ms"], "copy": copy["front_ms"], "propagate": prop["front_ms"]}})
    receipt = {
        "title": "Propagation des témoins certifiés parent→enfants sur le premier front v8 : prototype d'audit",
        "date": "2026-09-14", "author_role": "auditeur indépendant B", "git_head": head,
        "compile_command": " ".join(compile_cmd), "src_root": str(src_root), "pins_sha256": pins,
        "copy_vs_origin_diff": diff.stdout, "runs": runs, "comparisons": comparisons,
        "stable_digest_without_times": hashlib.sha256(json.dumps([{k: v for k, v in r.items() if k not in ("wall_seconds", "front_ms", "raw_stdout")} for r in runs], sort_keys=True).encode()).hexdigest(),
        "scope": "Prototype hors produit sur une copie instrumentée de front.cpp ; les compteurs sont déterministes, les temps indicatifs (machine partagée) et la copie porte un descripteur de tâche plus lourd que l'original (comparer copy et propagate pour l'effet de la propagation seule).",
    }
    Path(args.output).write_text(json.dumps(receipt, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"OK : {len(runs)} exécutions, {len(comparisons)} comparaisons, reçu {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
