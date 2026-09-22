"""Validate paired component receipts, never a geometric/FULL certificate."""

from __future__ import annotations

import math
import re
from typing import Any

from run_p0_matrix import WORK_FIELDS, require, uint, validate_work

# Largeur de coordonnée du moteur entier (18 bits depuis le 22 septembre 2026) : au plus 18 coupes
# au milieu par axe, donc 54 niveaux d'index et 55 cadres de pile ; bornes prouvées, jamais des quotas.
COORDINATE_BITS = 18
MAX_INDEX_DEPTH = 3 * COORDINATE_BITS
INDEX_STACK_FRAMES = MAX_INDEX_DEPTH + 1


def finite_times(row: dict[str, Any], names: tuple[str, ...]) -> None:
    for name in names:
        value = row.get(name)
        require(type(value) in (int, float) and math.isfinite(value) and value >= 0,
                f"{name}: expected a finite nonnegative time")


def fingerprint(value: Any, name: str) -> None:
    require(type(value) is str and re.fullmatch(r"[0-9a-f]{1,16}", value) is not None,
            f"{name}: invalid fingerprint")


def close(value: float, expected: float, name: str) -> None:
    require(math.isclose(value, expected, rel_tol=1e-12, abs_tol=1e-6),
            f"{name}: inconsistent timing partition")


