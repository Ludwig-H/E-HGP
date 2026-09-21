#!/usr/bin/env python3
"""Independent all-site Fraction census for one supplied edge/seed subtree.

The mathematical helper is explicitly reused from float32_identity_gate.py;
that source is pinned with this gate. No old u16 engine or native bound formula
is an oracle. Small exhaustive fixtures are judges, not candidate generators.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import hashlib
import importlib.util
from itertools import product
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
IDENTITY = ROOT / "morsehgp3D_v8/tests/float32_identity_gate.py"
_spec = importlib.util.spec_from_file_location("q3_census_fraction_authority", IDENTITY)
fraction_oracle = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(fraction_oracle)
MAXIMUM_K = (1 << 64)-1
SOURCES = (Path(__file__).resolve(), IDENTITY,
           ROOT / "morsehgp3D_v8/tests/float32_q3_census_probe.cpp", *(
               ROOT / "morsehgp3D_v8/src" / name for name in
               ("core/fixed_signed.hpp", "core/float32_predicates.hpp", "core/float32_ball.hpp", "core/float32_ball.cpp",
                "core/float32_q3_block.hpp", "core/float32_q3_block.cpp", "spatial/float32_index.hpp",
                "spatial/float32_index.cpp", "lanes/float32_q3_census.hpp", "lanes/float32_q3_census.cpp")))
BALL_FIELDS = fraction_oracle.BALL_FIELDS
BLOCK_FIELDS = "preparations gram_positive gram_unresolved center_axes_tightened center_intersection_fallbacks bound_queries inside_certificates outside_certificates unknown_bounds axis_parabolas vertex_clamps power_evaluations interval_additions interval_products interval_divisions scalar_divisions".split()
CENSUS_FIELDS = "calls input_seed_slots shared_frames shared_splits shared_witness_visits shared_endpoint_skips shared_inside_nodes shared_inside_sites shared_outside_nodes shared_witness_splits shared_saturated_blocks shared_rejected_seed_slots shared_children_with_credit relay_blocks relayed_seed_slots endpoint_seeds support_candidates invalid_supports valid_supports relays_with_credit relays_at_eof count_node_visits count_point_tests count_inside_nodes count_inside_sites count_outside_nodes count_splits saturated_supports accepted_supports shell_node_visits shell_point_tests shell_excluded_nodes shell_splits shell_ids callbacks peak_pending_frames stack_capacity_bytes peak_shell_capacity_bytes".split()
INDEX_FIELDS = "input_word_triples_copied finite_points_validated point_objects_constructed presort_comparisons duplicate_adjacent_tests box_endpoint_reads partition_rank_writes partition_id_reads partition_id_writes inverse_rank_writes nodes leaves".split()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def strict_json(data):
    return fraction_oracle.strict_json(data)


def bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def encoded(points):
    return tuple(tuple(bits(value) for value in point) for point in points)


def requests(n, a=0, b=1):
    result = []
    for kmax in (2, 5, 10, MAXIMUM_K):
        for grain in (1, 4, 16):
            for mode in (0, 1):
                result.append(dict(a=a, b=b, node=0, kmax=kmax, mode=mode, grain=grain))
    for node in (1, 2*n-2):
        for mode, grain in ((0, 1), (1, 1), (1, 4), (1, 16)):
            result.append(dict(a=a, b=b, node=node, kmax=5, mode=mode, grain=grain))
    result.extend(dict(a=b, b=a, node=0, kmax=5, mode=mode, grain=1) for mode in (0, 1))
    return result


def fixtures():
    result = []
    def add(name, points, a=0, b=1):
        points = tuple(tuple(p) for p in points)
        require(len(points) >= 3 and len(set(map(fraction_oracle.xyz, points))) == len(points), "fixture sites must be unique")
        result.append(dict(name=name, points=points, requests=requests(len(points), a, b)))
    basic = encoded(((-1, 0, 0), (1, 0, 0), (0, 2, 0)))
    add("one_acute_seed", basic)
    add("no_positive_seed", encoded(((-1, 0, 0), (1, 0, 0), (0, 1, 0), (0, .5, 0), (0, 0, 0), (2, 0, 0), (-2, 0, 0))))
    mixed = encoded(((-1, 0, 0), (1, 0, 0), (0, 2, 0), (0, 0, 2), (0, -2, 0), (0, 0, -2),
                     (0, 0, 0), (0, .5, 0), (0, 1, 0), (2, 0, 0), (0, .75, 1.25)))
    add("invalid_seed_is_interior_witness", mixed)
    add("signed_zero_ids", tuple(tuple(w if w else 0x80000000 for w in point) for point in mixed))
    for exponent in (-149, -120, 60, 120):
        points = tuple(tuple(bits(float(fraction_oracle.rational(w))*2.0**exponent) for w in point) for point in basic)
        add(f"tiny_or_large_{exponent}", points)
    huge = 0x7f7fffff
    add("extreme_positive_seed_with_invalid_witness", ((huge, 0, 0), (huge | 0x80000000, 0, 0),
        (0, huge, 1), (0, huge, 0), (0, huge | 0x80000000, 0), (0, 0, 0), (huge, 1, 0)))
    sphere = sorted({p for p in product(range(-5, 6), repeat=3) if sum(x*x for x in p) == 25})
    require(len(sphere) == 30, "cosphere fixture inventory")
    edge = ((5, 0, 0), (-3, 4, 0))
    ordered = list(edge)+[p for p in sphere if p not in edge]
    add("cosphere30", encoded(ordered))
    interiors = [(i/16, .125, -.125) for i in range(-8, 9)]
    add("cosphere30_with_interiors", encoded(ordered+interiors))
    cloud = [(-1, 0, 0), (1, 0, 0)]
    cloud += [((i % 3-1)/64, i/128, (i % 5-2)/128) for i in range(12)]
    cloud += [((i % 5-2)/64, 2+i/64, (i % 7-3)/128) for i in range(37)]
    add("seeds_and_witness_blocks", encoded(cloud))
    add("column37", encoded([(-1, 0, 0), (1, 0, 0)]+[(0, 2+i/4, 0) for i in range(35)]))
    rng = random.Random(0xF32CE353)
    for n in (5, 9, 17, 33):
        points = [(-1, 0, 0), (1, 0, 0)]
        while len(points) < n:
            point = tuple(rng.randint(-12, 12)/4 for _ in range(3))
            if point not in points:
                points.append(point)
        add(f"random_{n}", encoded(points))
    order = list(range(len(mixed)))
    rng.shuffle(order)
    add("permuted_original_ids", [mixed[i] for i in order], order.index(0), order.index(1))
    return result


def payload(case):
    rows = [f"{len(case['points'])} {len(case['requests'])}\n"]
    rows.extend(" ".join(f"{w:08x}" for w in point)+"\n" for point in case["points"])
    rows.extend(" ".join(str(r[k]) for k in ("a", "b", "node", "kmax", "mode", "grain"))+"\n" for r in case["requests"])
    return "".join(rows).encode()


def scalar_census(case, request, seed_ids):
    points = case["points"]
    coordinates = tuple(map(fraction_oracle.xyz, points))
    accepted, invalid, saturated = {}, 0, 0
    for seed in seed_ids:
        if seed in (request["a"], request["b"]):
            continue
        support = tuple(points[i] for i in (request["a"], request["b"], seed))
        sphere = fraction_oracle.circumball(support)
        if sphere is None:
            invalid += 1
            continue
        centre, radius2 = sphere
        depth, shell = 0, []
        for i, point in enumerate(coordinates):
            power = sum((value-c)**2 for value, c in zip(point, centre, strict=True))-radius2
            depth += power < 0
            if power == 0:
                shell.append(i)
        if depth < request["kmax"]-1:
            accepted[seed] = dict(seed=seed, depth=depth, shell=shell)
        else:
            saturated += 1
    return accepted, invalid, saturated


def validate_tree(row, case):
    points = [list(p) for p in case["points"]]
    require(row["points"] == points, "census owner changed original bits/IDs")
    xyz = tuple(map(fraction_oracle.xyz, points))
    n = len(points)
    order, nodes = row["permutation"], row["nodes"]
    require(type(order) is list and all(type(i) is int for i in order) and sorted(order) == list(range(n)), "census spatial permutation")
    require(type(nodes) is list and len(nodes) == 2*n-1, "census seed index node count")
    for node in nodes:
        require(type(node) is dict and set(node) == {"first", "last", "left", "right", "escape", "low", "high"} and
                all(type(node[k]) is int for k in ("first", "last", "escape")) and
                0 <= node["first"] < node["last"] <= n and 0 < node["escape"] <= len(nodes), "census node shape")
    stack, visited = [(0, 0, n, 0)], set()
    while stack:
        at, first, last, depth = stack.pop()
        require(type(at) is int and 0 <= at < len(nodes) and at not in visited and depth <= (n-1).bit_length(), "census index links/depth")
        visited.add(at)
        node = nodes[at]
        require((node["first"], node["last"]) == (first, last), "census subtree range")
        ids = order[first:last]
        for axis in range(3):
            low, high = min(xyz[i][axis] for i in ids), max(xyz[i][axis] for i in ids)
            require(fraction_oracle.rational(node["low"][axis]) == low and fraction_oracle.rational(node["high"][axis]) == high,
                    "census seed/witness box is not its exact Fraction envelope")
        if last-first == 1:
            require(node["left"] is node["right"] is None and node["escape"] == at+1, "census index leaf escape")
        else:
            left, right = node["left"], node["right"]
            require(type(left) is int and type(right) is int and left == at+1 and left < right < len(nodes) and
                    nodes[left]["escape"] == right and nodes[right]["escape"] == node["escape"], "census subtree escape")
            middle = first+(last-first)//2
            require(any(set(sorted(ids, key=lambda i: tuple(xyz[i][(axis+j) % 3] for j in range(3))+(i,))[:middle-first]) ==
                        set(order[first:middle]) for axis in range(3)), "census index median membership")
            stack.extend(((right, middle, last, depth+1), (left, first, middle, depth+1)))
    require(len(visited) == len(nodes) and nodes[0]["escape"] == len(nodes), "census index coverage")
    return order, nodes


def shape(work, fields):
    require(type(work) is dict and set(work) == set(fields) and
            all(type(v) is int and 0 <= v < 2**64 for v in work.values()), "work record shape/types")


def block_work(work):
    shape(work, BLOCK_FIELDS)
    require(work["gram_positive"]+work["gram_unresolved"] == work["preparations"] and
            work["center_axes_tightened"] <= 3*work["preparations"] and
            work["center_intersection_fallbacks"] <= work["gram_positive"], "block preparation ledger")
    require(work["bound_queries"] == sum(work[k] for k in ("inside_certificates", "outside_certificates", "unknown_bounds")) and
            work["axis_parabolas"] == work["vertex_clamps"] == 6*work["bound_queries"] and
            work["power_evaluations"] == 18*work["bound_queries"] and
            work["interval_divisions"] == 3*work["gram_positive"] and
            work["scalar_divisions"] == 2*work["interval_divisions"], "block query/division ledger")


def validate_work(work, query, seeds_count):
    require(type(work) is dict and set(work) == set(CENSUS_FIELDS) | {"supports", "power", "shared_bounds", "individual_bounds"},
            "census work fields")
    shape({k: work[k] for k in CENSUS_FIELDS}, CENSUS_FIELDS)
    for name in ("shared_bounds", "individual_bounds"):
        block_work(work[name])
    for name in ("supports", "power"):
        shape(work[name], BALL_FIELDS)
    w = work
    require(w["calls"] == 1 and w["input_seed_slots"] == seeds_count and
            w["input_seed_slots"] == w["relayed_seed_slots"]+w["shared_rejected_seed_slots"] and
            w["relayed_seed_slots"] == w["endpoint_seeds"]+w["support_candidates"] and
            w["support_candidates"] == w["invalid_supports"]+w["valid_supports"] and
            w["valid_supports"] == w["accepted_supports"]+w["saturated_supports"] and
            w["callbacks"] == w["accepted_supports"], "census seed/output conservation")
    require(w["endpoint_seeds"] <= 2 and w["count_point_tests"] <= w["count_node_visits"] and
            w["shell_point_tests"] <= w["shell_node_visits"] and
            w["count_node_visits"] == w["count_inside_nodes"]+w["count_outside_nodes"]+w["count_splits"] and
            w["shell_node_visits"] == w["shell_excluded_nodes"]+w["shell_splits"]+w["shell_ids"] and
            3*w["callbacks"] <= w["shell_ids"] and w["relays_with_credit"] <= w["valid_supports"] and
            w["relays_at_eof"] == 0, "census visit ledger or contact cursor invariant")
    threshold = query["kmax"]-1
    require(w["count_inside_sites"] <= threshold*w["valid_supports"] and
            w["shared_inside_sites"] <= threshold*w["shared_frames"] and
            w["shared_children_with_credit"] <= 2*w["shared_splits"] and w["shared_children_with_credit"] % 2 == 0,
            "census saturation/ticket ledger")
    shared, individual = w["shared_bounds"], w["individual_bounds"]
    require(individual["preparations"] == w["valid_supports"] and
            individual["bound_queries"] == w["count_node_visits"]-w["count_point_tests"]+w["shell_node_visits"]-w["shell_point_tests"] and
            shared["bound_queries"] == w["shared_witness_visits"]-w["shared_endpoint_skips"] and
            shared["inside_certificates"] == w["shared_inside_nodes"] and
            shared["outside_certificates"] == w["shared_outside_nodes"] and
            shared["unknown_bounds"] == w["shared_witness_splits"]+w["shared_splits"], "census/block work correspondence")
    if query["mode"] == 0:
        require(w["relay_blocks"] == 1 and all(w[k] == 0 for k in CENSUS_FIELDS if k.startswith("shared_")) and
                all(v == 0 for v in shared.values()) and
                w["relays_with_credit"] == w["peak_pending_frames"] == w["stack_capacity_bytes"] == 0,
                "individual path performed shared work")
    else:
        require(w["shared_frames"] == 1+2*w["shared_splits"] and
                w["shared_frames"] == w["relay_blocks"]+w["shared_saturated_blocks"]+w["shared_splits"] and
                shared["preparations"] <= w["shared_frames"] and w["peak_pending_frames"] > 0 and
                w["stack_capacity_bytes"] > 0, "shared frame traversal ledger")
    supports, powers = w["supports"], w["power"]
    candidates = w["support_candidates"]
    require(supports["q3_preparations"] == candidates and supports["q4_preparations"] == 0 and
            supports["preparation_filter_attempts"] == candidates and
            supports["preparation_filter_accepts"]+supports["preparation_exact_fallbacks"] == candidates and
            supports["preparation_exact_evaluations"] == supports["preparation_exact_fallbacks"] and
            supports["accepted_supports"] == w["valid_supports"] and supports["rejected_supports"] == w["invalid_supports"] and
            supports["exact_point_decodes"] == 3*supports["preparation_exact_fallbacks"] and
            all(supports[k] == 0 for k in BALL_FIELDS if k.startswith("power_")), "census positive-support ledger")
    queries = w["count_point_tests"]+w["shell_point_tests"]
    require(powers["power_queries"] == queries and powers["power_filter_attempts"] == queries and
            powers["power_filter_accepts"]+powers["power_exact_fallbacks"] == queries and
            powers["power_exact_evaluations"] == powers["power_exact_fallbacks"] and
            powers["exact_point_decodes"] == 4*powers["power_exact_fallbacks"] and
            all(powers[k] == 0 for k in BALL_FIELDS if k.startswith("preparation_") or k in
                ("q3_preparations", "q4_preparations", "accepted_supports", "rejected_supports")), "census exact point-query ledger")


def validate_row(row, case):
    require(type(row) is dict and set(row) == {"schema", "status", "points", "permutation", "nodes", "queries"} and
            row["schema"] == "mhgp8_float32_q3_census_probe_v1" and row["status"] == "passed", "census result schema")
    order, nodes = validate_tree(row, case)
    require(type(row["queries"]) is list and len(row["queries"]) == len(case["requests"]), "census query count")
    accepted, shell_ids, max_shell = 0, 0, 0
    # Same geometry/node/K in different modes/grains uses one independent judge.
    cache = {}
    for result, request in zip(row["queries"], case["requests"], strict=True):
        require(type(result) is dict and set(result) == {"request", "emissions", "work"} and
                type(result["request"]) is dict and set(result["request"]) == set(request) and
                all(type(v) is int for v in result["request"].values()) and result["request"] == request,
                "census request/result mismatch")
        node = nodes[request["node"]]
        seeds = order[node["first"]:node["last"]]
        key = tuple(request[k] for k in ("a", "b", "node", "kmax"))
        if key not in cache:
            cache[key] = scalar_census(case, request, seeds)
        expected, invalid, saturated = cache[key]
        require(type(result["emissions"]) is list, "census payload list")
        actual = {}
        for emission in result["emissions"]:
            require(type(emission) is dict and set(emission) == {"seed", "depth", "shell"} and
                    type(emission["seed"]) is int and type(emission["depth"]) is int and
                    emission["seed"] not in actual and type(emission["shell"]) is list and
                    all(type(i) is int and 0 <= i < len(case["points"]) for i in emission["shell"]) and
                    len(emission["shell"]) == len(set(emission["shell"])), "census emission duplicates/shape")
            actual[emission["seed"]] = dict(emission, shell=sorted(emission["shell"]))
        require(actual == expected, "census differs from global Fraction depth/complete-shell oracle")
        work = result["work"]
        validate_work(work, request, len(seeds))
        require(work["callbacks"] == len(expected) and work["shell_ids"] == sum(len(e["shell"]) for e in expected.values()),
                "census output counters differ from emitted payload")
        if request["mode"] == 0:
            require(work["invalid_supports"] == invalid and work["saturated_supports"] == saturated and
                    work["endpoint_seeds"] == sum(i in seeds for i in (request["a"], request["b"])), "individual support oracle ledger")
        accepted += len(expected)
        shell_ids += sum(len(e["shell"]) for e in expected.values())
        max_shell = max(max_shell, *(len(e["shell"]) for e in expected.values()), 0)
    return dict(accepted_supports=accepted, shell_ids=shell_ids, max_shell=max_shell)


def validate_rows(rows, cases):
    require(type(rows) is list and len(rows) == len(cases), "census fixture count")
    stats = [validate_row(row, case) for row, case in zip(rows, cases, strict=True)]
    fields = ("shared_splits", "shared_inside_nodes", "shared_outside_nodes", "shared_saturated_blocks",
              "shared_children_with_credit", "relays_with_credit", "invalid_supports", "saturated_supports",
              "count_inside_nodes", "count_splits", "shell_splits", "shell_excluded_nodes")
    coverage = {field: sum(q["work"][field] for row in rows for q in row["queries"]) for field in fields}
    require(all(v > 0 for v in coverage.values()), "required census branch was never positively exercised")
    require(max(s["max_shell"] for s in stats) == 30, "complete non-simplicial cosphere shell not exercised")
    return dict(fixtures=len(cases), queries=sum(len(c["requests"]) for c in cases),
                accepted_supports=sum(s["accepted_supports"] for s in stats), shell_ids=sum(s["shell_ids"] for s in stats),
                max_shell=max(s["max_shell"] for s in stats), coverage=coverage)


def mix(value):
    value = (value+0x9e3779b97f4a7c15) & MAXIMUM_K
    value = ((value ^ (value >> 30))*0xbf58476d1ce4e5b9) & MAXIMUM_K
    value = ((value ^ (value >> 27))*0x94d049bb133111eb) & MAXIMUM_K
    return value ^ (value >> 31)


def rotate(value, shift):
    return ((value << shift) | (value >> (64-shift))) & MAXIMUM_K


def digest(emissions):
    result = dict(callbacks=0, shell_ids=0, depth_sum=0, digest_sum=0, digest_xor=0)
    for e in emissions:
        total, parity = 0, 0
        for site in e["shell"]:
            total = (total+mix(site)) & MAXIMUM_K
            parity ^= mix(site)
        value = mix(e["seed"]) ^ rotate(mix(e["depth"]), 7) ^ rotate(total, 19) ^ rotate(parity, 31) ^ mix(len(e["shell"]))
        result["callbacks"] += 1
        result["shell_ids"] += len(e["shell"])
        result["depth_sum"] += e["depth"]
        result["digest_sum"] = (result["digest_sum"]+value) & MAXIMUM_K
        result["digest_xor"] ^= value
    return result


def validate_bench(row, n, regime, mode, kmax, relay_sites):
    require(type(row) is dict and set(row) == {"schema", "status", "n", "regime", "mode", "kmax", "relay_sites",
            "construction_ms", "census_ms", "index_work", "index_bytes", "work", "payload"} and
            row["schema"] == "mhgp8_float32_q3_census_bench_v1" and row["status"] == "passed" and
            (row["n"], row["regime"], row["mode"], row["kmax"], row["relay_sites"]) == (n, regime, mode, kmax, relay_sites) and
            regime in ("column", "slab") and n >= 3 and mode in (0, 1) and kmax >= 2 and relay_sites > 0,
            "census benchmark schema/command")
    require(all(type(row[k]) is int for k in ("n", "mode", "kmax", "relay_sites", "index_bytes")) and row["index_bytes"] > 0 and
            all(type(row[k]) in (int, float) and math.isfinite(row[k]) and row[k] >= 0 for k in ("construction_ms", "census_ms")),
            "census benchmark timing/size types")
    shape(row["index_work"], INDEX_FIELDS)
    iw = row["index_work"]
    require(all(iw[k] == n for k in ("input_word_triples_copied", "finite_points_validated", "point_objects_constructed", "inverse_rank_writes", "leaves")) and
            iw["nodes"] == 2*n-1 and iw["duplicate_adjacent_tests"] == n-1, "benchmark index coverage")
    request = dict(a=0, b=1, node=0, kmax=kmax, mode=mode, grain=relay_sites)
    validate_work(row["work"], request, n)
    p = row["payload"]
    shape(p, ("callbacks", "shell_ids", "depth_sum", "digest_sum", "digest_xor"))
    require(p["callbacks"] == row["work"]["callbacks"] and p["shell_ids"] == row["work"]["shell_ids"] and
            p["depth_sum"] <= p["callbacks"]*(kmax-2), "benchmark payload ledger")
    if regime == "column":
        # y_j>0: power_i(y_j)=(y_j-y_i)(y_j+1/y_i); depth_i=i.
        count = min(n-2, kmax-1)
        expected = digest(dict(seed=i+2, depth=i, shell=[0, 1, i+2]) for i in range(count))
        require(p == expected, "column benchmark differs from analytic all-site oracle")
    return row


def invalid_inputs():
    base = fixtures()[0]
    result = [("empty", b""), ("missing_header", b"3\n"), ("negative_size", b"-1 0\n"),
              ("empty_cloud", b"0 0\n"), ("truncated", payload(base)[:-5]), ("trailing", payload(base)+b"unexpected\n")]
    for field, value in (("a", 3), ("b", 3), ("b", 0), ("node", 5), ("kmax", 0), ("kmax", 1), ("mode", 2), ("grain", 0)):
        case = deepcopy(base)
        case["requests"] = [dict(base["requests"][0], **{field: value})]
        result.append((f"invalid_{field}_{value}", payload(case)))
    for field in ("a", "b", "node", "kmax", "mode", "grain"):
        case = deepcopy(base)
        case["requests"] = [dict(base["requests"][0], **{field: -1})]
        result.append(("negative_"+field, payload(case)))
    for word in (0x7f800000, 0xff800000, 0x7fc00000, 0x7f800001):
        for axis in range(3):
            case = deepcopy(base)
            points = [list(p) for p in base["points"]]
            points[2][axis] = word
            case["points"] = points
            result.append(("nonfinite", payload(case)))
    for changed in (((0, 0, 0), (0, 0, 0), base["points"][2]),
                    ((0, 0, 0), (0x80000000, 0, 0), base["points"][2])):
        case = dict(base, points=changed)
        result.append(("duplicate_geometric_site", payload(case)))
    result.extend((("short_hex", payload(base).replace(b"bf800000", b"bf80000", 1)),
                   ("bad_hex", payload(base).replace(b"bf800000", b"gf800000", 1)),
                   ("size_overflow", b"18446744073709551616 0\n")))
    return result


def check_selftest(row):
    require(type(row) is dict and set(row) == {"schema", "status", "tests", "callbacks_checked", "invalid_arguments",
            "callback_exceptions", "lifetime_checks", "concurrent_workers", "concurrent_calls", "rounding_modes", "flush_modes",
            "environment_calls", "bound_checks"} and row["schema"] == "mhgp8_float32_q3_census_selftest_v1" and row["status"] == "passed" and
            all(type(v) is int for k, v in row.items() if k not in ("schema", "status")) and row["flush_modes"] in (1, 4) and
            row["tests"] == 42+104*row["flush_modes"] and row["callbacks_checked"] == 7 and row["invalid_arguments"] == 10 and
            row["callback_exceptions"] == 1 and row["lifetime_checks"] == 2 and row["concurrent_workers"] == 4 and row["concurrent_calls"] == 32 and
            row["rounding_modes"] == 4 and row["environment_calls"] == 24*row["flush_modes"] and row["bound_checks"] == 56*row["flush_modes"],
            "census selftest inventory")


def execute(binary, output, name, data=None, arguments=(), expected_exit=0):
    if data is not None:
        (output/(name+".input")).write_bytes(data)
    record = dict(command=[str(binary), *arguments], exit_code=None)
    try:
        with (output/(name+".stdout")).open("xb") as stdout, (output/(name+".stderr")).open("xb") as stderr:
            child = subprocess.run(record["command"], input=data, stdout=stdout, stderr=stderr, check=False)
        record["exit_code"] = child.returncode
    except BaseException as error:
        record["error"] = f"{type(error).__name__}: {error}"
        raise
    finally:
        (output/(name+".json")).write_text(json.dumps(record, sort_keys=True)+"\n")
    require(record["exit_code"] == expected_exit, "census native exit differs; streams retained: "+name)


def read_command(output, binary, name, data=None, arguments=(), expected_exit=0):
    if data is not None:
        require((output/(name+".input")).read_bytes() == data, "census transcript input differs: "+name)
    record = strict_json((output/(name+".json")).read_bytes())
    require(type(record) is dict and set(record) == {"command", "exit_code"} and record["command"] == [str(binary), *arguments] and
            type(record["exit_code"]) is int and record["exit_code"] == expected_exit, "census command/exit mismatch")
    stdout, stderr = ((output/(name+"."+stream)).read_bytes() for stream in ("stdout", "stderr"))
    if expected_exit:
        require(not stdout and stderr.startswith(b"float32 q3 census probe: "), "census rejection diagnostic missing")
    else:
        require(not stderr, "census native success wrote stderr")
    return stdout


def read_transcript(output, binary):
    cases, bad = fixtures(), invalid_inputs()
    names = {f"case_{i:03}.{suffix}" for i in range(len(cases)) for suffix in ("input", "stdout", "stderr", "json")}
    names.update(f"reject_{i:03}.{suffix}" for i in range(len(bad)) for suffix in ("input", "stdout", "stderr", "json"))
    names.update("selftest."+suffix for suffix in ("stdout", "stderr", "json"))
    require(output.is_dir() and not output.is_symlink() and {p.name for p in output.iterdir()} == names and
            all(p.is_file() and not p.is_symlink() for p in output.iterdir()), "census transcript inventory/shape")
    before = {name: sha(output/name) for name in sorted(names)}
    rows = [strict_json(read_command(output, binary, f"case_{i:03}", payload(case))) for i, case in enumerate(cases)]
    stats = validate_rows(rows, cases)
    for i, (_, data) in enumerate(bad):
        read_command(output, binary, f"reject_{i:03}", data, expected_exit=1)
    selftest = strict_json(read_command(output, binary, "selftest", arguments=("--selftest",)))
    check_selftest(selftest)
    require({name: sha(output/name) for name in sorted(names)} == before, "census transcript changed while reading")
    digest_hash = hashlib.sha256(json.dumps(before, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    return dict(**stats, selftest=selftest, invalid_inputs=len(bad), transcript_sha256=digest_hash)


def mutations(rows, cases):
    changes = [lambda row: row.__setitem__("status", "failed"),
               lambda row: row["points"][0].__setitem__(0, 0),
               lambda row: row["permutation"].__setitem__(0, row["permutation"][1]),
               lambda row: row["nodes"][0].__setitem__("escape", 1),
               lambda row: row["nodes"][0]["low"].__setitem__(0, 0),
               lambda row: row["queries"].pop(),
               lambda row: row["queries"][0]["request"].__setitem__("kmax", 5),
               lambda row: row["queries"][0]["emissions"].pop(),
               lambda row: row["queries"][0]["emissions"][0].__setitem__("depth", 1),
               lambda row: row["queries"][0]["emissions"][0]["shell"].pop(),
               lambda row: row["queries"][0]["emissions"].append(deepcopy(row["queries"][0]["emissions"][0])),
               lambda row: row["queries"][0]["work"].__setitem__("callbacks", True),
               lambda row: row["queries"][0]["work"].__setitem__("input_seed_slots", 1),
               lambda row: row["queries"][0]["work"]["individual_bounds"].__setitem__("bound_queries", 0),
               lambda row: row["queries"][0]["work"]["power"].__setitem__("power_queries", 0),
               lambda row: row["queries"][0]["work"]["supports"].__setitem__("q3_preparations", 0)]
    rejected = 0
    for change in changes:
        row = deepcopy(rows[0])
        change(row)
        try:
            validate_row(row, cases[0])
        except ValueError:
            rejected += 1
    require(rejected == len(changes), "census output mutation survived")
    return rejected


def run(binary, output):
    sources, binary_hash = {str(p): sha(p) for p in SOURCES}, sha(binary)
    output.mkdir(parents=True, exist_ok=False)
    cases = fixtures()
    for i, case in enumerate(cases):
        execute(binary, output, f"case_{i:03}", payload(case))
    for i, (_, data) in enumerate(invalid_inputs()):
        execute(binary, output, f"reject_{i:03}", data, expected_exit=1)
    execute(binary, output, "selftest", arguments=("--selftest",))
    result = read_transcript(output, binary)
    rows = [strict_json((output/f"case_{i:03}.stdout").read_bytes()) for i in range(len(cases))]
    rejected = mutations(rows, cases)
    require(sha(binary) == binary_hash and {str(p): sha(p) for p in SOURCES} == sources, "census source/binary closure changed")
    return dict(schema="mhgp8_float32_q3_census_gate_v1", status="passed", **result, mutation_checks=rejected,
                binary=str(binary), binary_sha256=binary_hash, source_sha256=sources, gate_sha256=sha(Path(__file__)),
                optimized=bool(sys.flags.optimize), scope="one_supplied_edge_and_seed_subtree_not_generator_or_FULL")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(run(args.binary.resolve(strict=True), args.output.resolve()), sort_keys=True))


if __name__ == "__main__":
    main()
