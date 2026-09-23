#!/usr/bin/env python3
"""Recount the weighted-pair shadow from its complete local stdout."""

import json
import sys


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


def closed(mask, count3, count4):
    return (not (mask & 2) or count3 >= 4) and (not (mask & 4) or count4 >= 3)


def summary(ids, rows, weighted, include_ordinals=False):
    old = [i for i in ids if closed(rows[i][5], weighted[i][7], weighted[i][8])]
    new = [i for i in ids if closed(rows[i][5], weighted[i][9], weighted[i][10])]
    gained = sorted(set(new) - set(old))
    check(set(old) <= set(new), "weighted matching loses a rectangle")
    result = {
        "rectangles": len(ids),
        "old_closed": len(old),
        "weighted_closed": len(new),
        "gained": len(gained),
        "old_closed_s2_edges": sum(weighted[i][1] for i in old),
        "weighted_closed_s2_edges": sum(weighted[i][1] for i in new),
        "gained_s2_edges": sum(weighted[i][1] for i in gained),
        "old_closed_F": sum(weighted[i][2] for i in old),
        "weighted_closed_F": sum(weighted[i][2] for i in new),
        "gained_F": sum(weighted[i][2] for i in gained),
        "extra_pair_q3": sum(weighted[i][14] for i in ids),
        "extra_pair_q4": sum(weighted[i][15] for i in ids),
        "ratio_tests_at_representative": sum(weighted[i][11] for i in ids),
        "positive_representative_tests": sum(weighted[i][12] for i in ids),
        "additional_corner_tests": sum(weighted[i][13] for i in ids),
        "weighted_phase_ns": sum(weighted[i][16] for i in ids),
    }
    if include_ordinals:
        result["gained_positive_ordinals"] = [
            {"ordinal": i, "s2_edges": weighted[i][1], "F": weighted[i][2]}
            for i in gained if weighted[i][1] > 0
        ]
    return result


def without_timing(value):
    if isinstance(value, dict):
        return {k: without_timing(v) for k, v in value.items()
                if not k.endswith("_ns")}
    if isinstance(value, list):
        return [without_timing(v) for v in value]
    return value


def main(path, expected_path=None):
    rows = {}
    weighted = {}
    meta = None
    with open(path, encoding="ascii") as src:
        for line in src:
            parts = line.split()
            if not parts:
                continue
            if parts[0] == "META":
                check(meta is None, "duplicate META")
                meta = [int(x) for x in parts[1:]]
            elif parts[0] == "ROW":
                r = [int(x) for x in parts[1:]]
                check(len(r) == 35 and r[1] not in rows, "bad/duplicate ROW")
                rows[r[1]] = r
            elif parts[0] == "WROW":
                w = [int(x) for x in parts[1:]]
                check(len(w) == 17 and w[0] not in weighted, "bad/duplicate WROW")
                weighted[w[0]] = w
            elif parts[0] not in ("STRATUM", "CERTBOX", "CERTPAIR", "CERTEND"):
                raise RuntimeError("unknown record")
    check(meta is not None and meta[:9] == [123389, 246777, 6175011,
          238364135, 2548453, 22034426, 503488729, 3986433,
          559661741], "front/S2 baseline mismatch")
    check(len(rows) == len(weighted) == 1771 and rows.keys() == weighted.keys(),
          "missing rows")
    for i, r in rows.items():
        w = weighted[i]
        check((r[1], r[6], r[7], r[-2], r[-1]) ==
              (w[0], w[1], w[2], w[7], w[8]), "paired row mismatch")
        check(w[9] >= w[7] and w[10] >= w[8], "weighted graph lost edges")
    big = [i for i, r in rows.items() if r[0] >= 3]
    positive = [i for i in big if rows[i][6] > 0]
    failures = [i for i in big if not closed(rows[i][5], weighted[i][7], weighted[i][8])]
    result = {
        "scope": "08/000000 raw whole frame, 1mm/u18, K5/s8, B palette",
        "global_s2_edges": meta[7], "global_F": meta[8],
        "all_sampled": summary(list(rows), rows, weighted),
        "big": summary(big, rows, weighted),
        "positive_big": summary(positive, rows, weighted, include_ordinals=True),
        "conditional_after_equal_pair_failure": summary(failures, rows, weighted),
    }
    check(len(big) == 1747 and len(positive) == 299 and len(failures) == 696,
          "rectangle strata mismatch")
    check(result["positive_big"]["old_closed"] == 72 and
          result["positive_big"]["old_closed_F"] == 5059809,
          "published B baseline mismatch")
    check(result["positive_big"]["weighted_closed"] == 87 and
          result["positive_big"]["gained_F"] == 5633688,
          "weighted panel mismatch")
    if expected_path:
        with open(expected_path, encoding="utf-8") as src:
            expected = json.load(src)
        check(without_timing(result) == without_timing(expected),
              "expected non-timing results mismatch")
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    check(len(sys.argv) in (2, 3), "usage: analyze.py probe.stdout [expected.json]")
    main(sys.argv[1], sys.argv[2] if len(sys.argv) == 3 else None)
