#!/usr/bin/env python3
"""Bounded independent Fraction judge of the complete native q3 support stream.

The exact binary32 decode, Gram solver and canonical-key encoding are explicitly
reused from float32_identity_gate.py and pinned with this oracle. All unordered
triples and all witness sites are enumerated here, never through a native WSPD,
block bound, ownership predicate, or census. This is not a FULL/GPU gate.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
from fractions import Fraction
from functools import lru_cache
import hashlib
import importlib.util
from itertools import combinations, permutations, product
import json
from pathlib import Path
import random
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
IDENTITY = ROOT / "morsehgp3D_v8/tests/float32_identity_gate.py"
_spec = importlib.util.spec_from_file_location("global_q3_fraction_authority", IDENTITY)
fraction_oracle = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(fraction_oracle)
MAXIMUM_K = (1 << 64) - 1
ORACLE_SOURCES = (Path(__file__).resolve(), IDENTITY)
SCHEMA = "mhgp8_float32_q3_global_probe_v1"
GLOBAL_FIELDS = "calls rectangles edges edge_filter_calls witness_node_visits witness_inside_nodes witness_inside_sites witness_outside_nodes witness_splits rejected_edges emitted shell_ids peak_key_bytes".split()
EDGE_FIELDS = "preparations owner_queries owner_filter_accepts owner_exact_queries owner_equalities owner_rejections owner_box_queries owner_box_rejections citron_box_queries citron_inside citron_outside citron_unknown citron_point_queries citron_filter_accepts citron_exact_queries separation_queries separation_filter_accepts separation_exact_queries interval_additions interval_products exact_additions exact_products exact_decodes".split()
FRONT_FIELDS = "queries total_unordered_pairs product_visits diagonal_splits diagonal_leaves disjoint_splits witness_searches witness_descent_steps witness_box_tests proposed_sites proposals_in_factors witness_credits rejected_products rejected_pairs emitted_rectangles residual_pairs emitted_factor_sites leaf_pair_rectangles max_stack_size max_product_depth peak_stack_bytes".split()
OWNED_FIELDS = "calls input_seed_slots seed_node_visits seed_rejected_nodes seed_rejected_slots seed_splits shared_frames shared_witness_visits shared_endpoint_skips shared_inside_nodes shared_inside_sites shared_outside_nodes shared_witness_splits shared_saturated_blocks shared_rejected_seed_slots shared_children_with_credit relay_blocks relayed_seed_slots endpoint_seeds owner_candidates owner_rejections owned_seeds invalid_supports valid_supports relays_with_credit relays_at_eof count_node_visits count_point_tests count_inside_nodes count_inside_sites count_outside_nodes count_splits saturated_supports accepted_supports shell_node_visits shell_point_tests shell_excluded_nodes shell_splits shell_ids callbacks stack_reserves shell_growths peak_pending_frames stack_capacity_bytes peak_shell_capacity_bytes peak_workspace_bytes".split()
BLOCK_FIELDS = "edge_preparations edge_squared_positive edge_squared_unresolved block_preparations projected_envelopes gram_positive gram_unresolved xi_tightened xi_intersection_fallbacks center_axes_tightened center_intersection_fallbacks bound_queries inside_certificates outside_certificates unknown_bounds axis_parabolas vertex_clamps power_evaluations interval_additions interval_products interval_divisions scalar_divisions".split()
BALL_FIELDS = fraction_oracle.BALL_FIELDS
KEY_FIELDS = fraction_oracle.KEY_FIELDS


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def strict_json(data):
    return fraction_oracle.strict_json(data)


def bits(value):
    return fraction_oracle.bits(value)


def encoded(points):
    return fraction_oracle.encoded(points)


def distance2(a, b):
    return sum((x-y)**2 for x, y in zip(a, b, strict=True))


def owner_of(support, coordinates):
    """Smallest ID pair among the edges with greatest exact squared length."""
    edges = tuple(combinations(support, 2))
    largest = max(distance2(coordinates[a], coordinates[b]) for a, b in edges)
    return min(edge for edge in edges
               if distance2(coordinates[edge[0]], coordinates[edge[1]]) == largest)


@lru_cache(maxsize=None)
def all_records(points):
    coordinates = tuple(map(fraction_oracle.xyz, points))
    require(len(set(coordinates)) == len(points), "oracle refuses duplicate geometry")
    output = []
    for support in combinations(range(len(points)), 3):
        words = tuple(points[i] for i in support)
        sphere = fraction_oracle.circumball(words)
        if sphere is None:
            continue
        centre, radius2 = sphere
        depth = 0
        shell = []
        for site, point in enumerate(coordinates):
            power = distance2(point, centre) - radius2
            depth += power < 0
            if power == 0:
                shell.append(site)
        key = tuple(fraction_oracle.pack(fraction_oracle.key_oracle(words)))
        output.append((support, owner_of(support, coordinates), depth, tuple(shell), key))
    return tuple(output)


def expected_records(points, kmax):
    return tuple(record for record in all_records(tuple(map(tuple, points)))
                 if record[2] < kmax - 1)


def record_json(record):
    support, owner, depth, shell, key = record
    return dict(support=list(support), owner=list(owner), depth=depth,
                shell=list(shell), key=list(key))


def fixtures():
    result = []

    def add(name, points):
        points = tuple(tuple(point) for point in points)
        coordinates = tuple(map(fraction_oracle.xyz, points))
        require(len(set(coordinates)) == len(points), "fixture geometry duplicated: " + name)
        result.append(dict(name=name, points=points))

    add("empty", ())
    add("singleton_signed_zero", ((0x80000000, 0, bits(3)),))
    add("pair", encoded(((0, 0, 0), (10, 0, 0))))
    add("one_acute_triangle", encoded(((0, 0, 0), (10, 0, 0), (5, 8, 0))))
    add("collinear", encoded(((-2, 0, 0), (-1, 0, 0), (0, 0, 0), (1, 0, 0), (2, 0, 0))))
    add("B_F1_vertex_minimum", encoded(((0, 0, 0), (10, 0, 0), (5, 6, 0), (5, 0, 0))))
    axial = [(20, 20, 20), (40, 20, 20)] + [(30, y, 20) for y in range(32, 38)] + [(30, 31, 20)]
    add("B_F2_axial_shared_witness", encoded(axial))
    rotated = [(2*x-2*y+z+500, x+2*y+2*z+500, -2*x-y+2*z+500) for x, y, z in axial]
    add("B_F3_rotated", encoded(rotated))
    add("B_F4_nonowned_lambda_near_one", encoded(((0, 60, 60), (2, 60, 60), (1, 9, 8), (1, 60, 60))))
    add("B_F5_unresolved_gram", encoded(((20, 20, 20), (40, 20, 20), (30, 20, 20),
        (30, 21, 20), (30, 30, 20), (30, 36, 20), (30, 40, 20))))
    add("B_F6_seed_is_other_seed_witness", encoded(((0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 20, 0))))
    add("B_F7_right_obtuse_and_acute", encoded(((0, 0, 0), (10, 0, 0), (2, 4, 0),
        (0, 3, 0), (5, 2, 0), (12, 3, 0), (5, 8, 0))))
    f9 = encoded(((0, 0, 0), (10, 0, 0), (5, 8, 0), (9, 5, 0), (5, 1, 0)))
    add("B_F9_invalid_and_nonowned_remain_witnesses", f9)
    f10 = encoded(((0, 0, 0), (10, 0, 0), (6, 8, 0)))
    for index, order in enumerate(permutations(range(3))):
        add("B_F10_equal_max_edges_ids_" + str(index), tuple(f10[i] for i in order))
    add("three_equal_edges", encoded(((0, 0, 0), (2, 2, 0), (2, 0, 2))))
    add("prefix_contacts_and_strict_neighbours", encoded(((5, 0, 0), (-3, 4, 0), (-3, -4, 0),
        (0, 0, 5), (0, 0, -5), (3, 0, 4), (0, 0, 0))) +
        ((0, 0, bits(5)-1), (0, 0, bits(5)+1)))
    sphere = sorted(point for point in product(range(-5, 6), repeat=3)
                    if sum(x*x for x in point) == 25)
    require(len(sphere) == 30, "complete shell30 fixture")
    add("cosphere30", encoded(sphere))
    add("cosphere30_plus_strict_interior", encoded(sphere + [(0, 0, 0)]))
    add("signed_zero_geometry", tuple(tuple(word if word else 0x80000000 for word in point) for point in f9))
    for exponent in (-149, -120, 60, 120):
        original = encoded(((0, 0, 0), (4, 0, 0), (2, 3, 1), (2, 1, 0)))
        transformed = tuple(tuple(bits(float(fraction_oracle.rational(word))*2.0**exponent)
                                  for word in point) for point in original)
        add("scale_" + str(exponent), transformed)
    huge = 0x7f7fffff
    add("float_extremes_and_cancellation", ((huge, 0, 0), (huge | 0x80000000, 0, 0),
        (0, huge, 1), (0, huge, 0), (0, huge | 0x80000000, 1), (0, 0, 0), (1, 1, 1)))
    add("normal_subnormal_transition", ((0x00800000, 0, 0), (0x80800000, 0, 0),
        (0, 0x00800000, 1), (0, 0x007fffff, 0), (0, 0, 0), (1, 0, 0)))
    rng = random.Random(0xF32A1103)
    for number, n in enumerate((5, 7, 9, 12, 15, 18)):
        points = []
        while len(points) < n:
            point = tuple(rng.randint(-16, 16)/4 for _ in range(3))
            if point not in points:
                points.append(point)
        add("random_3d_" + str(number), encoded(points))
    # Each disjoint acute triple contributes an empty small circumball. This
    # exercises growing output, not the constant K-1 output of nested columns.
    for groups in (2, 4, 8):
        points = [(64*group+x, y, z) for group in range(groups)
                  for x, y, z in ((0, 0, 0), (4, 0, 0), (2, 3, 1))]
        add("growing_3d_groups_" + str(groups), encoded(points))
    return result


def options(kmax=5, separation=8, mode="shared", filter_mode="on", relay_sites=4):
    return dict(kmax=kmax, separation=separation, mode=mode,
                filter=filter_mode, relay_sites=relay_sites)


def cases():
    output = []
    for fixture in fixtures():
        configurations = [options(kmax=k, mode=mode, filter_mode=filter_mode, relay_sites=relay)
                          for k in (1, 2, 5, 10, MAXIMUM_K)
                          for mode, filter_mode, relay in (("individual", "off", 1), ("shared", "on", 4))]
        configurations += [options(separation=10, mode="individual", filter_mode="on", relay_sites=1),
                           options(separation=12, mode="shared", filter_mode="off", relay_sites=3),
                           options(separation=1, relay_sites=1), options(relay_sites=1), options(relay_sites=64)]
        for opt in configurations:
            output.append(dict(fixture, options=opt))
    # All eight native rounding/FTZ combinations on four distinct hazards.
    # Each pair of shared and individual traversals is judged independently.
    selected = {"B_F9_invalid_and_nonowned_remain_witnesses", "B_F10_equal_max_edges_ids_0",
                "scale_-149", "float_extremes_and_cancellation"}
    for fixture in fixtures():
        if fixture["name"] not in selected:
            continue
        for rounding, ftz, mode in product(("nearest", "down", "up", "zero"), (0, 1), ("individual", "shared")):
            output.append(dict(fixture, options=options(mode=mode, relay_sites=1), environment=(rounding, ftz)))
    return output


def arguments(case):
    opt = case["options"]
    prefix = ["--case-env", case["environment"][0], str(case["environment"][1])] if "environment" in case else ["--case"]
    return [*prefix, str(opt["kmax"]), str(opt["separation"]), opt["mode"], opt["filter"], str(opt["relay_sites"])]


def payload(case):
    return (str(len(case["points"])) + "\n" + "".join(
        " ".join(f"{word:08x}" for word in point) + "\n" for point in case["points"])).encode()


def numeric_work(value):
    require(type(value) is dict, "work must be an object")
    for counter in value.values():
        if type(counter) is dict:
            numeric_work(counter)
        else:
            require(type(counter) is int and 0 <= counter < 2**64, "work counter type/range")


def validate_work(work, records, case):
    numeric_work(work)
    require(set(work) == set(GLOBAL_FIELDS) | {"front", "edge_filter", "owned", "keys"}, "global work schema")
    require(set(work["front"]) == set(FRONT_FIELDS) | {"geometry"} and
            set(work["owned"]) == set(OWNED_FIELDS) | {"selection", "shared_bounds", "individual_bounds", "supports", "power"} and
            set(work["keys"]) == set(KEY_FIELDS) | {"support"}, "nested work schema")
    for value in (work["edge_filter"], work["front"]["geometry"], work["owned"]["selection"]):
        require(set(value) == set(EDGE_FIELDS), "edge work schema")
    for value in (work["owned"]["shared_bounds"], work["owned"]["individual_bounds"]):
        require(set(value) == set(BLOCK_FIELDS), "block work schema")
    for value in (work["owned"]["supports"], work["owned"]["power"], work["keys"]["support"]):
        require(set(value) == set(BALL_FIELDS), "ball work schema")
    require(work["emitted"] == len(records) and work["shell_ids"] == sum(len(record[3]) for record in records),
            "global callback/shell ledger differs from payload")
    keys = work["keys"]
    require(keys["from_support_requests"] == keys["keys_created"] == len(records) and
            keys["packed_words"] == sum(len(record[4]) for record in records), "global key creation/word ledger")
    front, owned = work["front"], work["owned"]
    require(work["calls"] == int(bool(case["points"])) and work["edges"] == front["residual_pairs"] and
            work["rectangles"] == front["emitted_rectangles"] and
            work["edges"] == owned["calls"] + work["rejected_edges"], "global/front/owned transfer ledger")
    require(owned["callbacks"] == owned["accepted_supports"] == work["emitted"] and
            owned["shell_ids"] == work["shell_ids"], "owned/global output ledger")
    require(owned["input_seed_slots"] == owned["calls"] * len(case["points"]) ==
            owned["seed_rejected_slots"] + owned["shared_rejected_seed_slots"] + owned["relayed_seed_slots"],
            "owned disjoint seed-slot partition")
    require(owned["relayed_seed_slots"] == owned["endpoint_seeds"] + owned["owner_candidates"] and
            owned["owner_candidates"] == owned["owner_rejections"] + owned["owned_seeds"] and
            owned["owned_seeds"] == owned["invalid_supports"] + owned["valid_supports"] and
            owned["valid_supports"] == owned["saturated_supports"] + owned["accepted_supports"], "owned seed/census partition")
    active = len(case["points"]) >= 3 and case["options"]["kmax"] > 1
    require(front["queries"] == int(active) and front["total_unordered_pairs"] ==
            (len(case["points"]) * (len(case["points"])-1)//2 if active else 0) and
            front["total_unordered_pairs"] == front["rejected_pairs"] + front["residual_pairs"], "front complete pair partition")
    require(work["edge_filter_calls"] == (work["edges"] if case["options"]["filter"] == "on" else 0), "edge filter calls")
    require(owned["stack_reserves"] <= 1, "global workspace reallocated stack per edge")
    if not case["points"]:
        def zero(value):
            return all(zero(item) if type(item) is dict else item == 0 for item in value.values())
        require(zero(work), "empty CLI path performed global work")


def validate_row(row, case):
    require(type(row) is dict and set(row) == {"schema", "kind", "options", "n", "records", "work"} and
            row["schema"] == SCHEMA and row["kind"] == "case", "global q3 result schema")
    require(type(row["options"]) is dict and row["options"] == case["options"] and
            all(type(row["options"][field]) is int for field in ("kmax", "separation", "relay_sites")) and
            type(row["n"]) is int and row["n"] == len(case["points"]), "global q3 command/options/input mismatch")
    require(type(row["records"]) is list, "global records must be a list")
    records = []
    seen = set()
    for record in row["records"]:
        require(type(record) is dict and set(record) == {"support", "owner", "depth", "shell", "key"}, "record shape")
        for field, size in (("support", 3), ("owner", 2), ("shell", None)):
            ids = record[field]
            require(type(ids) is list and (size is None or len(ids) == size) and
                    all(type(site) is int and 0 <= site < row["n"] for site in ids) and
                    ids == sorted(set(ids)), "record sorted unique IDs: " + field)
        require(set(record["owner"]) <= set(record["support"]) <= set(record["shell"]), "owner/support/global shell incidence")
        require(type(record["depth"]) is int and 0 <= record["depth"] < case["options"]["kmax"] - 1,
                "record strict interior threshold")
        require(type(record["key"]) is list and record["key"] and
                all(type(word) is int and 0 <= word < 2**32 for word in record["key"]), "key word shape")
        support = tuple(record["support"])
        require(support not in seen, "duplicate support emitted")
        seen.add(support)
        records.append((support, tuple(record["owner"]), record["depth"], tuple(record["shell"]), tuple(record["key"])))
    records.sort()
    expected = expected_records(case["points"], case["options"]["kmax"])
    require(tuple(records) == expected, "global support/owner/depth/complete-shell/key differs from exhaustive Fraction oracle")
    validate_work(row["work"], records, case)
    return dict(records=len(records), shell_ids=sum(len(record[3]) for record in records),
                key_words=sum(len(record[4]) for record in records), max_shell=max((len(record[3]) for record in records), default=0))


def synthetic_row(case):
    records = expected_records(case["points"], case["options"]["kmax"])
    zeros = lambda fields: dict.fromkeys(fields, 0)
    work = zeros(GLOBAL_FIELDS)
    work["front"] = dict(zeros(FRONT_FIELDS), geometry=zeros(EDGE_FIELDS))
    work["edge_filter"] = zeros(EDGE_FIELDS)
    work["owned"] = dict(zeros(OWNED_FIELDS), selection=zeros(EDGE_FIELDS), shared_bounds=zeros(BLOCK_FIELDS),
                         individual_bounds=zeros(BLOCK_FIELDS), supports=zeros(BALL_FIELDS), power=zeros(BALL_FIELDS))
    work["keys"] = dict(zeros(KEY_FIELDS), support=zeros(BALL_FIELDS))
    # Synthetic counter values exist only to unit-test the result judge;
    # native captures below must pass the same identities on actual counters.
    require(len(case["points"]) == 3 and len(records) == 1, "synthetic mutation fixture changed")
    work.update(calls=1, rectangles=1, edges=1, edge_filter_calls=1, emitted=1, shell_ids=3)
    work["front"].update(queries=1, total_unordered_pairs=3, rejected_pairs=2, residual_pairs=1, emitted_rectangles=1)
    work["owned"].update(calls=1, input_seed_slots=3, relayed_seed_slots=3, endpoint_seeds=2, owner_candidates=1,
                         owned_seeds=1, valid_supports=1, accepted_supports=1, callbacks=1, shell_ids=3)
    work["keys"].update(from_support_requests=1, keys_created=1, packed_words=len(records[0][4]))
    return dict(schema=SCHEMA, kind="case", options=deepcopy(case["options"]), n=len(case["points"]),
                records=[record_json(record) for record in records], work=work)


def mutations(row, case):
    require(row["records"], "mutation source must have output")
    changes = [lambda value: value.__setitem__("schema", "old_u16"),
               lambda value: value.__setitem__("kind", "bench"),
               lambda value: value.__setitem__("n", True),
               lambda value: value["options"].__setitem__("separation", 7),
               lambda value: value["records"].pop(),
               lambda value: value["records"].append(deepcopy(value["records"][0])),
               lambda value: value["records"][0]["support"].reverse(),
               lambda value: value["records"][0]["owner"].reverse(),
               lambda value: value["records"][0].__setitem__("owner", value["records"][0]["support"][1:]),
               lambda value: value["records"][0].__setitem__("depth", value["records"][0]["depth"] + 1),
               lambda value: value["records"][0]["shell"].pop(),
               lambda value: value["records"][0]["shell"].reverse(),
               lambda value: value["records"][0]["key"].__setitem__(0, 2),
               lambda value: value["records"][0]["key"].append(0),
               lambda value: value["records"][0]["key"].__setitem__(0, True),
               lambda value: value["work"].__setitem__("emitted", True),
               lambda value: value["work"].__setitem__("shell_ids", value["work"]["shell_ids"] + 1),
               lambda value: value["work"]["keys"].__setitem__("keys_created", 0),
               lambda value: value["work"]["keys"].__setitem__("packed_words", value["work"]["keys"]["packed_words"] + 1)]
    for index, change in enumerate(changes):
        changed = deepcopy(row)
        change(changed)
        try:
            validate_row(changed, case)
        except ValueError:
            continue
        raise ValueError("global result corruption survived: " + str(index))
    return len(changes)


def oracle_selftest():
    by_name = {fixture["name"]: fixture for fixture in fixtures()}
    f9 = by_name["B_F9_invalid_and_nonowned_remain_witnesses"]["points"]
    main = next(record for record in all_records(f9) if record[0] == (0, 1, 2))
    require(main[1] == (0, 1) and main[2] == 2 and main[3] == (0, 1, 2), "B F9 exact owner/depth/shell")
    centre, radius2 = fraction_oracle.circumball(f9[:3])
    require(distance2(fraction_oracle.xyz(f9[3]), centre) - radius2 == Fraction(-67, 8) and
            distance2(fraction_oracle.xyz(f9[4]), centre) - radius2 == Fraction(-231, 8), "B F9 witness powers")
    require(fraction_oracle.circumball((f9[0], f9[1], f9[3])) is not None and
            owner_of((0, 1, 3), tuple(map(fraction_oracle.xyz, f9))) == (0, 3) and
            fraction_oracle.circumball((f9[0], f9[1], f9[4])) is None, "B F9 nonowned and invalid witness classification")
    require(main not in expected_records(f9, 3) and main in expected_records(f9, 4), "strict depth boundary K-1")
    for index in range(6):
        points = by_name["B_F10_equal_max_edges_ids_" + str(index)]["points"]
        record, = all_records(points)
        coordinates = tuple(map(fraction_oracle.xyz, points))
        maximum = max(distance2(coordinates[a], coordinates[b]) for a, b in combinations(range(3), 2))
        tied = [edge for edge in combinations(range(3), 2)
                if distance2(coordinates[edge[0]], coordinates[edge[1]]) == maximum]
        require(len(tied) == 2 and record[1] == min(tied), "B F10 exact maximal-distance/minlex ownership")
    growing = []
    for groups in (2, 4, 8):
        points = by_name["growing_3d_groups_" + str(groups)]["points"]
        records = expected_records(points, 2)
        supports = {record[0] for record in records}
        require(all((3*i, 3*i+1, 3*i+2) in supports for i in range(groups)), "independent 3D empty balls lost")
        growing.append(len(records))
    require(growing[0] < growing[1] < growing[2], "growing-output oracle family is constant")
    sphere = by_name["cosphere30"]["points"]
    records = expected_records(sphere, 2)
    common = [record for record in records if len(record[3]) == 30]
    require(len(common) > 1 and len({record[4] for record in common}) == 1,
            "cosphere incidences must remain distinct despite identical BallKeys")
    case = dict(by_name["one_acute_triangle"], options=options())
    mutation_count = mutations(synthetic_row(case), case)
    require(expected_records((), 1) == () and expected_records(by_name["pair"]["points"], MAXIMUM_K) == (), "terminal empty q3 domains")
    return dict(fixtures=len(by_name), growing_outputs=growing, cosphere_incidences=len(common),
                mutations=mutation_count, scope="Fraction_only_no_native_execution")


def invalid_inputs():
    base = dict(fixtures()[3], options=options())
    args, data = arguments(base), payload(base)
    result = [("missing_input", args, b""), ("negative_n", args, b"-1\n"),
              ("n_overflow", args, b"18446744073709551616\n"), ("truncated", args, data[:-5]),
              ("trailing", args, data+b"unexpected\n"), ("missing_argument", args[:-1], data),
              ("extra_argument", args+["unexpected"], data), ("q4_unported", ["--q4", *args[1:]], data)]
    for at, value in ((1, "0"), (1, "-1"), (1, str(1 << 64)), (2, "0"), (2, "-1"),
                      (2, "nan"), (2, str(1 << 32)), (3, "invalid"), (4, "invalid"), (5, "0"), (5, "-1")):
        changed = list(args)
        changed[at] = value
        result.append(("bad_option_"+str(at)+"_"+value, changed, data))
    for word in (0x7f800000, 0xff800000, 0x7fc00000, 0x7f800001):
        for axis in range(3):
            points = [list(point) for point in base["points"]]
            points[2][axis] = word
            result.append(("nonfinite", args, payload(dict(base, points=points))))
    for points in (((0, 0, 0), (0, 0, 0), base["points"][2]),
                   ((0, 0, 0), (0x80000000, 0x80000000, 0x80000000), base["points"][2])):
        result.append(("duplicate_geometry", args, payload(dict(base, points=points))))
    for spelling in (b"gg000000", b"0x00000000", b"100000000"):
        result.append(("malformed_hex", args, data.replace(b"00000000", spelling, 1)))
    result.append(("empty_invalid_K", ["--case", "0", *args[2:]], b"0\n"))
    result.append(("bad_round", ["--case-env", "invalid", "0", *args[1:]], data))
    result.append(("bad_ftz", ["--case-env", "nearest", "2", *args[1:]], data))
    return result


def execute(binary, directory, name, data, argv, expected_exit):
    (directory / (name + ".input")).write_bytes(data)
    record = dict(command=[str(binary), *argv], exit_code=None, expected_exit=expected_exit)
    try:
        with (directory / (name + ".stdout")).open("xb") as stdout, (directory / (name + ".stderr")).open("xb") as stderr:
            child = subprocess.run(record["command"], input=data, stdout=stdout, stderr=stderr, check=False)
        record["exit_code"] = child.returncode
    except BaseException as error:
        record["error"] = f"{type(error).__name__}: {error}"
        raise
    finally:
        (directory / (name + ".json")).write_text(json.dumps(record, sort_keys=True) + "\n")
    require(record["exit_code"] == expected_exit, "native code mismatch; streams preserved: " + name)


def read_command(directory, binary, name, data, argv, expected_exit):
    require((directory / (name + ".input")).read_bytes() == data, "transcript input mismatch: " + name)
    record = strict_json((directory / (name + ".json")).read_bytes())
    require(type(record) is dict and set(record) == {"command", "exit_code", "expected_exit"} and
            record["command"] == [str(binary), *argv] and type(record["exit_code"]) is int and
            type(record["expected_exit"]) is int and record["exit_code"] == record["expected_exit"] == expected_exit,
            "transcript command/code mismatch: " + name)
    stdout, stderr = ((directory / (name + "." + stream)).read_bytes() for stream in ("stdout", "stderr"))
    require((not stdout and bool(stderr)) if expected_exit else not stderr,
            "transcript stdout/stderr status mismatch: " + name)
    return stdout


def read_transcript(directory, binary):
    requests, bad = cases(), invalid_inputs()
    names = {f"case_{i:04}.{suffix}" for i in range(len(requests)) for suffix in ("input", "stdout", "stderr", "json")}
    names.update(f"reject_{i:03}.{suffix}" for i in range(len(bad)) for suffix in ("input", "stdout", "stderr", "json"))
    require(directory.is_dir() and not directory.is_symlink() and {p.name for p in directory.iterdir()} == names and
            all(p.is_file() and not p.is_symlink() for p in directory.iterdir()), "global transcript inventory")
    before = {name: sha(directory / name) for name in sorted(names)}
    totals = dict(records=0, shell_ids=0, key_words=0, max_shell=0)
    rows = []
    for i, case in enumerate(requests):
        row = strict_json(read_command(directory, binary, f"case_{i:04}", payload(case), arguments(case), 0))
        stats = validate_row(row, case)
        rows.append(row)
        for name in ("records", "shell_ids", "key_words"):
            totals[name] += stats[name]
        totals["max_shell"] = max(totals["max_shell"], stats["max_shell"])
    for i, (_, argv, data) in enumerate(bad):
        read_command(directory, binary, f"reject_{i:03}", data, argv, 2)
    require(totals["records"] > 1000 and totals["max_shell"] == 30, "native global output nonvacuity")
    selected = next(i for i, case in enumerate(requests)
                    if case["name"] == "one_acute_triangle" and case["options"]["kmax"] == 5)
    mutation_count = mutations(rows[selected], requests[selected])
    require({name: sha(directory / name) for name in sorted(names)} == before, "transcript changed while reading")
    digest = hashlib.sha256(json.dumps(before, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    return dict(cases=len(requests), fixtures=len(fixtures()), invalid_inputs=len(bad), mutation_checks=mutation_count,
                **totals, transcript_sha256=digest)


def run(binary, output):
    sources = {str(path): sha(path) for path in ORACLE_SOURCES}
    binary_hash = sha(binary)
    output.mkdir(parents=True, exist_ok=False)
    for i, case in enumerate(cases()):
        execute(binary, output, f"case_{i:04}", payload(case), arguments(case), 0)
    for i, (_, argv, data) in enumerate(invalid_inputs()):
        execute(binary, output, f"reject_{i:03}", data, argv, 2)
    result = read_transcript(output, binary)
    require(sha(binary) == binary_hash and {str(path): sha(path) for path in ORACLE_SOURCES} == sources,
            "oracle source/native binary changed during gate")
    return dict(schema="mhgp8_float32_q3_global_gate_v1", status="passed", **result,
                binary=str(binary), binary_sha256=binary_hash, oracle_source_sha256=sources,
                optimized=bool(sys.flags.optimize), scope="native_global_q3_support_stream_not_q4_or_FULL_or_GPU")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--read", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.selftest:
        require(args.binary is None and args.output is None and not args.read, "selftest takes no native arguments")
        print(json.dumps(oracle_selftest(), sort_keys=True))
    else:
        require(args.binary is not None and args.output is not None, "binary and output are required")
        binary = args.binary.resolve(strict=True)
        result = read_transcript(args.output.resolve(strict=True), binary) if args.read else run(binary, args.output.resolve())
        print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
