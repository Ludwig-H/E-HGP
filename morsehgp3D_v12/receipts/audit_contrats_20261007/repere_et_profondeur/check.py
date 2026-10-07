#!/usr/bin/env python3
"""Témoins synthétiques de contrats, pas un juge FULL v12 ni un banc de temps."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import subprocess


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def frame(points):
    need(bool(points), "repère vide")
    low = tuple(min(p[j] for p in points) for j in range(3))
    high = tuple(max(p[j] for p in points) for j in range(3))
    width = max(high[j] - low[j] for j in range(3))
    return low, high, width.bit_length()


def shell(base, scale, offset):
    return sorted({tuple(offset + sign * x * scale for sign, x in zip(signs, p))
                   for p in set(itertools.permutations(base))
                   for signs in itertools.product((-1, 1), repeat=3)})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path, help="mhgp11_catalogue_probe Release u21")
    parser.add_argument("--capture", type=Path, help="nouveau répertoire de sorties brutes synthétiques")
    args = parser.parse_args()
    result = {"schema": "mhgp12.audit.contract_witnesses.v1", "public_status": "not_claimed",
              "v12_native_tested": False, "frames": [], "depth": []}
    for bits in (21, 24, 32):
        sites = [(0, 0, 0), ((1 << bits) - 1, 0, 0)]
        # La boîte T0 de boxes.cpp utilise max(site)+1, y compris à l'extrémité du domaine.
        corners = list(itertools.product((0, 1 << bits), (0, 1), (0, 1)))
        _, _, site_bits = frame(sites)
        _, _, closure_bits = frame(sites + corners)
        need(site_bits == bits and closure_bits == bits + 1, "borne supérieure de boîte")
        need(all(0 <= x < (1 << bits) for p in sites for x in p), "domaine sites")
        result["frames"].append({"input_bits": bits, "site_frame_bits": site_bits,
                                 "closed_box_frame_bits": closure_bits,
                                 "upper_endpoint": 1 << bits})
    need(frame([(7, 8, 9)])[2] == 0, "étendue nulle")
    # Une liste K-certifiée n'est pas contenue dans sa boîte de centres.
    corners = list(itertools.product((0, 1), repeat=3))
    need(frame(corners)[2] == 1 and frame(corners + [(1 << 20, 0, 0)])[2] == 21,
         "le site extérieur doit participer au certificat")
    result["external_site"] = {"box_bits": 1, "union_bits": 21}
    # s+2 suffit pour chaque paire (support, un site gardé), pas pour l'union de tout le pavé.
    support = [(0, 0, 0), (3, 0, 0)]
    guarded = [(-7, 0, 0), (11, 0, 0)]  # -2M < x < 3M, M=4.
    need(all(frame(support + [x])[2] <= 4 for x in guarded), "garde par site")
    need(frame(support + guarded)[2] == 5, "union de toute la garde")
    result["guard_union"] = {"support_bits": 2, "per_query_bits_max": 4,
                              "union_bits": 5, "points": support + guarded}
    cases = [("shell24_k2_leaf16", shell((1, 2, 2), 1 << 18, 1 << 19), 2, 16, 60),
             ("shell48_k5_leaf24", shell((1, 2, 3), 1 << 18, 3 << 18), 5, 24, 63)]
    if args.capture:
        need(args.native is not None, "capture sans natif")
        args.capture.mkdir(parents=True, exist_ok=False)
    if args.native:
        binary = args.native.resolve()
        result["native_sha256"] = hashlib.sha256(binary.read_bytes()).hexdigest()
        for name, points, k, leaf, depth in cases:
            need(all(0 <= x < (1 << 21) for p in points for x in p), "fixture u21")
            request = f"{k} {leaf} 256 50000 4294967295 134217728 {len(points)}\n"
            request += "".join(f"{x} {y} {z} {i}\n" for i, (x, y, z) in enumerate(points))
            run = subprocess.run([str(binary)], input=request, capture_output=True, text=True, timeout=20)
            need(run.returncode == 0 and not run.stderr, "appel natif")
            parsed = json.loads(run.stdout)
            need(parsed["status"] == "ok" and parsed["coord_bits"] == 21, "refus/profil natif")
            need(parsed["ledger"]["max_depth"] == depth, "profondeur attendue")
            need(depth > 38, "témoin de la borne 38")
            need(parsed["used_before"] == parsed["used_after"] == 0, "libération")
            if args.capture:
                (args.capture / (name + ".input.txt")).write_text(request)
                (args.capture / (name + ".output.json")).write_text(run.stdout)
            result["depth"].append({"name": name, "sites": len(points), "k": k, "leaf": leaf,
                                     "request_sha256": hashlib.sha256(request.encode()).hexdigest(),
                                     "ledger": parsed["ledger"], "peak": parsed["peak"]})
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
