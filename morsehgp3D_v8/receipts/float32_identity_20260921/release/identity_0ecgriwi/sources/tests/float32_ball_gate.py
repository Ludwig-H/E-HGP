#!/usr/bin/env python3
"""Independent rational circumball oracle for binary32 q3/q4 primitives.

The oracle solves a Gram system over Fraction, obtains the affine circumcentre
and tests strictly positive barycentric coordinates. It does not import any
native formula or use a floating tolerance. These tests do not qualify census,
WSPD, a canonical ball catalogue, a hierarchy, or GPU execution.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
from fractions import Fraction
from functools import lru_cache
import hashlib
from itertools import permutations
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCES = (Path(__file__).resolve(), ROOT / "morsehgp3D_v8/tests/float32_ball_probe.cpp",
           ROOT / "morsehgp3D_v8/src/core/float32_ball.hpp",
           ROOT / "morsehgp3D_v8/src/core/float32_ball.cpp",
           ROOT / "morsehgp3D_v8/src/core/fixed_signed.hpp",
           ROOT / "morsehgp3D_v8/src/core/float32_predicates.hpp")
FIELDS = ("q3_preparations", "q4_preparations", "preparation_filter_attempts", "preparation_filter_accepts",
          "preparation_exact_fallbacks", "preparation_exact_evaluations", "accepted_supports", "rejected_supports",
          "power_queries", "power_filter_attempts", "power_filter_accepts", "power_exact_fallbacks",
          "power_exact_evaluations", "interval_additions", "interval_products", "exact_additions",
          "exact_products", "exact_point_decodes")
WORK_KINDS = ("filtered_preparation_work", "exact_preparation_work", "filtered_power_work", "exact_power_work")


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
    exp, mantissa = (word >> 23) & 255, word & 0x7fffff
    if exp:
        mantissa |= 1 << 23
    value = Fraction(mantissa) * Fraction(2)**(exp-150 if exp else -149)
    return -value if word & 0x80000000 else value


def xyz(words):
    return tuple(rational(word) for word in words)


def dot(a, b):
    return sum((x*y for x, y in zip(a, b, strict=True)), Fraction(0))


def solve(matrix, rhs):
    """Independent Gaussian elimination, not a determinant expansion."""
    n = len(rhs)
    rows = [list(row)+[value] for row, value in zip(matrix, rhs, strict=True)]
    for column in range(n):
        pivot = next((r for r in range(column, n) if rows[r][column]), None)
        if pivot is None:
            return None
        rows[column], rows[pivot] = rows[pivot], rows[column]
        scale = rows[column][column]
        rows[column] = [v/scale for v in rows[column]]
        for r in range(n):
            if r != column:
                factor = rows[r][column]
                rows[r] = [v-factor*w for v, w in zip(rows[r], rows[column], strict=True)]
    return tuple(row[-1] for row in rows)


@lru_cache(maxsize=None)
def ball(support):
    points = tuple(map(xyz, support))
    origin = points[0]
    edges = [tuple(x-a for x, a in zip(p, origin, strict=True)) for p in points[1:]]
    gram = [[dot(a, b) for b in edges] for a in edges]
    coefficient = solve(gram, [dot(edge, edge)/2 for edge in edges])
    if coefficient is None:
        return None
    barycentric = (1-sum(coefficient), *coefficient)
    if not all(value > 0 for value in barycentric):
        return None
    centre = tuple(origin[axis]+sum(c*edge[axis] for c, edge in zip(coefficient, edges, strict=True))
                   for axis in range(3))
    radius2 = sum((a-c)**2 for a, c in zip(origin, centre, strict=True))
    # Independent internal consistency, paid only in this bounded oracle.
    require(all(sum((a-c)**2 for a, c in zip(point, centre, strict=True)) == radius2 for point in points),
            "rational centre is not equidistant from its support")
    return centre, radius2


def oracle(case):
    result = ball(case["support"])
    if result is None:
        return False, None
    centre, radius2 = result
    power = sum((z-c)**2 for z, c in zip(xyz(case["query"]), centre, strict=True))-radius2
    return True, (power > 0)-(power < 0)


def fixtures():
    result = []
    def add(label, support, query, valid=None, sign=None):
        case = dict(label=label, support=tuple(tuple(p) for p in support), query=tuple(query))
        actual_valid, actual_sign = oracle(case)
        require(valid is None or actual_valid == valid, "incorrect support fixture: " + label)
        require(sign is None or actual_sign == sign, "incorrect power fixture: " + label)
        result.append(case)
    def encoded(points):
        return tuple(tuple(bits(x) for x in p) for p in points)
    triangle = encoded(((-1, 0, 0), (1, 0, 0), (0, 2, 0)))
    tetra = encoded(((1, 1, 1), (1, -1, -1), (-1, 1, -1), (-1, -1, 1)))
    tq = encoded(((0, .75, 0), (0, .75, 1.25), (0, .75, 2), (-1, 0, 0), (1, 0, 0),
                  (0, 2, 0), (0, -.5, 0), (1.25, .75, 0)))
    tq += ((0, bits(.75), bits(1.25)-1), (0, bits(.75), bits(1.25)+1))
    sq = encoded(((0, 0, 0), (2, 0, 0), (1, 1, -1), *tuple(tuple(float(rational(w)) for w in p) for p in tetra)))
    sq += ((bits(1)-1, bits(1), bits(1)), (bits(1)+1, bits(1), bits(1)))
    for arity, support, queries in ((3, triangle, tq), (4, tetra, sq)):
        for permutation in permutations(support):
            for query in queries:
                add(f"q{arity}_permutations", permutation, query, valid=True)
    invalid3 = [encoded(((0, 0, 0), (0, 0, 0), (1, 0, 0))),
                encoded(((-1, 0, 0), (0, 0, 0), (1, 0, 0))),
                encoded(((-1, 0, 0), (1, 0, 0), (0, .5, 0))),
                encoded(((-1, 0, 0), (1, 0, 0), (0, 1, 0)))]
    facet = encoded(((-1, 0, 0), (1, 0, 0), (0, 2, 0), (0, .75, 1.25)))
    invalid4 = [tetra[:3]+(tetra[0],), encoded(((-1, -1, 0), (1, -1, 0), (1, 1, 0), (-1, 1, 0))),
                encoded(((0, 0, 0), (1, 0, 0), (2, 0, 0), (3, 0, 0))),
                encoded(((0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1))), facet]
    for arity, family in ((3, invalid3), (4, invalid4)):
        for i, support in enumerate(family):
            for permutation in permutations(support):
                add(f"q{arity}_invalid_{i}", permutation, (0, 0, 0), valid=False)
    for delta in (-1, 0, 1):
        support = triangle[:2]+((0, bits(1)+delta, 0),)
        for offset in (-1, 0, 1):
            add("q3_right_angle_ulp", support, (0, bits(1)+delta+offset, 0), valid=delta > 0)
        support4 = facet[:3]+((0, bits(.75), bits(1.25)+delta),)
        for query in ((0, bits(.75), 0), support4[-1], (bits(2), 0, 0)):
            add("q4_centre_facet_ulp", support4, query, valid=delta > 0)
    for exponent in (-149, -140, -120, -60, 0, 60, 120, 125):
        scale = 2.0**exponent
        for arity, support, queries in ((3, triangle, tq), (4, tetra, sq)):
            transformed = tuple(tuple(bits(float(rational(w))*scale) for w in p) for p in support)
            for query in queries:
                z = tuple(bits(float(rational(w))*scale) for w in query)
                add(f"q{arity}_scale_{exponent}", transformed, z, valid=True)
    for huge in (bits(2.0**120), 0x7f7fffff):
        tiny = 1
        thin3 = ((huge, 0, 0), (huge | 0x80000000, 0, 0), (0, huge, tiny))
        thin4 = thin3+((0, huge | 0x80000000, tiny),)
        for arity, support in ((3, thin3), (4, thin4)):
            queries = ((0, 0, 0), (huge, tiny, 0), (huge, 0, tiny), support[-1],
                       (0, huge, 0), (0, huge | 0x80000000, 0))
            for permutation in permutations(support):
                for query in queries:
                    add(f"q{arity}_extreme_cancellation", permutation, query, valid=True)
        add("q4_extreme_positive_tiny_power", thin4, (huge, tiny, 0), valid=True, sign=1)
        add("q4_extreme_extra_contact", thin4, (huge, 0, tiny), valid=True, sign=0)
    thin_regular = ((bits(1), bits(1), 1), (bits(1), bits(-1), 0x80000001),
                    (bits(-1), bits(1), 0x80000001), (bits(-1), bits(-1), 1))
    for permutation in permutations(thin_regular):
        for query, sign in (((bits(1), bits(1), 0), -1), ((bits(1), bits(1), 2), 1), (thin_regular[0], 0)):
            add("q4_flat_regular_tiny_power", permutation, query, valid=True, sign=sign)
    for delta in (-1, 0, 1):
        face5 = encoded(((5, 0, 0), (-3, 4, 0), (-3, -4, 0)))+((0, 0, bits(5)+delta),)
        for permutation in permutations(face5):
            for query in ((0, 0, 0), face5[-1], (bits(8), 0, 0)):
                add("q4_face5_permuted_ulp", permutation, query, valid=delta > 0)
    normal_subnormal = ((0, 0, 0), (0x00800000, 0, 0), (0x00400000, 0x00400000, 1))
    for permutation in permutations(normal_subnormal):
        add("q3_normal_subnormal_decode_boundary", permutation, normal_subnormal[0], valid=True, sign=0)
    for mask in range(8):
        support = tuple(tuple(w if w else (0x80000000 if mask & (1 << axis) else 0)
                              for axis, w in enumerate(p)) for p in triangle)
        add("q3_signed_zeros", support, (0x80000000, bits(.75), 0x80000000), valid=True, sign=-1)
    rng = random.Random(0xF32BA112026)
    for arity in (3, 4):
        for _ in range(40):
            support = tuple(tuple(bits(rng.randint(-7, 7)) for _ in range(3)) for _ in range(arity))
            for query in ((0, 0, 0), support[0], (bits(32),)*3,
                          tuple(bits(rng.randint(-9, 9)) for _ in range(3))):
                add(f"q{arity}_random_integer", support, query)
        for _ in range(30):
            def finite():
                while True:
                    word = rng.getrandbits(32)
                    if word & 0x7f800000 != 0x7f800000:
                        return word
            support = tuple(tuple(finite() for _ in range(3)) for _ in range(arity))
            add(f"q{arity}_random_words", support, tuple(finite() for _ in range(3)))
    return result


def payload(cases):
    return "".join(str(len(c["support"]))+" "+" ".join(f"{w:08x}" for p in (*c["support"], c["query"]) for w in p)+"\n"
                   for c in cases).encode()


def invalid_inputs():
    good = payload([dict(support=((0, 0, 0), (bits(1), 0, 0), (0, bits(1), 0)), query=(0, 0, 0))])
    result = [("empty", b""), ("bad_arity", good.replace(b"3 ", b"2 ", 1)),
              ("too_short", b"3 00000000\n"), ("too_long", good.rstrip()+b" 00000000\n"),
              ("bad_hex", good.replace(b"00000000", b"xxxxxxxx", 1))]
    for arity in (3, 4):
        for location in (0, arity):
            for axis in range(3):
                for word in (0x7f800000, 0xff800000, 0x7fc00001, 0x7f800001):
                    words = [[0, 0, 0] for _ in range(arity+1)]
                    words[location][axis] = word
                    result.append((f"nonfinite_{arity}_{location}_{axis}_{word:08x}",
                                   payload([dict(support=words[:-1], query=words[-1])])) )
    return result


def check_work(work, arity, preparation, filtered, valid, sign):
    require(type(work) is dict and set(work) == set(FIELDS) and all(type(v) is int and 0 <= v < 2**64 for v in work.values()),
            "ball work shape/types")
    if not preparation and not valid:
        require(not any(work.values()), "invalid support performed a power query")
        return
    if preparation:
        require(work["q3_preparations"] == (arity == 3) and work["q4_preparations"] == (arity == 4) and
                work["accepted_supports"] == valid and work["rejected_supports"] == (not valid), "support work partition")
        require(all(work[k] == 0 for k in FIELDS if k.startswith("power_")), "preparation charged power work")
        stem, decoded = "preparation", arity
    else:
        require(work["q3_preparations"] == work["q4_preparations"] == work["accepted_supports"] == work["rejected_supports"] == 0 and
                all(work[k] == 0 for k in FIELDS if k.startswith("preparation_")) and work["power_queries"] == 1,
                "query changed preparation ledger")
        stem, decoded = "power", arity+1
    if filtered:
        require(work[stem+"_filter_attempts"] == 1 and
                work[stem+"_filter_accepts"]+work[stem+"_exact_fallbacks"] == 1 and
                work[stem+"_exact_evaluations"] == work[stem+"_exact_fallbacks"], "filter decision partition")
        if not preparation and sign == 0:
            require(work["power_exact_fallbacks"] == 1, "filter fabricated an exact contact")
        require(work["interval_additions"] > 0 and work["interval_products"] > 0, "filter omitted interval arithmetic")
    else:
        require(work[stem+"_filter_attempts"] == work[stem+"_filter_accepts"] == work[stem+"_exact_fallbacks"] == 0 and
                work[stem+"_exact_evaluations"] == 1 and work["interval_additions"] == work["interval_products"] == 0,
                "exact-only path used floating filter")
    exact = work[stem+"_exact_evaluations"]
    require(work["exact_point_decodes"] == decoded*exact, "exact fallback did not decode the original full support/query")
    require((work["exact_additions"] > 0 and work["exact_products"] > 0) if exact else
            (work["exact_additions"] == work["exact_products"] == 0), "exact arithmetic ledger differs from evaluations")


def validate_rows(rows, cases):
    require(type(rows) is list and len(rows) == len(cases), "ball row count")
    counts = {str(q): dict(valid=0, invalid=0, signs={"-1": 0, "0": 0, "1": 0}, preparation_filter_accepts=0,
                          preparation_exact_fallbacks=0, power_filter_accepts=0, power_exact_fallbacks=0) for q in (3, 4)}
    for row, case in zip(rows, cases, strict=True):
        arity = len(case["support"])
        valid, sign = oracle(case)
        require(type(row) is dict and set(row) == {"arity", "filtered_valid", "exact_valid", "filtered_sign", "exact_sign", *WORK_KINDS},
                "ball result shape")
        require(type(row["arity"]) is int and row["arity"] == arity and
                type(row["filtered_valid"]) is type(row["exact_valid"]) is bool and
                row["filtered_valid"] == row["exact_valid"] == valid, "support positivity differs from Fraction Gram oracle")
        require((type(row["filtered_sign"]) is type(row["exact_sign"]) is int and row["filtered_sign"] == row["exact_sign"] == sign)
                if valid else (row["filtered_sign"] is row["exact_sign"] is None), "power differs from rational centre/radius")
        for key in WORK_KINDS:
            check_work(row[key], arity, "preparation" in key, key.startswith("filtered"), valid, sign)
        summary = counts[str(arity)]
        summary["valid" if valid else "invalid"] += 1
        if valid:
            summary["signs"][str(sign)] += 1
        for key in ("preparation_filter_accepts", "preparation_exact_fallbacks"):
            summary[key] += row["filtered_preparation_work"][key]
        for key in ("power_filter_accepts", "power_exact_fallbacks"):
            summary[key] += row["filtered_power_work"][key]
    for summary in counts.values():
        require(summary["valid"] and summary["invalid"] and all(summary["signs"].values()) and
                all(summary[key] > 0 for key in ("preparation_filter_accepts", "preparation_exact_fallbacks",
                                                "power_filter_accepts", "power_exact_fallbacks")), "q3/q4 oracle/filter non-vacuity")
    return dict(cases=len(cases), populations=counts)


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
    require(record["exit_code"] == expected_exit, "native ball exit differs; streams retained: " + name)


def read_command(output, binary, name, data=None, arguments=(), expected_exit=0):
    if data is not None:
        require((output / (name+".input")).read_bytes() == data, "native ball input differs: " + name)
    record = strict_json((output / (name+".json")).read_bytes())
    require(type(record) is dict and set(record) == {"command", "exit_code"} and record["command"] == [str(binary), *arguments] and
            type(record["exit_code"]) is int and record["exit_code"] == expected_exit, "native ball command/exit differs")
    stdout, stderr = ((output / (name+"."+stream)).read_bytes() for stream in ("stdout", "stderr"))
    if expected_exit:
        require(not stdout and stderr.startswith(b"float32 ball probe: "), "negative native ball diagnostic missing")
    else:
        require(not stderr, "native ball success wrote stderr")
    return stdout


def read_transcript(output, binary):
    cases, rejected = fixtures(), invalid_inputs()
    names = {"probe."+s for s in ("input", "stdout", "stderr", "json")} | {"selftest."+s for s in ("stdout", "stderr", "json")}
    names.update(f"reject_{i:03}.{s}" for i in range(len(rejected)) for s in ("input", "stdout", "stderr", "json"))
    require(output.is_dir() and not output.is_symlink() and {p.name for p in output.iterdir()} == names and
            all(p.is_file() and not p.is_symlink() for p in output.iterdir()), "ball transcript inventory/file shape")
    before = {name: sha(output/name) for name in sorted(names)}
    stdout = read_command(output, binary, "probe", payload(cases))
    result = validate_rows([strict_json(line) for line in stdout.splitlines()], cases)
    for i, (_, data) in enumerate(rejected):
        read_command(output, binary, f"reject_{i:03}", data, expected_exit=1)
    native = strict_json(read_command(output, binary, "selftest", arguments=("--selftest",)))
    require(type(native) is dict and set(native) == {"schema", "status", "tests", "query_checks", "invalid_modes",
            "nonfinite_rejections", "rounding_modes", "flush_modes", "concurrent_workers", "concurrent_queries", "support_bytes"} and
            native["schema"] == "mhgp8_float32_ball_selftest_v1" and native["status"] == "passed" and
            all(type(v) is int for k, v in native.items() if k not in ("schema", "status")) and
            native["flush_modes"] in (1, 4) and native["tests"] == 39+312*native["flush_modes"] and
            native["query_checks"] == 132+104*native["flush_modes"] and native["invalid_modes"] == 2 and
            native["nonfinite_rejections"] == 18 and native["rounding_modes"] == 4 and
            native["concurrent_workers"] == 4 and native["concurrent_queries"] == 128 and 0 < native["support_bytes"] <= 512,
            "native ball selftest lacks required coverage")
    require({name: sha(output/name) for name in sorted(names)} == before, "ball transcript changed while reading")
    digest = hashlib.sha256(json.dumps(before, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    return dict(**result, rejected_inputs=len(rejected), native_selftest=native, transcript_sha256=digest)


def mutations(rows, cases):
    variants = []
    for arity in (3, 4):
        for sign in (-1, 0, 1):
            changed = deepcopy(rows)
            at = next(i for i, row in enumerate(rows) if row["arity"] == arity and row["exact_sign"] == sign)
            changed[at]["filtered_sign"] = -1 if sign == 1 else 1
            variants.append(changed)
    valid = next(i for i, row in enumerate(rows) if row["exact_valid"])
    invalid = next(i for i, row in enumerate(rows) if not row["exact_valid"])
    for at, key, value in ((valid, "filtered_valid", False), (invalid, "exact_valid", True),
                           (valid, "filtered_sign", None)):
        changed = deepcopy(rows)
        changed[at][key] = value
        variants.append(changed)
    variants.append(rows[:-1])
    changed = deepcopy(rows)
    changed[valid]["exact_power_work"]["exact_point_decodes"] -= 1
    variants.append(changed)
    changed = deepcopy(rows)
    changed[valid]["filtered_preparation_work"]["preparation_filter_attempts"] = 0
    variants.append(changed)
    changed = deepcopy(rows)
    del changed[valid]["arity"]
    variants.append(changed)
    rejected = 0
    for changed in variants:
        try:
            validate_rows(changed, cases)
        except ValueError:
            rejected += 1
    require(rejected == len(variants) == 13, "ball oracle mutation survived")
    return rejected


def run(binary, output):
    sources = {str(p): sha(p) for p in SOURCES}
    binary_hash = sha(binary)
    output.mkdir(parents=True, exist_ok=False)
    cases = fixtures()
    execute(binary, output, "probe", payload(cases))
    for i, (_, data) in enumerate(invalid_inputs()):
        execute(binary, output, f"reject_{i:03}", data, expected_exit=1)
    execute(binary, output, "selftest", arguments=("--selftest",))
    result = read_transcript(output, binary)
    rows = [strict_json(line) for line in read_command(output, binary, "probe", payload(cases)).splitlines()]
    rejected = mutations(rows, cases)
    require(sha(binary) == binary_hash and {str(p): sha(p) for p in SOURCES} == sources, "ball binary/sources changed during gate")
    return dict(schema="mhgp8_float32_ball_gate_v1", status="passed", **result, rejected_mutations=rejected,
                binary=str(binary), binary_sha256=binary_hash, source_sha256=sources, gate_sha256=sha(Path(__file__)),
                optimized=bool(sys.flags.optimize), scope="binary32_positive_q3_q4_supports_and_power_not_census_or_FULL")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(run(args.binary.resolve(strict=True), args.output.resolve()), sort_keys=True))


if __name__ == "__main__":
    main()
