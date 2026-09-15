#!/usr/bin/env python3
"""Reçu du bilan net de la « surproposition » (auditeur B, 15 sept. 2026).

Lance ``surproposition_probe`` compilé contre la copie d'audit patchée du moteur
2741d614 pour cinq variantes par configuration : référence L = K, puis L = 2K et
L = 4K avec la politique « petits facteurs » (B0 = 16) et « tous produits » (B0 = 0),
chacune répétée (``--repeats``, temps minimum retenu), puis le même probe compilé
sans patch contre la bibliothèque non patchée de 2741d614 (``--reference-binary``).

Contrôles, tous exécutés : la référence non patchée et la variante L = K ont les
mêmes compteurs de front, de census et de flux (supports, totaux, condensés) ; les
cinq variantes ont le même flux ; le front seul égale le front du pipeline
(front_match) ; extension nulle en référence, non nulle pour L > 1 (plancher de
non-vacuité : produits et propositions étendus strictement positifs, rejets de
l'extension positifs sauf sur le régime de surcoût pur gravé où ils doivent être
nuls, tous produits >= petits facteurs) ; identités de propositions
(propositions = recherches x min(K, n) + propositions étendues,
propositions étendues <= produits étendus x (facteur - 1) x K) ; monotonie
(rectangles émis et candidats <= référence, rejets du census diminués d'autant) ;
supports = paires admises. Les temps sont rapportés, jamais contrôlés. Les hash
des trois sources patchées sont calculés depuis ``--patched-src``. Aucun ``assert``,
rejoué sous ``python3 -O``.
"""
import argparse
import hashlib
import json
import os
import pathlib
import subprocess
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
VARIANTS = [("1", "0"), ("2", "16"), ("2", "0"), ("4", "16"), ("4", "0")]
PATCHED_FILES = ["src/wspd/front.cpp", "src/wspd/front.hpp", "src/parallel/work_reduction.hpp"]
CONFIGS_FULL = [
    (8000, "uniform", 10, 8), (8000, "uniform", 5, 8),
    (8000, "terrain", 10, 8), (8000, "terrain", 5, 8),
    (8000, "clusters", 10, 8), (8000, "clusters", 5, 8),
    (8000, "rows", 10, 8), (8000, "rows", 5, 8),
    (16000, "uniform", 10, 8), (16000, "terrain", 10, 8), (16000, "clusters", 10, 8), (16000, "rows", 10, 8),
    (32000, "uniform", 10, 8), (32000, "terrain", 10, 8), (32000, "clusters", 10, 8), (32000, "rows", 10, 8),
]
CONFIGS_QUICK = [(2000, "uniform", 10, 8), (2000, "terrain", 5, 8)]
# Régime de surcoût pur attendu : le reçu plafond (plafond_proposeur_20260915) ne trouve aucun rectangle émis
# à K = 5 témoins universels sur les rangées 8k ; l'extension y propose sans jamais rejeter. Le contrôle exige
# alors zéro rejet, et un rejet strictement positif partout ailleurs.
ZERO_REJECTION_EXPECTED = {(8000, "rows", 5)}
FLUX_KEYS = ("supports", "interior_total", "shell_total", "digest_sum", "digest_xor", "accepted")
FRONT_KEYS = ("front_searches", "front_descent_steps", "front_proposals", "front_in_factors", "front_h_tests",
              "front_lane_credits", "front_rejected_products", "front_emitted", "front_residual_mass")
CENSUS_KEYS = ("census_input_rectangles", "anchor_queries", "candidates", "rejected", "count_node_visits",
               "count_bound_tests", "count_point_tests", "cursor_advances", "witness_splits", "payload_node_visits")
TIME_KEYS = ("total_ms", "front_only_ms", "count_ms", "payload_ms", "pool_preparation_ms", "pool_selected_total_ms")


def sha256_of(path):
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def parse_line(line):
    entry = {}
    for token in line.split():
        key, value = token.split("=", 1)
        if key in ("family", "build", "l_factor", "b0") or key.startswith("digest"):
            entry[key] = value
        elif "." in value:
            entry[key] = float(value)
        else:
            entry[key] = int(value)
    return entry


def run_probe(binary, n, family, kmax, s, seed, pool, env, repeats):
    """Répète le probe ; les compteurs doivent être identiques d'une répétition à l'autre, les temps prennent le minimum."""
    command = [binary, str(n), family, str(kmax), str(s), str(seed), str(pool)]
    merged = None
    for _ in range(repeats):
        completed = subprocess.run(command, capture_output=True, text=True, check=False, env=env)
        if completed.returncode != 0:
            return {"status": "error", "returncode": completed.returncode, "stderr": completed.stderr[-2000:],
                    "stdout": completed.stdout[-2000:]}
        entry = parse_line(completed.stdout.strip().splitlines()[-1])
        if merged is None:
            merged = entry
            merged["repeats"] = 1
            continue
        for key, value in entry.items():
            if key in TIME_KEYS:
                merged[key] = min(merged[key], value)
            elif merged.get(key) != value:
                merged.setdefault("nondeterministic", []).append(key)
        merged["repeats"] += 1
    return merged