def common(row: dict[str, Any], command: list[str], kind: str) -> None:
    require(type(row) is dict and len(command) == (8 if kind == "additive" else 7),
            "invalid paired result or command length")
    fixed = {
        "schema": f"mhgp8_{kind}_probe_v1", "status": "completed",
        "scope": ("single_rectangle_three_lane_credit_batch" if kind == "batch"
                  else "single_rectangle_axis_q2_residual" if kind == "axis"
                  else "single_rectangle_additive_axis_q2_residual"),
        "backend": "cpu_reference", "profile": "quantized_u16_input_only",
        "public_status": "not_claimed",
        "s_role": "rectangle_precondition_not_wspd_generation",
        "totals_kind": "common_setup_plus_one_arm_not_pair_wall_time",
    }
    for key, expected in fixed.items():
        require(row.get(key) == expected, f"{key}: wrong paired scope/status")
    for key, expected in (("threads", 1), ("owner_preparations", 1)):
        require(uint(row.get(key), key) == expected, f"{key}: wrong count")
    require(row.get("same_owner") is True and row.get("seed", 0) is None,
            "unpaired owner or random fixture")
    require(row.get("candidates_expanded") is False and
            row.get("downstream_measured") is False, "unexpected downstream claim")
    expected_tuple = {"n": int(command[1]), "strategy": command[2],
                      "family": command[3], "kmax": int(command[4]),
                      "separation_s": int(command[5]), "order": command[6]}
    for key, expected in expected_tuple.items():
        if type(expected) is int:
            uint(row.get(key), key)
        require(row.get(key) == expected, f"paired command/result mismatch: {key}")
    n = row["n"]
    b = max(1, n // 16) if row["family"] == "skew" else n - n // 2
    require(uint(row.get("n_a"), "n_a") == n - b and
            uint(row.get("n_b"), "n_b") == b, "wrong factor cardinalities")
    version = 2 if row["family"] == "sheet_full" else 1
    require(uint(row.get("fixture_version"), "fixture_version") == version,
            "wrong fixture version")
    fingerprint(row.get("input_fnv1a64_le_u16_xyz"), "input")
    validate_work(row.get("preparation_work"), "preparation_work")
    prep = row["preparation_work"]
    require(prep["validation_points"] == n and
            all(prep[key] == 0 for key in WORK_FIELDS
                if key not in ("validation_points", "uniqueness_comparisons")) and
            all(value == 0 for value in prep["predicates"].values()),
            "unexpected owner preparation for no-core fixture")


def validate_batch(row: dict[str, Any], command: list[str]) -> None:
    common(row, command, "batch")
    require(row.get("plans_identical") is True and
            row.get("checksum_kind") == "fnv1a64_le_u64_plan_v1",
            "missing physical comparison/checksum contract")
    finite_times(row, ("generation_ms", "prepare_ms", "baseline_shared_owner_ms",
                      "batch_ms", "comparison_ms", "paired_execution_ms",
                      "baseline_total_ms", "batch_total_ms"))
    setup = row["generation_ms"] + row["prepare_ms"]
    close(row["baseline_total_ms"], setup + row["baseline_shared_owner_ms"], "baseline")
    close(row["batch_total_ms"], setup + row["batch_ms"], "batch")
    require(row["paired_execution_ms"] + 1e-6 >= setup + row["baseline_shared_owner_ms"] +
            row["batch_ms"] + row["comparison_ms"], "paired execution omits measured work")
    shared = row.get("shared_work")
    validate_work(shared, "shared_work")
    require(type(row.get("lanes")) is list and len(row["lanes"]) == 3,
            "missing or duplicated geometric lanes")
    prep_fields = {"tube_records", "tube_cells", "tube_sort_comparisons",
                   "tube_separation_fallbacks"}
    require(all(shared[key] == 0 for key in WORK_FIELDS if key not in prep_fields) and
            all(value == 0 for value in shared["predicates"].values()),
            "query work hidden in the shared preparation")
    if row["strategy"] == "tubes" and row["separation_s"] >= 8:
        require(shared["tube_records"] == row["n"] and
                shared["tube_separation_fallbacks"] == 0,
                "missing tube preparation on a separated active no-core fixture")
    for q, lane in zip((2, 3, 4), row["lanes"]):
        require(type(lane) is dict, "lane must be an object")
        require(uint(lane.get("lane"), "lane") == q, "wrong lane order")
        h = max(0, row["kmax"] + 2 - q)
        for name in ("threshold", "core_credit", "total_pairs", "candidate_pairs",
                     "candidate_descriptors"):
            uint(lane.get(name), name)
        require(lane["threshold"] == h and lane["core_credit"] == 0 and
                lane["total_pairs"] == row["n_a"] * row["n_b"] and
                lane["candidate_pairs"] <= lane["total_pairs"] and
                lane["candidate_descriptors"] <= h * (h + 1) // 2 and
                (lane["candidate_descriptors"] == 0) == (lane["candidate_pairs"] == 0),
                "inconsistent lane cardinalities")
        for arm in ("baseline", "batch"):
            fingerprint(lane.get(f"{arm}_checksum"), f"{arm}_checksum")
        require(lane["baseline_checksum"] == lane["batch_checksum"], "different plans")
        old, new = lane.get("baseline_work"), lane.get("batch_work")
        require(type(new) is dict, "batch_work must be an object")
        uint(new.get("tube_records"), "batch_work.tube_records")
        validate_work(old, "baseline_work")
        # Queries deliberately record no preparation. Borrow only the record
        # count to validate the sweep bound; never rewrite the actual receipt.
        allowance = dict(new)
        if row["strategy"] == "tubes":
            allowance["tube_records"] = shared["tube_records"]
        validate_work(allowance, "batch_work")
        if row["strategy"] == "tubes":
            require(all(new[key] == 0 for key in prep_fields), "repeated batch preparation")
            require(all(old[key] == new[key] for key in WORK_FIELDS
                        if key not in prep_fields) and old["predicates"] == new["predicates"],
                    "query work changed under sharing")
            require(all(old[key] == (shared[key] if h else 0) for key in prep_fields),
                    "shared preparation differs from independent preparation")
        else:
            require(old == new and all(shared[key] == 0 for key in WORK_FIELDS),
                    "non-tube strategy changed work")
        require(old["validation_points"] == new["validation_points"] == 0 and
                old["tube_credited_sites"] <= row["n"] * h,
                "repeated validation or unsaturated credits")


AXIS_FIELDS = ("sort_passes", "sorted_sites", "sort_comparisons", "columns",
               "constrained_anchors", "slab_bound_updates", "tree_point_visits",
               "tree_nodes", "query_nodes", "contained_nodes", "disjoint_nodes",
               "whole_factor_accepts", "whole_factor_rejects", "emitted_blocks",
               "max_tree_depth")


def validate_axis(row: dict[str, Any], command: list[str]) -> None:
    common(row, command, "axis")
    require(row.get("baseline_kind") == "single_q2_credit_plan", "wrong baseline")
    require(row.get("checksum_kind") == "fnv1a64_le_u64_axis_plan_v1",
            "wrong axis checksum convention")
    finite_times(row, ("generation_ms", "prepare_ms", "baseline_ms", "axis_ms",
                      "inspection_ms", "paired_execution_ms", "baseline_total_ms",
                      "axis_total_ms"))
    setup = row["generation_ms"] + row["prepare_ms"]
    close(row["baseline_total_ms"], setup + row["baseline_ms"], "baseline")
    close(row["axis_total_ms"], setup + row["axis_ms"], "axis")
    require(row["paired_execution_ms"] + 1e-6 >= setup + row["baseline_ms"] + row["axis_ms"] +
            row["inspection_ms"], "paired execution omits measured work")
    total = row["n_a"] * row["n_b"]
    require(uint(row.get("total_pairs"), "total_pairs") == total, "wrong pair total")
    for name in ("baseline_candidates", "axis_candidates", "axis_descriptors"):
        uint(row.get(name), name)
    require(row["baseline_candidates"] <= total and row["axis_candidates"] <= total and
            (row["axis_descriptors"] == 0) == (row["axis_candidates"] == 0),
            "invalid residual cardinality")
    fingerprint(row.get("axis_checksum"), "axis_checksum")
    validate_work(row.get("baseline_work"), "baseline_work")
    work = row.get("axis_work")
    require(type(work) is dict and set(work) == set(AXIS_FIELDS), "wrong axis counters")
    for name in AXIS_FIELDS:
        uint(work[name], name)
    require(work["sort_passes"] == 3 and work["sorted_sites"] == 3 * row["n_a"] and
            work["constrained_anchors"] <= row["n_a"] and work["max_tree_depth"] <= MAX_INDEX_DEPTH and
            work["emitted_blocks"] == row["axis_descriptors"] and
            work["contained_nodes"] + work["whole_factor_accepts"] == work["emitted_blocks"] and
            work["whole_factor_accepts"] + work["whole_factor_rejects"] <= row["n_a"] and
            work["slab_bound_updates"] <= 6 * row["n_a"] and
            work["contained_nodes"] + work["disjoint_nodes"] <= work["query_nodes"],
            "inconsistent axis work")


def stable_signature(row: dict[str, Any], kind: str) -> Any:
    if kind == "batch":
        return (row["input_fnv1a64_le_u16_xyz"], row["preparation_work"],
                row["shared_work"], row["lanes"])
    if kind == "additive":
        return tuple(row[key] for key in (
            "input_fnv1a64_le_u16_xyz", "preparation_work", "variant", "need", "total_pairs",
            "independent_candidates", "independent_descriptors", "independent_checksum", "independent_work",
            "variant_candidates", "variant_descriptors", "variant_checksum", "variant_work",
            "local_plan_present", "local_candidates", "local_descriptors", "local_checksum", "local_work"))
    return (row["input_fnv1a64_le_u16_xyz"], row["preparation_work"],
            row["baseline_candidates"], row["axis_candidates"], row["axis_descriptors"],
            row["axis_checksum"], row["baseline_work"], row["axis_work"])


ADDITIVE_FIELDS = ("axis_bound_queries", "axis_count_queries", "axis_rank_comparisons",
                   "axis_pruned_nodes", "axis_slab_rejects", "restriction_credit_copies", "restriction_credit_visits",
                   "restriction_bound_queries", "restriction_pruned_nodes", "coalesced_blocks")


def additive_invariants(row: dict[str, Any]) -> list[tuple[tuple[Any, ...], Any]]:
    """The reference cannot depend on the unused local strategy or variant."""
    identity = (row["family"], row["n"], row["kmax"])
    arms = ["independent"]
    if row["variant"] == "additive":
        arms.append("variant")
    return [((arm, *identity), tuple(row[f"{arm}_{name}"] for name in
                                    ("candidates", "descriptors", "checksum", "work")))
            for arm in arms]


def validate_additive(row: dict[str, Any], command: list[str]) -> None:
    common(row, command, "additive")
    require(row.get("independent_kind") == "axis_q2_independent", "wrong independent baseline")
    require(row.get("variant") == command[7] and command[7] in ("additive", "intersection"),
            "variant: command/result mismatch or invalid mode")
    require(row["order"] in ("independent-first", "variant-first"), "wrong additive order")
    require(row.get("checksum_kind") == "fnv1a64_le_u64_axis_plan_v1" and
            row.get("local_checksum_kind") == "fnv1a64_le_u64_plan_v1",
            "wrong additive/local checksum convention")
    restricted = row["variant"] == "intersection"
    require(row.get("local_plan_present") is restricted, "incorrect restriction presence")
    finite_times(row, ("generation_ms", "prepare_ms", "independent_ms", "local_plan_ms",
                      "selection_ms", "variant_ms", "inspection_ms", "paired_execution_ms",
                      "independent_total_ms", "variant_total_ms"))
    setup = row["generation_ms"] + row["prepare_ms"]
    close(row["variant_ms"], row["local_plan_ms"] + row["selection_ms"], "variant")
    close(row["independent_total_ms"], setup + row["independent_ms"], "independent total")
    close(row["variant_total_ms"], setup + row["variant_ms"], "variant total")
    require(row["paired_execution_ms"] + 1e-6 >= setup + row["independent_ms"] +
            row["variant_ms"] + row["inspection_ms"], "paired execution omits measured work")
    total = row["n_a"] * row["n_b"]
    require(uint(row.get("total_pairs"), "total_pairs") == total and
            uint(row.get("need"), "need") == row["kmax"], "incorrect total or no-core need")
    for arm in ("independent", "variant"):
        candidates = uint(row.get(f"{arm}_candidates"), f"{arm}_candidates")
        descriptors = uint(row.get(f"{arm}_descriptors"), f"{arm}_descriptors")
        require(descriptors <= candidates <= total and (descriptors == 0) == (candidates == 0),
                f"{arm}: incorrect residual counts")
        fingerprint(row.get(f"{arm}_checksum"), f"{arm}_checksum")
        work = row.get(f"{arm}_work")
        require(type(work) is dict and set(work) == {*AXIS_FIELDS, *ADDITIVE_FIELDS},
                f"{arm}: incorrect axis work fields")
        for name in (*AXIS_FIELDS, *ADDITIVE_FIELDS):
            uint(work[name], f"{arm}.{name}")
        require(work["sort_passes"] == 3 and work["sorted_sites"] == 3 * row["n_a"] and
                work["max_tree_depth"] <= MAX_INDEX_DEPTH and work["constrained_anchors"] <= row["n_a"] and
                work["slab_bound_updates"] <= 6 * row["n_a"] and
                work["emitted_blocks"] == descriptors and
                work["contained_nodes"] + work["whole_factor_accepts"] ==
                descriptors + work["coalesced_blocks"] and
                work["whole_factor_accepts"] + work["whole_factor_rejects"] <= row["n_a"] and
                work["contained_nodes"] + work["disjoint_nodes"] <= work["query_nodes"],
                f"{arm}: inconsistent selection accounting")
        if arm == "independent":
            require(all(work[name] == 0 for name in ADDITIVE_FIELDS),
                    "independent baseline used additive or restriction work")
        else:
            require(work["axis_pruned_nodes"] + work["axis_slab_rejects"] + work["restriction_pruned_nodes"] ==
                    work["disjoint_nodes"] + work["whole_factor_rejects"],
                    "pruning reasons disagree with rejected nodes")
            require(work["axis_bound_queries"] + work["axis_slab_rejects"] +
                    work["restriction_pruned_nodes"] == work["query_nodes"] +
                    work["whole_factor_accepts"] + work["whole_factor_rejects"] and
                    work["axis_bound_queries"] <= work["axis_count_queries"] <=
                    6 * work["axis_bound_queries"] and
                    work["axis_rank_comparisons"] <= row["need"].bit_length() * work["axis_count_queries"],
                    "additive bound/count/rank work is inconsistent")
            require(work["restriction_credit_copies"] == (row["n"] if restricted else 0),
                    "restriction credits not copied exactly once")
            if not restricted:
                require(all(work[name] == 0 for name in ADDITIVE_FIELDS
                            if name.startswith("restriction_")), "restriction work without a local plan")
            else:
                require(work["restriction_bound_queries"] == work["query_nodes"] +
                        work["whole_factor_accepts"] + work["whole_factor_rejects"] and
                        work["restriction_credit_visits"] >= row["n_b"],
                        "restriction bounds or root-credit scan omitted")
    require(row["variant_candidates"] <= row["independent_candidates"],
            "additive variant enlarged the independent residual")
    local_fields = ("local_candidates", "local_descriptors", "local_checksum", "local_work")
    if not restricted:
        require(row["local_plan_ms"] == 0 and all(row.get(name, False) is None for name in local_fields),
                "unrestricted additive arm fabricated local work")
    else:
        candidates = uint(row.get("local_candidates"), "local_candidates")
        descriptors = uint(row.get("local_descriptors"), "local_descriptors")
        require(row["variant_candidates"] <= candidates <= total and
                descriptors <= row["need"] * (row["need"] + 1) // 2 and
                (descriptors == 0) == (candidates == 0), "intersection enlarged local residual")
        fingerprint(row.get("local_checksum"), "local_checksum")
        validate_work(row.get("local_work"), "local_work")
        work = row["local_work"]
        require(work["validation_points"] == work["uniqueness_comparisons"] == 0 and
                work["tube_credited_sites"] <= row["n"] * row["need"],
                "local plan repeated ownership validation or unsaturated credits")
