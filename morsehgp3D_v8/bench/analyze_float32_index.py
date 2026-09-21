#!/usr/bin/env python3
"""Report all predeclared construction growth pairs; no query/FULL scaling claim."""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

import run_float32_index_checks as qualification


def analyze(path):
    checked = qualification.read(path)
    values = checked["measurements"]
    relationships = []
    for regime in ("uniform", "terrain", "clusters"):
        for smaller, larger in ((8000,16000), (16000,32000)):
            relationships.append((f"synthetic_{regime}_{larger}", f"synthetic_{regime}_{smaller}"))
    relationships.extend(("scene_0_full", "scene_0_half_x_"+side) for side in ("neg", "nonneg"))
    relationships.extend(("scene_0_half_x_"+x, "scene_0_quarter_x_"+x+"_y_"+y)
                         for x in ("neg", "nonneg") for y in ("neg", "nonneg"))
    qualification.require(all(parent in values and child in values for parent,child in relationships),
                          "growth needs all synthetic sizes and all seven scene0 pieces")
    pairs = []
    for parent, child in relationships:
        large, small = values[parent], values[child]
        ratio_n = large["n"] / small["n"]
        qualification.require(ratio_n > 1, "non-growing input relation")
        costs = {}
        for field in ("build_ms_median", "retained_bytes", "construction_peak_vector_bytes"):
            costs[field] = (large[field], small[field])
        for field in large["work"]:
            costs["work."+field] = (large["work"][field], small["work"][field])
        metrics = {}
        for key, (numerator, denominator) in costs.items():
            if numerator <= 0 or denominator <= 0:
                metrics[key] = dict(parent=numerator, child=denominator, ratio=None, exponent=None,
                                    below_quadratic=None, reason="nonpositive_work")
                continue
            ratio = numerator / denominator
            metrics[key] = dict(parent=numerator, child=denominator, ratio=ratio,
                                exponent=math.log(ratio)/math.log(ratio_n), below_quadratic=ratio < ratio_n**2)
        pairs.append(dict(parent=parent, child=child, n_parent=large["n"], n_child=small["n"],
                          n_ratio=ratio_n, quadratic_ratio=ratio_n**2, metrics=metrics))
    return dict(schema="mhgp8_float32_index_growth_v1", status="passed", capture=checked["path"],
                completion_sha256=checked["completion_sha256"], analyzer_sha256=qualification.sha(Path(__file__)),
                reader_sha256=qualification.sha(Path(qualification.__file__)), pairs=pairs, measurements=values,
                scope="construction_only_not_query_scaling_WSPD_census_or_FULL",
                qualifications=dict(cpu_only=True, gcp_used=False, repetitions=3, public_status="not_claimed",
                                    asymptotic_bound="O(n log n) construction and O(n) storage by separate proof",
                                    timing="local shared host, no dedicated machine or speedup claim"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--path", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(analyze(args.path), sort_keys=True))


if __name__ == "__main__":
    main()
