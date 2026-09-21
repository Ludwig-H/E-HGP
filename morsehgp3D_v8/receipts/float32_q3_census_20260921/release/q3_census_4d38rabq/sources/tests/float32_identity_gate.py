#!/usr/bin/env python3
"""Independent Fraction oracles for binary32 ball identity and seed events.

Canonical coefficients are obtained from the rational centre and radius, not
from native determinants. Event ordering uses that same centre and the oriented
normal of the seed. Neither test provides radius sorting or a full hierarchy.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
from fractions import Fraction
from functools import lru_cache
import hashlib
from itertools import permutations
import json
from math import gcd, lcm
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
UNIT = 1 << 149
SOURCES = (Path(__file__).resolve(), ROOT / "morsehgp3D_v8/tests/float32_key_probe.cpp",
           ROOT / "morsehgp3D_v8/tests/float32_q4_events_probe.cpp", *(
               ROOT / "morsehgp3D_v8/src/core" / name for name in
               ("fixed_signed.hpp", "float32_predicates.hpp", "float32_ball.hpp", "float32_ball.cpp",
                "float32_ball_key.hpp", "float32_ball_key.cpp", "float32_q4_events.hpp", "float32_q4_events.cpp")))
BALL_FIELDS = ("q3_preparations", "q4_preparations", "preparation_filter_attempts", "preparation_filter_accepts",
               "preparation_exact_fallbacks", "preparation_exact_evaluations", "accepted_supports", "rejected_supports",
               "power_queries", "power_filter_attempts", "power_filter_accepts", "power_exact_fallbacks",
               "power_exact_evaluations", "interval_additions", "interval_products", "exact_additions",
               "exact_products", "exact_point_decodes")
KEY_FIELDS = ("q2_requests", "q3_requests", "q4_requests", "from_support_requests", "rejected_supports",
              "keys_created", "canonical_gcd_calls", "canonical_divisions", "packed_words")
EVENT_FIELDS = ("preparations", "accepted_seeds", "rejected_seeds", "interval_preparations", "side_queries",
                "side_filter_attempts", "side_filter_accepts", "side_exact_fallbacks", "side_exact_evaluations",
                "root_queries", "root_coplanar_rejections", "root_filter_attempts", "root_filter_accepts",
                "root_exact_fallbacks", "root_exact_evaluations", "root_equalities", "interval_additions",
                "interval_products", "exact_additions", "exact_products", "exact_point_decodes")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def strict_json(data):
    def pairs(values):
        result = {}
        for key, value in values:
            require(key not in result, "duplicate JSON key")
            result[key] = value
        return result
    def invalid(value):
        raise ValueError("nonfinite JSON value: " + value)
    return json.loads(data, object_pairs_hook=pairs, parse_constant=invalid)


def bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def rational(word):
    require(type(word) is int and 0 <= word < 2**32 and word & 0x7f800000 != 0x7f800000,
            "nonfinite or invalid binary32 word")
    exponent, mantissa = (word >> 23) & 255, word & 0x7fffff
    if exponent:
        mantissa |= 1 << 23
    value = Fraction(mantissa)*Fraction(2)**(exponent-150 if exponent else -149)
    return -value if word & 0x80000000 else value


def xyz(point):
    return tuple(rational(w) for w in point)


def encoded(points):
    return tuple(tuple(bits(x) for x in p) for p in points)


def dot(a, b):
    return sum((x*y for x, y in zip(a, b, strict=True)), Fraction(0))


def subtract(a, b):
    return tuple(x-y for x, y in zip(a, b, strict=True))


def solve(matrix, rhs):
    rows = [list(row)+[value] for row, value in zip(matrix, rhs, strict=True)]
    n = len(rhs)
    for col in range(n):
        pivot = next((r for r in range(col, n) if rows[r][col]), None)
        if pivot is None:
            return None
        rows[col], rows[pivot] = rows[pivot], rows[col]
        scale = rows[col][col]
        rows[col] = [v/scale for v in rows[col]]
        for r in range(n):
            if r != col:
                factor = rows[r][col]
                rows[r] = [v-factor*w for v, w in zip(rows[r], rows[col], strict=True)]
    return tuple(row[-1] for row in rows)


@lru_cache(maxsize=None)
def circumball(support):
    points = tuple(map(xyz, support))
    origin = points[0]
    edges = tuple(subtract(point, origin) for point in points[1:])
    coefficient = solve([[dot(a, b) for b in edges] for a in edges], [dot(edge, edge)/2 for edge in edges])
    if coefficient is None or not all(v > 0 for v in (1-sum(coefficient), *coefficient)):
        return None
    centre = tuple(origin[d]+sum(t*edge[d] for t, edge in zip(coefficient, edges, strict=True)) for d in range(3))
    radius2 = dot(subtract(origin, centre), subtract(origin, centre))
    require(all(dot(subtract(p, centre), subtract(p, centre)) == radius2 for p in points), "oracle centre is not equidistant")
    return centre, radius2


def key_oracle(support):
    sphere = circumball(tuple(support))
    if sphere is None:
        return None
    centre, radius2 = sphere
    rational_coefficients = (Fraction(1), *(-2*UNIT*c for c in centre), UNIT*UNIT*(dot(centre, centre)-radius2))
    denominator = lcm(*(v.denominator for v in rational_coefficients))
    integers = tuple(int(v*denominator) for v in rational_coefficients)
    divisor = gcd(*integers)
    canonical = tuple(v//divisor for v in integers)
    require(canonical[0] > 0 and gcd(*canonical) == 1, "oracle coefficient normalization")
    return canonical


def event_oracle(seed, first, second):
    sphere = circumball(tuple(seed))
    if sphere is None:
        return dict(valid=False, sides=None, order=None, coplanar=False)
    centre, radius2 = sphere
    a, b, c = map(xyz, seed)
    u, v = subtract(b, a), subtract(c, a)
    normal = (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])
    sides, roots = [], []
    for words in (first, second):
        point = xyz(words)
        denominator = dot(normal, subtract(point, a))
        sides.append((denominator > 0)-(denominator < 0))
        power = dot(subtract(point, centre), subtract(point, centre))-radius2
        roots.append(power/denominator if denominator else None)
    coplanar = 0 in sides
    order = None if coplanar else (roots[0] > roots[1])-(roots[0] < roots[1])
    return dict(valid=True, sides=sides, order=order, coplanar=coplanar)


def key_fixtures():
    # A dyadic unit sphere in dimension three has only axial dyadic points.
    # Radius five instead provides strict q2/q3/q4 supports with integer words.
    common = (encoded(((-5, 0, 0), (5, 0, 0))),
              encoded(((0, -5, 0), (0, 5, 0))),
              encoded(((5, 0, 0), (-3, 4, 0), (-3, -4, 0))),
              encoded(((5, 0, 0), (-3, 0, 4), (-3, 0, -4))),
              encoded(((3, 0, 4), (3, 0, -4), (-3, 4, 0), (-3, -4, 0))))
    result = []
    def add(label, support, valid=None):
        support = tuple(tuple(p) for p in support)
        actual = key_oracle(support)
        require(valid is None or (actual is not None) == valid, "incorrect key fixture: " + label)
        result.append(dict(label=label, support=support))
    for support in common:
        for permutation in permutations(support):
            add("common_ball_cross_arity", permutation, True)
    for exponent in (-149, -120, -1, 0, 60, 120, 124):
        for support in (common[0], common[2], common[4]):
            transformed = tuple(tuple(bits(float(rational(w))*2.0**exponent) for w in p) for p in support)
            add(f"common_ball_scaled_{exponent}", transformed, True)
    for translation in ((16, -8, 4), (-32, 64, 0), (.5, .25, -.125)):
        for support in (common[0], common[2], common[4]):
            moved = tuple(tuple(bits(float(rational(w))+translation[d]) for d, w in enumerate(p)) for p in support)
            add("common_ball_translated", moved, True)
    for delta in (-1, 0, 1):
        # Adjacent representable centres, not a tolerance-sized perturbation.
        centre = rational(bits(24)+delta)
        add("adjacent_centre", ((bits(float(centre)-.5), 0, 0), (bits(float(centre)+.5), 0, 0)), True)
    for endpoint in (bits(5)-1, bits(5), bits(5)+1):
        add("adjacent_radius", ((endpoint | 0x80000000, 0, 0), (endpoint, 0, 0)), True)
    for mask in range(8):
        for support in (common[0], common[2]):
            signed = tuple(tuple(w if w else (0x80000000 if mask & (1 << d) else 0) for d, w in enumerate(p)) for p in support)
            add("signed_zero_identity", signed, True)
    invalid = (encoded(((0, 0, 0), (0, 0, 0))),
               ((0, 0, 0), (0x80000000, 0x80000000, 0x80000000)),
               encoded(((0, 0, 0), (1, 0, 0), (2, 0, 0))),
               encoded(((-1, 0, 0), (1, 0, 0), (0, 1, 0))),
               encoded(((-1, 0, 0), (1, 0, 0), (0, .5, 0))),
               encoded(((0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0))),
               encoded(((5, 0, 0), (-3, 4, 0), (-3, -4, 0), (0, 0, 5))))
    for support in invalid:
        add("invalid_support", support, False)
    huge, tiny = 0x7f7fffff, 1
    extreme3 = ((huge, 0, 0), (huge | 0x80000000, 0, 0), (0, huge, tiny))
    extreme4 = extreme3+((0, huge | 0x80000000, tiny),)
    for support in (((huge, 0, 0), (huge | 0x80000000, 0, 0)), ((0, 0, 0), (1, 0, 0)), extreme3, extreme4):
        for permutation in permutations(support):
            add("extreme_exponents", permutation, True)
    rng = random.Random(0xF32D3A1D)
    for arity in (2, 3, 4):
        for _ in range(24):
            support = tuple(tuple(bits(rng.randint(-11, 11)) for _ in range(3)) for _ in range(arity))
            add("random_integer", support)
        for _ in range(8):
            def finite():
                while True:
                    word = rng.getrandbits(32)
                    if word & 0x7f800000 != 0x7f800000:
                        return word
            add("random_finite_words", tuple(tuple(finite() for _ in range(3)) for _ in range(arity)))
    return result


def event_fixtures():
    result = []
    seed = encoded(((5, 0, 0), (-3, 4, 0), (-3, -4, 0)))
    queries = encoded(((0, 0, 5), (0, 0, -5), (3, 0, 4), (0, 3, 4), (0, 0, 1), (0, 0, -1),
                       (0, 0, 10), (0, 0, -2.5), (0, 0, 0), (5, 0, 0), (6, 0, 0)))
    def add(label, support, first, second):
        result.append(dict(label=label, seed=tuple(tuple(p) for p in support), first=tuple(first), second=tuple(second)))
    for permutation in permutations(seed):
        for first in queries:
            for second in queries:
                add("oriented_roots_and_coplanar", permutation, first, second)
    for delta in (-1, 0, 1):
        add("near_equal_event", seed, (0, 0, bits(5)+delta), (0, 0, bits(5)))
    for exponent in (-149, -120, 60, 120, 124):
        transformed = tuple(tuple(bits(float(rational(w))*2.0**exponent) for w in p) for p in seed)
        for a, b in ((queries[4], queries[5]), (queries[0], queries[1]), (queries[6], queries[7])):
            z1, z2 = (tuple(bits(float(rational(w))*2.0**exponent) for w in p) for p in (a, b))
            add("scaled_event", transformed, z1, z2)
    huge = 0x7f7fffff
    thin = ((huge, 0, 0), (huge | 0x80000000, 0, 0), (0, huge, 1))
    extreme_queries = ((huge, 1, 0), (huge, 0, 1), (0, huge, 0), (0, huge | 0x80000000, 0), (0, 0, 0))
    for permutation in permutations(thin):
        for a in extreme_queries:
            for b in extreme_queries:
                add("extreme_cancellation", permutation, a, b)
    for support in (encoded(((0, 0, 0), (1, 0, 0), (2, 0, 0))),
                    encoded(((-1, 0, 0), (1, 0, 0), (0, 1, 0))),
                    encoded(((-1, 0, 0), (1, 0, 0), (0, .5, 0)))):
        for permutation in permutations(support):
            add("invalid_seed", permutation, queries[0], queries[1])
    rng = random.Random(0xF32E71D)
    for _ in range(48):
        support = tuple(tuple(bits(rng.randint(-6, 6)) for _ in range(3)) for _ in range(3))
        first, second = (tuple(bits(rng.randint(-9, 9)) for _ in range(3)) for _ in range(2))
        add("random_integer", support, first, second)
    return result


def key_payload(cases):
    return "".join(str(len(c["support"]))+" "+" ".join(f"{w:08x}" for p in c["support"] for w in p)+"\n" for c in cases).encode()


def event_payload(cases):
    return "".join(" ".join(f"{w:08x}" for p in (*c["seed"], c["first"], c["second"]) for w in p)+"\n" for c in cases).encode()


def pack(coefficients):
    output = [1]
    for coefficient in coefficients:
        if not coefficient:
            output.append(0)
            continue
        magnitude = abs(coefficient)
        zero_bits = (magnitude & -magnitude).bit_length()-1
        odd = magnitude >> zero_bits
        limbs = []
        while odd:
            limbs.append(odd & 0xffffffff)
            odd >>= 32
        output.extend((2*len(limbs)+(coefficient < 0), zero_bits, *limbs))
    return output


def coefficients_hex(values):
    return [("-" if value < 0 else "")+format(abs(value), "x") for value in values]


def work_shape(work, fields, nested=None):
    require(type(work) is dict and set(work) == set(fields) | ({nested} if nested else set()), "work record shape")
    require(all(type(work[k]) is int and 0 <= work[k] < 2**64 for k in fields), "work counter type/range")
    if nested:
        work_shape(work[nested], BALL_FIELDS)


def key_work(work, arity, valid, words, emitted=False):
    work_shape(work, KEY_FIELDS, "support")
    require(all(work[f"q{q}_requests"] == int(q == arity and not emitted) for q in (2, 3, 4)) and
            work["from_support_requests"] == int(valid and (emitted or arity != 2)) and
            work["rejected_supports"] == int(not valid) and work["keys_created"] == int(valid), "key request/result ledger")
    require(work["canonical_gcd_calls"] == 4*int(valid) and work["canonical_divisions"] == 5*int(valid) and
            work["packed_words"] == (len(words) if valid else 0), "key canonicalization/word ledger")
    support = work["support"]
    require(all(support[k] == 0 for k in BALL_FIELDS if k.startswith("power_") or k.startswith("interval_") or
                k in ("preparation_filter_attempts", "preparation_filter_accepts", "preparation_exact_fallbacks")),
            "exact key repeated power queries or used floating filters")
    prepared = arity != 2 and not emitted
    require(support["q3_preparations"] == int(prepared and arity == 3) and
            support["q4_preparations"] == int(prepared and arity == 4) and
            support["preparation_exact_evaluations"] == int(prepared) and
            support["accepted_supports"] == int(prepared and valid) and
            support["rejected_supports"] == int(prepared and not valid), "key support validity repeated/omitted")
    decodes = arity if emitted or arity == 2 or not valid else 2*arity
    require(support["exact_point_decodes"] == decodes, "key exact coordinate decoding ledger")


def validate_keys(rows, cases):
    require(type(rows) is list and len(rows) == len(cases), "key result count")
    counts = {str(q): dict(valid=0, invalid=0) for q in (2, 3, 4)}
    groups, lengths = {}, []
    for row, case in zip(rows, cases, strict=True):
        arity = len(case["support"])
        expected = key_oracle(case["support"])
        valid = expected is not None
        require(type(row) is dict and set(row) == {"arity", "valid", "words", "coefficients", "work", "from_support_words", "from_support_work"},
                "key result shape")
        require(type(row["arity"]) is int and row["arity"] == arity and type(row["valid"]) is bool and row["valid"] == valid,
                "key support validity differs from Gram oracle")
        words = pack(expected) if valid else None
        if valid:
            require(type(row["words"]) is list and all(type(w) is int and 0 <= w < 2**32 for w in row["words"]) and
                    row["words"] == words and row["coefficients"] == coefficients_hex(expected),
                    "global primitive coefficients or unique word encoding differ from Fraction centre/radius")
            groups.setdefault(tuple(words), set()).add(arity)
            lengths.append(len(words))
        else:
            require(row["words"] is row["coefficients"] is None, "rejected support acquired a key")
        key_work(row["work"], arity, valid, words)
        if valid and arity != 2:
            require(row["from_support_words"] == words, "emission key differs from facade/global identity")
            key_work(row["from_support_work"], arity, True, words, emitted=True)
        else:
            require(row["from_support_words"] is row["from_support_work"] is None, "unsupported emission path executed")
        counts[str(arity)]["valid" if valid else "invalid"] += 1
    cross = sum(arities == {2, 3, 4} for arities in groups.values())
    require(all(v["valid"] and v["invalid"] for v in counts.values()) and cross > 0 and len(groups) > 1,
            "key cross-arity/rejection/distinct-ball coverage absent")
    return dict(cases=len(cases), populations=counts, distinct_balls=len(groups), cross_arity_balls=cross,
                serialized_words=dict(min=min(lengths), max=max(lengths), sum=sum(lengths)))


def seed_work(work, filtered, valid):
    work_shape(work, BALL_FIELDS)
    require(work["q3_preparations"] == 1 and work["q4_preparations"] == 0 and
            work["accepted_supports"] == int(valid) and work["rejected_supports"] == int(not valid) and
            all(work[k] == 0 for k in BALL_FIELDS if k.startswith("power_")), "event q3 eligibility ledger")
    if filtered:
        require(work["preparation_filter_attempts"] == 1 and
                work["preparation_filter_accepts"]+work["preparation_exact_fallbacks"] == 1 and
                work["preparation_exact_evaluations"] == work["preparation_exact_fallbacks"], "event seed filter partition")
    else:
        require(work["preparation_filter_attempts"] == work["preparation_filter_accepts"] == work["preparation_exact_fallbacks"] == 0 and
                work["preparation_exact_evaluations"] == 1 and work["interval_additions"] == work["interval_products"] == 0,
                "exact event seed used interval arithmetic")
    require(work["exact_point_decodes"] == 3*work["preparation_exact_evaluations"], "seed exact decoding ledger")


def event_work(work, stage, filtered, expected):
    work_shape(work, EVENT_FIELDS, "seed")
    valid, coplanar = expected["valid"], expected["coplanar"]
    if stage == "preparation":
        seed_work(work["seed"], filtered, valid)
        require(work["preparations"] == 1 and work["accepted_seeds"] == int(valid) and
                work["rejected_seeds"] == int(not valid) and work["interval_preparations"] == int(filtered and valid),
                "event preparation partition")
        require(all(work[k] == 0 for k in EVENT_FIELDS if k.startswith(("side_", "root_", "exact_"))),
                "event preparation charged a side/root/exact query")
        if filtered and valid:
            require(work["interval_additions"] > 0 and work["interval_products"] > 0, "missing interval preparation work")
        else:
            require(work["interval_additions"] == work["interval_products"] == 0, "unused interval preparation")
        return
    require(not any(work["seed"].values()), "event query repeated seed eligibility")
    if not valid:
        require(not any(work[k] for k in EVENT_FIELDS), "invalid seed performed event queries")
        return
    comparison = stage == "comparison"
    require(work["preparations"] == work["accepted_seeds"] == work["rejected_seeds"] == work["interval_preparations"] == 0 and
            work["side_queries"] == 2 and work["root_queries"] == int(comparison) and
            work["root_coplanar_rejections"] == int(comparison and coplanar), "event query call/coplanar ledger")
    root = int(comparison and not coplanar)
    if filtered:
        require(work["side_filter_attempts"] == 2 and work["side_filter_accepts"]+work["side_exact_fallbacks"] == 2 and
                work["side_exact_evaluations"] == work["side_exact_fallbacks"] and
                work["side_exact_fallbacks"] >= expected["sides"].count(0), "event side filter partition/contact")
        require(work["root_filter_attempts"] == root and work["root_filter_accepts"]+work["root_exact_fallbacks"] == root and
                work["root_exact_evaluations"] == work["root_exact_fallbacks"], "event root filter partition")
    else:
        require(work["side_filter_attempts"] == work["side_filter_accepts"] == work["side_exact_fallbacks"] == 0 and
                work["side_exact_evaluations"] == 2 and work["root_filter_attempts"] == work["root_filter_accepts"] ==
                work["root_exact_fallbacks"] == 0 and work["root_exact_evaluations"] == root and
                work["interval_additions"] == work["interval_products"] == 0, "exact event query used filters")
    require(work["root_equalities"] == int(bool(root) and expected["order"] == 0) and
            (not root or expected["order"] != 0 or work["root_exact_evaluations"] == 1), "event equality was not exact")
    require(work["exact_point_decodes"] == 4*work["side_exact_evaluations"]+5*work["root_exact_evaluations"],
            "event query exact decoding ledger")


def validate_events(rows, cases):
    require(type(rows) is list and len(rows) == len(cases), "event row count")
    names = {prefix+"_"+field for prefix in ("filtered", "exact") for field in
             ("valid", "sides", "order", "coplanar", "preparation_work", "side_work", "comparison_work")}
    counts = dict(valid=0, invalid=0, coplanar=0, orders={"-1": 0, "0": 0, "1": 0},
                  sides={"-1": 0, "0": 0, "1": 0}, side_filter_accepts=0, side_exact_fallbacks=0,
                  root_filter_accepts=0, root_exact_fallbacks=0)
    for row, case in zip(rows, cases, strict=True):
        expected = event_oracle(case["seed"], case["first"], case["second"])
        require(type(row) is dict and set(row) == names, "event row shape")
        for prefix in ("filtered", "exact"):
            require(type(row[prefix+"_valid"]) is type(row[prefix+"_coplanar"]) is bool and
                    row[prefix+"_valid"] == expected["valid"] and row[prefix+"_coplanar"] == expected["coplanar"],
                    "event seed validity/coplanarity differs from Fraction oracle")
            sides, order = row[prefix+"_sides"], row[prefix+"_order"]
            require((type(sides) is list and all(type(v) is int for v in sides) and sides == expected["sides"])
                    if expected["valid"] else sides is None, "event side orientation differs from rational normal")
            require(order is None if expected["order"] is None else type(order) is int and order == expected["order"],
                    "event order differs from rational power/normal ratio")
            for stage in ("preparation", "side", "comparison"):
                event_work(row[prefix+"_"+stage+"_work"], stage, prefix == "filtered", expected)
        counts["valid" if expected["valid"] else "invalid"] += 1
        if expected["valid"]:
            for side in expected["sides"]:
                counts["sides"][str(side)] += 1
        if expected["coplanar"]:
            counts["coplanar"] += 1
        if expected["order"] is not None:
            counts["orders"][str(expected["order"])] += 1
        for field in ("side_filter_accepts", "side_exact_fallbacks", "root_filter_accepts", "root_exact_fallbacks"):
            counts[field] += row["filtered_comparison_work"][field]
    require(counts["valid"] and counts["invalid"] and counts["coplanar"] and all(counts["orders"].values()) and
            all(counts["sides"].values()) and all(counts[k] > 0 for k in
                ("side_filter_accepts", "side_exact_fallbacks", "root_filter_accepts", "root_exact_fallbacks")), "event population/filter non-vacuity")
    return dict(cases=len(cases), **counts)


def invalid_inputs():
    key = key_payload([dict(support=encoded(((0, 0, 0), (1, 0, 0))))])
    event = event_payload([dict(seed=encoded(((0, 0, 0), (1, 0, 0), (0, 1, 0))), first=(0, 0, 0), second=(0, 0, 0))])
    keys = [("empty", b""), ("arity", key.replace(b"2 ", b"1 ", 1)), ("short", b"2 00000000\n"),
            ("long", key.rstrip()+b" 00000000\n"), ("hex", key.replace(b"00000000", b"xxxxxxxx", 1))]
    events = [("empty", b""), ("short", b"00000000\n"), ("long", event.rstrip()+b" 00000000\n"),
              ("hex", event.replace(b"00000000", b"xxxxxxxx", 1))]
    for arity in (2, 3, 4):
        for position in (0, arity-1):
            for axis in range(3):
                for word in (0x7f800000, 0xff800000, 0x7fc00001, 0x7f800001):
                    support = [[0, 0, 0] for _ in range(arity)]
                    support[position][axis] = word
                    keys.append((f"nonfinite_{arity}_{position}_{axis}_{word:08x}", key_payload([dict(support=support)])))
    for position in range(5):
        for axis in range(3):
            for word in (0x7f800000, 0xff800000, 0x7fc00001, 0x7f800001):
                points = [[0, 0, 0] for _ in range(5)]
                points[position][axis] = word
                events.append((f"nonfinite_{position}_{axis}_{word:08x}", event_payload([dict(seed=points[:3], first=points[3], second=points[4])])) )
    return keys, events


def execute(binary, output, name, data=None, arguments=(), expected_exit=0):
    if data is not None:
        (output / (name+".input")).write_bytes(data)
    record = dict(command=[str(binary), *arguments], exit_code=None)
    try:
        with (output / (name+".stdout")).open("xb") as stdout, (output / (name+".stderr")).open("xb") as stderr:
            child = subprocess.run(record["command"], input=data, stdout=stdout, stderr=stderr, check=False)
        record["exit_code"] = child.returncode
    except BaseException as error:
        record["error"] = f"{type(error).__name__}: {error}"
        raise
    finally:
        (output / (name+".json")).write_text(json.dumps(record, sort_keys=True)+"\n")
    require(record["exit_code"] == expected_exit, "identity native exit differs; streams retained: " + name)


def read_command(output, binary, name, data=None, arguments=(), expected_exit=0):
    if data is not None:
        require((output / (name+".input")).read_bytes() == data, "identity native input differs: " + name)
    record = strict_json((output / (name+".json")).read_bytes())
    require(type(record) is dict and set(record) == {"command", "exit_code"} and record["command"] == [str(binary), *arguments] and
            type(record["exit_code"]) is int and record["exit_code"] == expected_exit, "identity native command/exit differs")
    stdout, stderr = ((output / (name+"."+stream)).read_bytes() for stream in ("stdout", "stderr"))
    if expected_exit:
        prefix = b"float32 key probe: " if name.startswith("key_") else b"float32 q4 events probe: "
        require(not stdout and stderr.startswith(prefix), "identity rejection diagnostic missing")
    else:
        require(not stderr, "identity native success wrote stderr")
    return stdout


def check_selftests(key, event):
    require(type(key) is dict and set(key) == {"schema", "status", "tests", "rounding_modes", "flush_modes", "concurrent_workers",
            "concurrent_iterations", "key_object_bytes"} and key["schema"] == "mhgp8_float32_key_selftest_v1" and key["status"] == "passed" and
            all(type(v) is int for k, v in key.items() if k not in ("schema", "status")) and key["flush_modes"] in (1, 4) and
            key["tests"] == 20+8*key["flush_modes"] and key["rounding_modes"] == 4 and key["concurrent_workers"] == 4 and
            key["concurrent_iterations"] == 64 and key["key_object_bytes"] > 0, "key selftest inventory")
    require(type(event) is dict and set(event) == {"schema", "status", "tests", "queries", "equality_checks", "coplanar_rejections",
            "rejected_seeds", "invalid_modes", "counter_overflows", "owner_replacements", "rounding_modes", "flush_modes",
            "concurrent_workers", "concurrent_queries", "prepared_bytes"} and
            event["schema"] == "mhgp8_float32_q4_events_selftest_v1" and event["status"] == "passed" and
            all(type(v) is int for k, v in event.items() if k not in ("schema", "status")) and event["flush_modes"] in (1, 4) and
            event["tests"] == 81+152*event["flush_modes"] and event["queries"] == 72+124*event["flush_modes"] and
            event["equality_checks"] == 8+24*event["flush_modes"] and event["coplanar_rejections"] == 4 and
            event["rejected_seeds"] == 8 and event["invalid_modes"] == event["counter_overflows"] == event["owner_replacements"] == 1 and
            event["rounding_modes"] == 4 and event["concurrent_workers"] == 4 and event["concurrent_queries"] == 32 and
            event["prepared_bytes"] > 0, "event selftest inventory")


def read_transcript(output, key_binary, event_binary):
    key_cases, event_cases = key_fixtures(), event_fixtures()
    key_bad, event_bad = invalid_inputs()
    names = {stem+"."+suffix for stem in ("keys", "events") for suffix in ("input", "stdout", "stderr", "json")}
    names.update(stem+"."+suffix for stem in ("key_selftest", "event_selftest") for suffix in ("stdout", "stderr", "json"))
    for prefix, count in (("key_reject", len(key_bad)), ("event_reject", len(event_bad))):
        names.update(f"{prefix}_{i:03}.{suffix}" for i in range(count) for suffix in ("input", "stdout", "stderr", "json"))
    require(output.is_dir() and not output.is_symlink() and {p.name for p in output.iterdir()} == names and
            all(p.is_file() and not p.is_symlink() for p in output.iterdir()), "identity transcript inventory/file shape")
    before = {name: sha(output/name) for name in sorted(names)}
    keys = validate_keys([strict_json(line) for line in read_command(output, key_binary, "keys", key_payload(key_cases)).splitlines()], key_cases)
    events = validate_events([strict_json(line) for line in read_command(output, event_binary, "events", event_payload(event_cases)).splitlines()], event_cases)
    for prefix, binary, cases in (("key", key_binary, key_bad), ("event", event_binary, event_bad)):
        for i, (_, data) in enumerate(cases):
            read_command(output, binary, f"{prefix}_reject_{i:03}", data, expected_exit=1)
    key_selftest = strict_json(read_command(output, key_binary, "key_selftest", arguments=("--selftest",)))
    event_selftest = strict_json(read_command(output, event_binary, "event_selftest", arguments=("--selftest",)))
    check_selftests(key_selftest, event_selftest)
    require({name: sha(output/name) for name in sorted(names)} == before, "identity transcript changed while reading")
    digest = hashlib.sha256(json.dumps(before, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    return dict(keys=keys, events=events, rejected_inputs=dict(keys=len(key_bad), events=len(event_bad)),
                key_selftest=key_selftest, event_selftest=event_selftest, transcript_sha256=digest)


def mutations(key_rows, key_cases, event_rows, event_cases):
    valid = next(i for i, row in enumerate(key_rows) if row["valid"])
    invalid = next(i for i, row in enumerate(key_rows) if not row["valid"])
    q3 = next(i for i, row in enumerate(key_rows) if row["valid"] and row["arity"] == 3)
    key_changes = [lambda rows: rows[valid]["words"].__setitem__(0, 2),
                   lambda rows: rows[valid]["coefficients"].__setitem__(0, "0"),
                   lambda rows: rows[valid]["words"].__setitem__(1, rows[valid]["words"][1]+1),
                   lambda rows: rows[valid]["words"].append(0),
                   lambda rows: rows[valid].__setitem__("valid", False),
                   lambda rows: rows[invalid].__setitem__("valid", True),
                   lambda rows: rows[q3]["from_support_words"].pop(),
                   lambda rows: rows[valid]["work"].__setitem__("keys_created", 0),
                   lambda rows: rows[q3]["from_support_work"]["support"].__setitem__("q3_preparations", 1),
                   lambda rows: rows.pop()]
    event_changes = []
    for sign in (-1, 0, 1):
        at = next(i for i, row in enumerate(event_rows) if row["exact_order"] == sign)
        event_changes.append(lambda rows, at=at, sign=sign: rows[at].__setitem__("filtered_order", -1 if sign == 1 else 1))
    at = next(i for i, row in enumerate(event_rows) if row["exact_valid"] and -1 in row["exact_sides"])
    side = event_rows[at]["exact_sides"].index(-1)
    event_changes.append(lambda rows: rows[at]["filtered_sides"].__setitem__(side, 1))
    cp = next(i for i, row in enumerate(event_rows) if row["exact_coplanar"])
    event_changes.append(lambda rows: rows[cp].__setitem__("filtered_coplanar", False))
    event_changes.append(lambda rows: rows[at].__setitem__("exact_valid", False))
    event_changes.append(lambda rows: rows[at]["exact_comparison_work"].__setitem__("side_queries", 1))
    equal = next(i for i, row in enumerate(event_rows) if row["exact_order"] == 0)
    event_changes.append(lambda rows: rows[equal]["exact_comparison_work"].__setitem__("root_equalities", 0))
    rejected = 0
    for changes, baseline, cases, validate in ((key_changes, key_rows, key_cases, validate_keys),
                                               (event_changes, event_rows, event_cases, validate_events)):
        for change in changes:
            altered = deepcopy(baseline)
            change(altered)
            try:
                validate(altered, cases)
            except ValueError:
                rejected += 1
    require(rejected == 18, "identity output mutation survived")
    return rejected


def run(key_binary, event_binary, output):
    sources = {str(path): sha(path) for path in SOURCES}
    binary_hash, event_hash = sha(key_binary), sha(event_binary)
    output.mkdir(parents=True, exist_ok=False)
    key_cases, event_cases = key_fixtures(), event_fixtures()
    execute(key_binary, output, "keys", key_payload(key_cases))
    execute(event_binary, output, "events", event_payload(event_cases))
    for prefix, binary, cases in (("key", key_binary, invalid_inputs()[0]), ("event", event_binary, invalid_inputs()[1])):
        for i, (_, data) in enumerate(cases):
            execute(binary, output, f"{prefix}_reject_{i:03}", data, expected_exit=1)
    execute(key_binary, output, "key_selftest", arguments=("--selftest",))
    execute(event_binary, output, "event_selftest", arguments=("--selftest",))
    result = read_transcript(output, key_binary, event_binary)
    key_rows = [strict_json(line) for line in read_command(output, key_binary, "keys", key_payload(key_cases)).splitlines()]
    event_rows = [strict_json(line) for line in read_command(output, event_binary, "events", event_payload(event_cases)).splitlines()]
    rejected = mutations(key_rows, key_cases, event_rows, event_cases)
    require(sha(key_binary) == binary_hash and sha(event_binary) == event_hash and {str(p): sha(p) for p in SOURCES} == sources,
            "identity source/binary closure changed")
    return dict(schema="mhgp8_float32_identity_gate_v1", status="passed", **result, rejected_mutations=rejected,
                binary=str(key_binary), binary_sha256=binary_hash, events_binary=str(event_binary), events_binary_sha256=event_hash,
                source_sha256=sources, gate_sha256=sha(Path(__file__)), optimized=bool(sys.flags.optimize),
                scope="binary32_ball_identity_and_q4_event_primitives_not_radius_sort_census_or_FULL")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--events-binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(run(args.binary.resolve(strict=True), args.events_binary.resolve(strict=True), args.output.resolve()), sort_keys=True))


if __name__ == "__main__":
    main()
