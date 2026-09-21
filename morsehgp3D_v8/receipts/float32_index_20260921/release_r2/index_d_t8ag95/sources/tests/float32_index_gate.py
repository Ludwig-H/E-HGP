#!/usr/bin/env python3
"""Independent Fraction oracle for the immutable binary32 index, not FULL.

Each native invocation retains its exact input, stdout, stderr and command,
including rejection fixtures and failed/interrupted invocations. Reading a
transcript repeats the independent geometry/structure checks without rerunning
the binary. No NumPy, engine imports or u16 conversions are used.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCES = (Path(__file__).resolve(), ROOT / "morsehgp3D_v8/tests/float32_index_probe.cpp",
           ROOT / "morsehgp3D_v8/src/spatial/float32_index.hpp",
           ROOT / "morsehgp3D_v8/src/spatial/float32_index.cpp",
           ROOT / "morsehgp3D_v8/src/core/float32_predicates.hpp")
INDEX_FIELDS = ("input_word_triples_copied", "finite_points_validated", "point_objects_constructed",
                "presort_comparisons", "duplicate_adjacent_tests", "box_endpoint_reads",
                "partition_rank_writes", "partition_id_reads", "partition_id_writes",
                "inverse_rank_writes", "nodes", "leaves")
QUERY_FIELDS = ("node_visits", "axis_tests", "rejected_nodes", "accepted_nodes", "refined_nodes",
                "emitted_sites", "callbacks")
MAXIMUM = 0x7f7fffff
ALL_BOX = ((0xff7fffff,) * 3, (MAXIMUM,) * 3)


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
    def nonfinite(value):
        raise ValueError("nonfinite JSON value: " + value)
    return json.loads(data, object_pairs_hook=pairs, parse_constant=nonfinite)


def bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def rational(word):
    require(type(word) is int and 0 <= word < 2**32 and word & 0x7f800000 != 0x7f800000,
            "invalid finite binary32 word")
    # Decode the IEEE representation as integers, independently of native
    # ordered keys, as_double, hardware subnormal modes or floating arithmetic.
    exponent = (word >> 23) & 255
    mantissa = word & 0x7fffff
    if exponent:
        mantissa |= 1 << 23
    power = exponent - 150 if exponent else -149
    result = Fraction(mantissa) * Fraction(2)**power
    return -result if word & 0x80000000 else result


def coordinates(words):
    require(type(words) in (list, tuple) and len(words) == 3, "coordinate triple shape")
    return tuple(rational(word) for word in words)


def fixture(name, points):
    require(points and len(set(map(coordinates, points))) == len(points), "fixture sites must be unique")
    queries = [ALL_BOX]
    queries.extend((p, p) for p in points[:min(5, len(points))])
    queries.extend((((0, 0, 0), (0, 0, 0)),
                    ((0x80000000,) * 3, (0x80000000,) * 3),
                    ((1, 0, 0), (2, 0, 0)),
                    ((bits(-1),) * 3, (bits(1),) * 3)))
    for a, b in zip(points[:6], reversed(points[-6:])):
        low = tuple(x if rational(x) <= rational(y) else y for x, y in zip(a, b, strict=True))
        high = tuple(y if rational(x) <= rational(y) else x for x, y in zip(a, b, strict=True))
        queries.append((low, high))
    return dict(name=name, points=points, queries=queries)


def fixtures():
    result = [fixture("one_signed_zero", [(0x80000000, 0, 0x80000000)]),
              fixture("exponent_chain", [(0, 0, 0)] + [(bits(2.0**e), 0, 0) for e in range(-149, 128)]),
              fixture("negative_exponent_chain", [(0, 0, 0)] + [(bits(-2.0**e), 0, 0) for e in range(-149, 128)]),
              fixture("subnormal_boundary", [(w, 0, 0) for w in
                  (0, 1, 2, 0x007ffffe, 0x007fffff, 0x00800000, 0x00800001,
                   0x80000001, 0x80000002, 0x807fffff, 0x80800000)]),
              fixture("extremes_and_neighbours", [(w, 0, 0) for w in
                  (0xff7fffff, 0xff7ffffe, 0x7f7fffff, 0x7f7ffffe, bits(-1), bits(1)-1, bits(1), bits(1)+1, 1)]),
              fixture("unique_signed_zeros", [(0x80000000, bits(1), 0x80000000),
                  (0, bits(-1), 0), (bits(1), 0x80000000, 0), (bits(-1), 0, 0x80000000)]),
              fixture("axis_ties", [(bits(x), bits(y), bits(z)) for x in (-1, 1)
                                      for y in (-1, 1) for z in (-1, 1)]),
              fixture("rotated_lex_ties", [(0, bits((i * 7) % 17), bits(i // 17)) for i in range(51)])]
    rng = random.Random(0xF321D3A2026)
    for number, n in enumerate((2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65)):
        points, seen = [], set()
        while len(points) < n:
            candidate = tuple(rng.getrandbits(32) for _ in range(3))
            if any(w & 0x7f800000 == 0x7f800000 for w in candidate):
                continue
            key = coordinates(candidate)
            if key not in seen:
                seen.add(key)
                points.append(candidate)
        result.append(fixture(f"random_finite_{number:02}", points))
    for axis in range(3):
        points = []
        for i in range(37):
            point = [0, 0, 0]
            point[axis] = bits(((i * 13) % 37) - 18)
            points.append(tuple(point))
        result.append(fixture(f"shuffled_axis_{axis}", points))
    return result


def payload(case):
    rows = [f"{len(case['points'])} {len(case['queries'])}\n"]
    rows.extend(" ".join(f"{w:08x}" for w in p) + "\n" for p in case["points"])
    rows.extend(" ".join(f"{w:08x}" for w in (*low, *high)) + "\n" for low, high in case["queries"])
    return "".join(rows).encode()


def rejection_cases():
    result = [("empty", b"0 0\n")]
    for mask in range(8):
        zero = tuple(0x80000000 if mask & (1 << axis) else 0 for axis in range(3))
        result.append((f"duplicate_zero_{mask}", payload(dict(points=[(0, 0, 0), zero], queries=[]))))
    for axis in range(3):
        for word in (0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000, 0x7f800001, 0xff800001):
            bad = [0, 0, 0]
            bad[axis] = word
            result.append((f"nonfinite_{axis}_{word:08x}", payload(dict(points=[bad], queries=[]))))
        reversed_low = [0, 0, 0]
        reversed_low[axis] = bits(1)
        result.append((f"reversed_box_{axis}", payload(dict(points=[(0, 0, 0)], queries=[(reversed_low, (0, 0, 0))]))))
    result.extend((("short_input", b"1 0\n00000000 00000000\n"),
                   ("long_input", b"1 0\n00000000 00000000 00000000 trailing\n"),
                   ("bad_hex", b"1 0\nzzzzzzzz 00000000 00000000\n"),
                   ("negative_size", b"-1 0\n"), ("fractional_size", b"1.5 0\n")))
    return result


def counters(value, fields):
    require(type(value) is dict and set(value) == set(fields) and
            all(type(v) is int and 0 <= v < 2**64 for v in value.values()), "work shape/types")


def validate(row, case):
    require(type(row) is dict and set(row) == {"schema", "status", "points", "permutation", "inverse", "nodes",
            "work", "max_depth", "retained_bytes", "construction_peak_vector_bytes", "queries"}, "index result shape")
    require(row["schema"] == "mhgp8_float32_index_probe_v1" and row["status"] == "passed", "index result schema/status")
    expected_points = [list(p) for p in case["points"]]
    require(row["points"] == expected_points, "index changed point bits or original IDs")
    xyz = [coordinates(p) for p in row["points"]]
    n = len(xyz)
    order, inverse, nodes = row["permutation"], row["inverse"], row["nodes"]
    require(type(order) is list and len(order) == n and all(type(i) is int for i in order) and sorted(order) == list(range(n)),
            "spatial order is not a permutation")
    require(type(inverse) is list and len(inverse) == n and all(type(i) is int for i in inverse) and
            all(inverse[original] == rank for rank, original in enumerate(order)), "inverse does not map original IDs to ranks")
    require(type(nodes) is list and len(nodes) == 2*n-1, "index node count")
    visited, internal_mass, deepest = set(), 0, 0
    for node in nodes:
        require(type(node) is dict and set(node) == {"first", "last", "left", "right", "escape", "low", "high"}, "node shape")
        require(all(type(node[k]) is int for k in ("first", "last", "escape")) and
                0 <= node["first"] < node["last"] <= n and 0 < node["escape"] <= len(nodes), "node range/escape type")
        coordinates(node["low"])
        coordinates(node["high"])
    stack = [(0, 0, n, 0)]
    while stack:
        at, first, last, depth = stack.pop()
        require(type(at) is int and 0 <= at < len(nodes) and at not in visited, "node link cycles or leaves index")
        visited.add(at)
        node = nodes[at]
        require((node["first"], node["last"]) == (first, last), "node does not partition parent ranks")
        deepest = max(deepest, depth)
        ids = order[first:last]
        for axis in range(3):
            values = [xyz[i][axis] for i in ids]
            require(rational(node["low"][axis]) == min(values) and rational(node["high"][axis]) == max(values),
                    "node box differs from exact Fraction extrema")
            require(node["low"][axis] in [row["points"][i][axis] for i in ids] and
                    node["high"][axis] in [row["points"][i][axis] for i in ids], "box endpoint bits are not from its sites")
        if last-first == 1:
            require(node["left"] is None and node["right"] is None and node["escape"] == at+1, "leaf identity/escape")
        else:
            internal_mass += last-first
            left, right = node["left"], node["right"]
            require(type(left) is int and type(right) is int and left == at+1 and left < right < len(nodes), "child preorder links")
            require(nodes[left]["escape"] == right and nodes[right]["escape"] == node["escape"], "subtree escape lost sibling")
            middle = first+(last-first)//2
            # At least one numerical axis order must define the rank median.
            # Which axis a rounded-width heuristic chooses is not geometric
            # authority and need not be duplicated in this Fraction oracle.
            require(any(set(sorted(ids, key=lambda i: tuple(xyz[i][(axis+o) % 3] for o in range(3))+(i,))[:middle-first]) ==
                        set(order[first:middle]) for axis in range(3)), "child membership is not a numerical median")
            stack.extend(((right, middle, last, depth+1), (left, first, middle, depth+1)))
    require(len(visited) == len(nodes) and nodes[0]["escape"] == len(nodes), "unreachable nodes or wrong final sentinel")
    require(type(row["max_depth"]) is int and row["max_depth"] == deepest <= (n-1).bit_length(), "median depth bound")
    work = row["work"]
    counters(work, INDEX_FIELDS)
    exact_work = dict(input_word_triples_copied=n, finite_points_validated=n, point_objects_constructed=n,
                      duplicate_adjacent_tests=n-1, box_endpoint_reads=6*len(nodes), partition_rank_writes=internal_mass,
                      partition_id_reads=5*internal_mass, partition_id_writes=4*internal_mass,
                      inverse_rank_writes=n, nodes=len(nodes), leaves=n)
    require(all(work[k] == v for k, v in exact_work.items()), "index paid work differs from tree populations")
    require((work["presort_comparisons"] == 0 if n == 1 else work["presort_comparisons"] > 0) and
            work["presort_comparisons"] <= 100*n*(deepest+1), "presort work absent or exceeds n log n envelope")
    require(all(type(row[k]) is int for k in ("retained_bytes", "construction_peak_vector_bytes")) and
            12*n <= row["retained_bytes"] <= row["construction_peak_vector_bytes"] <= 1024*n,
            "vector storage exceeds linear envelope or omits retained arrays")
    require(type(row["queries"]) is list and len(row["queries"]) == len(case["queries"]), "query result count")
    emitted = empty = refined = 0
    for result, (lo_words, hi_words) in zip(row["queries"], case["queries"], strict=True):
        require(type(result) is dict and set(result) == {"low", "high", "ids", "work"} and
                result["low"] == list(lo_words) and result["high"] == list(hi_words), "query provenance/shape")
        low, high = coordinates(lo_words), coordinates(hi_words)
        expected = [i for i, point in enumerate(xyz) if all(l <= x <= h for l, x, h in zip(low, point, high, strict=True))]
        require(type(result["ids"]) is list and all(type(i) is int for i in result["ids"]) and
                sorted(result["ids"]) == expected, "closed-box query differs from Fraction census")
        paid = dict.fromkeys(QUERY_FIELDS, 0)
        traversal, at = [], 0
        while at < len(nodes):
            node = nodes[at]
            paid["node_visits"] += 1
            outside, inside = False, True
            for axis in range(3):
                paid["axis_tests"] += 1
                nl, nh = rational(node["low"][axis]), rational(node["high"][axis])
                if nh < low[axis] or nl > high[axis]:
                    outside = True
                    break
                inside = inside and low[axis] <= nl and nh <= high[axis]
            if outside:
                paid["rejected_nodes"] += 1
                at = node["escape"]
            elif inside:
                paid["accepted_nodes"] += 1
                paid["callbacks"] += 1
                ids = order[node["first"]:node["last"]]
                paid["emitted_sites"] += len(ids)
                traversal.extend(ids)
                at = node["escape"]
            else:
                paid["refined_nodes"] += 1
                require(node["left"] is not None, "undecidable singleton")
                at = node["left"]
        counters(result["work"], QUERY_FIELDS)
        require(result["work"] == paid and result["ids"] == traversal, "query cursor/work differs from independent box traversal")
        emitted += len(expected)
        empty += not expected
        refined += paid["refined_nodes"]
    return dict(sites=n, nodes=len(nodes), queries=len(case["queries"]), emitted_sites=emitted,
                empty_queries=empty, refined_nodes=refined, internal_population=internal_mass, max_depth=deepest)


def execute(binary, output, name, data=None, arguments=(), expected_exit=0):
    record = dict(command=[str(binary), *arguments], exit_code=None)
    if data is not None:
        (output / (name+".input")).write_bytes(data)
    try:
        with (output / (name+".stdout")).open("xb") as stdout, (output / (name+".stderr")).open("xb") as stderr:
            child = subprocess.run(record["command"], input=data, stdout=stdout, stderr=stderr, check=False)
        record["exit_code"] = child.returncode
    except BaseException as error:
        record["error"] = f"{type(error).__name__}: {error}"
        raise
    finally:
        (output / (name+".json")).write_text(json.dumps(record, sort_keys=True)+"\n")
    require(record["exit_code"] == expected_exit, "unexpected native exit; streams retained: " + name)


def read_command(output, binary, name, data=None, arguments=(), expected_exit=0):
    if data is not None:
        require((output / (name+".input")).read_bytes() == data, "native input differs: " + name)
    record = strict_json((output / (name+".json")).read_bytes())
    require(type(record) is dict and set(record) == {"command", "exit_code"} and
            record["command"] == [str(binary), *arguments] and type(record["exit_code"]) is int and
            record["exit_code"] == expected_exit, "native command/exit differs: " + name)
    stdout, stderr = ((output / (name+"."+stream)).read_bytes() for stream in ("stdout", "stderr"))
    if expected_exit:
        require(not stdout and stderr.startswith(b"float32 index probe: "), "negative fixture lost its diagnostic")
        return None
    require(not stderr, "native success wrote stderr: " + name)
    return strict_json(stdout)


def read_transcript(output, binary):
    cases, bad = fixtures(), rejection_cases()
    expected_files = {"selftest."+suffix for suffix in ("stdout", "stderr", "json")}
    for prefix, count in (("case", len(cases)), ("reject", len(bad))):
        expected_files.update(f"{prefix}_{i:03}.{suffix}" for i in range(count)
                              for suffix in ("input", "stdout", "stderr", "json"))
    require(output.is_dir() and not output.is_symlink() and
            {p.name for p in output.iterdir()} == expected_files and
            all(p.is_file() and not p.is_symlink() for p in output.iterdir()), "index transcript inventory/file shape")
    before = {name: sha(output/name) for name in sorted(expected_files)}
    totals = dict(sites=0, nodes=0, queries=0, emitted_sites=0, empty_queries=0, refined_nodes=0, internal_population=0)
    depths = {}
    for number, case in enumerate(cases):
        row = read_command(output, binary, f"case_{number:03}", payload(case))
        actual = validate(row, case)
        depths[case["name"]] = actual.pop("max_depth")
        for key, value in actual.items():
            totals[key] += value
    for number, (_, data) in enumerate(bad):
        read_command(output, binary, f"reject_{number:03}", data, expected_exit=1)
    native = read_command(output, binary, "selftest", arguments=("--selftest",))
    require(type(native) is dict and set(native) == {"schema", "status", "tests", "invalid_inputs", "rounding_modes",
            "flush_modes", "concurrent_workers", "concurrent_queries"} and
            native["schema"] == "mhgp8_float32_index_selftest_v1" and native["status"] == "passed" and
            all(type(native[key]) is int for key in ("tests", "invalid_inputs", "rounding_modes", "flush_modes",
                                                    "concurrent_workers", "concurrent_queries")) and
            native["tests"] == 103+12*native["flush_modes"] and native["invalid_inputs"] == 67 and
            native["rounding_modes"] == 4 and native["flush_modes"] in (1, 4) and
            native["concurrent_workers"] == 4 and native["concurrent_queries"] == 64,
            "index native selftest lacks required coverage")
    require(depths["exponent_chain"] == depths["negative_exponent_chain"] == 9 and
            totals["empty_queries"] > 0 and totals["refined_nodes"] > 0, "index oracle non-vacuity")
    require({name: sha(output/name) for name in sorted(expected_files)} == before, "index transcript changed while reading")
    digest = hashlib.sha256(json.dumps(before, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    return dict(fixtures=len(cases), rejected_inputs=len(bad), **totals, native_selftest=native,
                exponent_chain_depth=9, transcript_sha256=digest)


def mutations(row, case):
    altered = []
    def change(action):
        item = deepcopy(row)
        action(item)
        altered.append(item)
    change(lambda r: r["points"][0].__setitem__(0, 1))
    change(lambda r: r["permutation"].__setitem__(0, r["permutation"][1]))
    change(lambda r: r["inverse"].__setitem__(r["permutation"][0], 1))
    change(lambda r: r["nodes"][0]["high"].__setitem__(0, 0))
    change(lambda r: r["nodes"][0].__setitem__("escape", 1))
    change(lambda r: r["nodes"][1].__setitem__("last", 1))
    change(lambda r: r["queries"][0]["ids"].pop())
    change(lambda r: r["queries"][0]["work"].__setitem__("node_visits", 0))
    change(lambda r: r["work"].__setitem__("partition_id_reads", 0))
    change(lambda r: r.__setitem__("max_depth", 48))
    change(lambda r: r.__setitem__("construction_peak_vector_bytes", 0))
    rejected = 0
    for item in altered:
        try:
            validate(item, case)
        except ValueError:
            rejected += 1
    require(rejected == len(altered) == 11, "index oracle mutation survived")
    return rejected


def run(binary, output):
    before = {str(source): sha(source) for source in SOURCES}
    binary_hash = sha(binary)
    output.mkdir(parents=True, exist_ok=False)
    cases = fixtures()
    for number, case in enumerate(cases):
        execute(binary, output, f"case_{number:03}", payload(case))
    for number, (_, data) in enumerate(rejection_cases()):
        execute(binary, output, f"reject_{number:03}", data, expected_exit=1)
    execute(binary, output, "selftest", arguments=("--selftest",))
    result = read_transcript(output, binary)
    row = read_command(output, binary, "case_001", payload(cases[1]))
    rejected = mutations(row, cases[1])
    require(sha(binary) == binary_hash and {str(source): sha(source) for source in SOURCES} == before,
            "index binary/sources changed during gate")
    return dict(schema="mhgp8_float32_index_gate_v1", status="passed", **result, rejected_mutations=rejected,
                binary=str(binary), binary_sha256=binary_hash, source_sha256=before,
                gate_sha256=sha(Path(__file__)), optimized=bool(sys.flags.optimize),
                scope="binary32_immutable_index_and_box_queries_not_WSPD_census_or_FULL")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(run(args.binary.resolve(strict=True), args.output.resolve()), sort_keys=True))


if __name__ == "__main__":
    main()
