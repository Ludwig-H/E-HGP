"""Small independent rational oracle for the captured new-date ordering helper.

No geometric FULL execution or selection. All generated square roots are
rational: expected orders use Fraction alone, not em_dates or vc_certif.
"""
from bisect import bisect_right
from collections import Counter
from fractions import Fraction as F
import hashlib
import importlib.util
import json
from pathlib import Path
import random
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    obj = importlib.util.module_from_spec(spec)
    sys.modules[name] = obj
    spec.loader.exec_module(obj)
    return obj


provenance = json.loads((HERE / "provenance.json").read_text())
for item in provenance["files"]:
    raw = (HERE / "source" / item["file"]).read_bytes()
    require(hashlib.sha256(raw).hexdigest() == item["sha256"], "source hash")
CF = module("vc_certif", HERE / "source" / "vc_certif.py")
ED = module("em_dates", HERE / "source" / "em_dates.py")
import numpy as np


def inputs():
    # Positive levels within the magnitude of u18 radii, including arbitrarily
    # close rational dates. These are scalar helper inputs, not realized MEBs.
    for base in (F(1), F(1000), F(131071)):
        for exponent in (0, 20, 40, 48, 55):
            step = F(1, 2**exponent)
            radii = [base + step * k for k in range(12)]
            dates = []
            for k in (0, 2, 5, 9):
                for theta in (F(1, 16), F(1, 3), F(1, 2), F(15, 16)):
                    value = (1 - theta) * radii[k] + theta * radii[k + 2]
                    dates.append(("pq", k, k + 2, theta, value, 0))
            for k in (0, 1, 3, 7, 11):
                for bias in (-1, 0, 1):
                    dates.append(("free", 0, 0, F(0), radii[k], bias))
            # Distinct declarations of equal dates must share their rank.
            dates.append(("free", 0, 0, F(0), dates[0][4], 0))
            yield radii, dates


def run(radii, descriptors, order, mutant=None, force=False):
    levels = [x*x for x in radii]
    ED.MUTANTS.clear()
    ED.FORCE.clear()
    if mutant:
        ED.MUTANTS.add(mutant)
    if force:
        ED.FORCE.add("tout_exact")
    nv = ED.Nouvelles([float(x) for x in levels], levels.__getitem__)
    ids, values = [], []
    for index in order:
        kind, a, d, q, value, bias = descriptors[index]
        if kind == "pq":
            ident = nv.pq(1-q, a, q, d)
        else:
            # Bound 1e-9 deliberately exercises the free-date interval filter.
            mf = float(value) * (1.0 + bias * 0.5e-9)
            ident = nv.libre(mf, 1e-9, lambda value=value: CF.Racines.rat(value))
        ids.append(ident)
        values.append(value)
    keys = nv.cles()
    expected_by_gap = {}
    for value in values:
        gap = bisect_right(radii, value)-1
        if value != radii[gap]:
            expected_by_gap.setdefault(gap, set()).add(value)
    expected_by_gap = {k: sorted(v) for k, v in expected_by_gap.items()}
    expected = []
    for value in values:
        gap = bisect_right(radii, value)-1
        sub = 0 if value == radii[gap] else expected_by_gap[gap].index(value)+1
        expected.append((gap, sub))
    actual = [keys[i] for i in ids]
    discrepancies = sum(a != b for a, b in zip(actual, expected))
    if not mutant:
        require(actual == expected, "new-date key differs from rational oracle")
        rk_map, new, count, inverse = ED.raffiner(len(radii), keys)
        merged = sorted(set(radii) | set(values))
        require(count == len(merged), "refined cardinality")
        require(rk_map.tolist() == [merged.index(x) for x in radii], "base levels moved")
        require([new[i] for i in ids] == [merged.index(x) for x in values], "refined date rank")
        for k, value in enumerate(merged):
            require(int(inverse[k]) == (radii.index(value) if value in radii else -1), "inverse map")
    return discrepancies, dict(nv.compte), len(set(float(x) for x in levels)) < len(levels)


counts = Counter()
mutants = Counter()
tables = 0
for case, (radii, descriptors) in enumerate(inputs()):
    n = len(descriptors)
    shuffled = list(range(n))
    random.Random(73411+case).shuffle(shuffled)
    orders = [list(range(n)), list(reversed(range(n))), shuffled]
    for order in orders:
        for force in (False, True):
            _, counters, colliding = run(radii, descriptors, order, force=force)
            counts.update(counters)
            counts["declaration_checks"] += n
            counts["refinement_checks"] += 3 + len(set(radii) | {d[4] for d in descriptors})
            counts["runs"] += 1
            counts["runs_with_float_level_collision"] += int(colliding)
    tables += 1
    for name in ED.MUTANTS_CONNUS:
        mismatches, _, _ = run(radii, descriptors, orders[0], mutant=name)
        mutants[name] += mismatches
ED.MUTANTS.clear()
ED.FORCE.clear()
require(all(mutants[name] > 0 for name in ED.MUTANTS_CONNUS), "numeric mutant survived")
print(json.dumps({"status": "pass", "scope": "scalar captured helper; rational oracle; no native FULL",
                  "tables": tables, "counts": dict(sorted(counts.items())),
                  "mutant_wrong_keys": dict(sorted(mutants.items())),
                  "numpy": np.__version__}, sort_keys=True, indent=2))