def check_config(variants, reference):
    failures = []
    ref = variants[0]
    kmax = ref["kmax"]
    n = ref["n"]
    for v in variants:
        tag = f"L={v['l_factor']} B0={v['b0']}"
        if v.get("nondeterministic"):
            failures.append(f"nondeterministic counters for {tag}: {sorted(set(v['nondeterministic']))}")
        if v["front_match"] != 1:
            failures.append(f"front alone differs from the pipeline front for {tag}")
        for key in FLUX_KEYS:
            if v[key] != ref[key]:
                failures.append(f"{key} differs for {tag}")
        if v["supports"] != v["accepted"]:
            failures.append(f"supports differ from accepted pairs for {tag}")
        historical = v["front_searches"] * min(kmax, n)
        if v["front_proposals"] != historical + v["extended_proposals"]:
            failures.append(f"proposal identity broken for {tag}")
        factor = int(v["l_factor"])
        if v["extended_proposals"] > v["extended_products"] * (factor - 1) * kmax:
            failures.append(f"extended proposals exceed their bound for {tag}")
        if v["front_emitted"] > ref["front_emitted"] or v["candidates"] > ref["candidates"]:
            failures.append(f"emitted rectangles or candidates exceed the reference for {tag}")
        if ref["rejected"] - v["rejected"] != ref["candidates"] - v["candidates"]:
            failures.append(f"rejected pairs do not follow the candidates for {tag}")
        if factor == 1:
            if v["extended_products"] or v["extended_proposals"] or v["extended_rejections"]:
                failures.append("reference variant shows extension activity")
        else:
            if v["extended_products"] <= 0 or v["extended_proposals"] <= 0:
                failures.append(f"extension inactive (vacuous) for {tag}")
            expected_zero = (n, v["family"], kmax) in ZERO_REJECTION_EXPECTED
            if expected_zero and v["extended_rejections"] != 0:
                failures.append(f"extension rejects on the pure-overhead regime for {tag}")
            if not expected_zero and v["extended_rejections"] <= 0:
                failures.append(f"extension never rejects (vacuous) for {tag}")
    by_key = {(v["l_factor"], v["b0"]): v for v in variants}
    for factor in ("2", "4"):
        if by_key[(factor, "0")]["extended_products"] < by_key[(factor, "16")]["extended_products"]:
            failures.append(f"all-products policy extends fewer products than small-factors at L={factor}K")
    if reference is not None:
        if reference.get("status") == "error":
            failures.append("unpatched reference probe failed")
        else:
            for key in FLUX_KEYS + FRONT_KEYS + CENSUS_KEYS:
                if reference[key] != ref[key]:
                    failures.append(f"unpatched engine differs from L = K on {key}")
    return failures


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", required=True, help="probe relié à la bibliothèque patchée")
    parser.add_argument("--reference-binary", default=None, help="probe compilé avec -DMHGP8_AUDIT_UNPATCHED et relié à 2741d614 non patché")
    parser.add_argument("--patched-src", required=True, help="racine morsehgp3D_v8 de la copie patchée (hash des sources)")
    parser.add_argument("--engine-commit", default="2741d614")
    parser.add_argument("--out", default=str(HERE / "SURPROPOSITION_CHECKS.json"))
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--seed", type=int, default=3)
    parser.add_argument("--pool", type=int, default=64)
    args = parser.parse_args()
    configs = CONFIGS_QUICK if args.quick else CONFIGS_FULL
    results = []
    status = "pass"
    for n, family, kmax, s in configs:
        variants = []
        for factor, b0 in VARIANTS:
            env = dict(os.environ, MHGP8_AUDIT_L_FACTOR=factor, MHGP8_AUDIT_B0=b0)
            entry = run_probe(args.binary, n, family, kmax, s, args.seed, args.pool, env, args.repeats)
            if entry.get("status") == "error":
                print("ERROR", n, family, kmax, factor, b0, entry.get("stderr", "")[-300:], file=sys.stderr)
            variants.append(entry)
        reference = None
        if args.reference_binary:
            env = {k: v for k, v in os.environ.items() if not k.startswith("MHGP8_AUDIT_")}
            reference = run_probe(args.reference_binary, n, family, kmax, s, args.seed, args.pool, env, 1)
        ok = all(v.get("status") != "error" for v in variants)
        failures = check_config(variants, reference) if ok else ["probe error"]
        if failures:
            status = "fail"
        summary = []
        for v in variants:
            if v.get("status") == "error":
                continue
            summary.append(f"L{v['l_factor']}/B{v['b0']}: total {v['total_ms']/1000:.2f}s front {v['front_only_ms']/1000:.2f}s"
                           f" cand {v['candidates']} Z {v['count_node_visits']}")
        print(f"{family} n={n} K={kmax} s={s}: " + " | ".join(summary) + (" FAIL " + "; ".join(failures) if failures else " pass"), flush=True)
        results.append({"n": n, "family": family, "kmax": kmax, "s": s, "variants": variants, "unpatched_reference": reference,
                        "failures": failures, "status": "pass" if not failures else "fail"})
    src = pathlib.Path(args.patched_src)
    receipt = {
        "title": "Bilan net de la surproposition de témoins (fenêtre L = K, 2K, 4K ; politiques B0 = 16 et tous produits)",
        "auditor": "B",
        "date": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "engine_commit": args.engine_commit,
        "patched_sources_sha256": {name: sha256_of(src / name) for name in PATCHED_FILES},
        "patch_sha256": sha256_of(HERE / "front_filter.patch"),
        "probe_sha256": sha256_of(HERE / "surproposition_probe.cpp"),
        "runner_sha256": sha256_of(__file__),
        "seed": args.seed,
        "pool_min_factor": args.pool,
        "repeats": args.repeats,
        "quick": args.quick,
        "unpatched_reference": bool(args.reference_binary),
        "census_settings": "SharedBlocks, Saturating, ComplementFirst, Individual, MidpointSamples, masque 1 pour le front seul",
        "status": status,
        "configs": results,
    }
    pathlib.Path(args.out).write_text(json.dumps(receipt, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    print("status", status, "->", args.out)
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    sys.exit(main())
