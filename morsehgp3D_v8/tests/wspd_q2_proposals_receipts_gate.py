#!/usr/bin/env python3
"""Bounded real captures of the widened front proposal window and hostile receipt mutations.

Explicit port of the batched receipt mutations at 8d615cfd. The new row and
pair readers alone are imported; geometric independence is supplied by the
separate C++ gate (window judge, q2 front replay, brute-force oracle), not by
a historical receipt or by the checks in this Python file.
"""

import argparse
from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bench"))
from run_wspd_q2_proposals_checks import (  # noqa: E402
    ARTIFACT_NAMES, SOURCE_PATHS, VARIANTS, strict_json, validate_pairs, validate_pins, validate_reference,
    validate_row)


def require(value, message):
    if not value:
        raise RuntimeError(message)


def at(value, path):
    for key in path:
        value = value[key]
    return value


REJECTIONS = (RuntimeError, ValueError, KeyError, TypeError, OverflowError)


def reject(row, command, label):
    try:
        validate_row(row, command)
    except REJECTIONS:
        return 1
    raise RuntimeError(f"proposals reader accepted mutant: {label}")


def replace(row, command, path, value):
    mutant = deepcopy(row)
    at(mutant, path[:-1])[path[-1]] = value
    return reject(mutant, command, repr(path))


def dictionaries(value, path=()):
    if type(value) is dict:
        yield path, value
        for key, child in value.items():
            yield from dictionaries(child, (*path, key))
    elif type(value) is list:
        for key, child in enumerate(value):
            yield from dictionaries(child, (*path, key))


def check_unit_contracts():
    """Pure unit models: no claimed capture, executable, or qualification."""
    def rejected_call(call, label):
        try:
            call()
        except REJECTIONS:
            return 1
        raise RuntimeError(f"reader accepted unit-contract mutant: {label}")

    require(strict_json('{"x":1,"nested":{"x":2},"values":[null,true]}') ==
            {"x": 1, "nested": {"x": 2}, "values": [None, True]}, "valid strict JSON rejected")
    count = 0
    for text in ('{"x":1,"x":2}', '{"outer":{"x":1,"x":2}}',
                 '{"x":NaN}', '{"x":Infinity}', '{"x":1e999}'):
        count += rejected_call(lambda: strict_json(text), "duplicate/nonfinite JSON")

    # This nonexistent model build tests pin membership/path rules only. Hash
    # values are deliberately synthetic, not measurements of repository files.
    root = Path(__file__).resolve().parents[2]
    build = root / "build" / "proposals_receipts_gate_model"
    relative = build.relative_to(root)
    cache = str(relative / "CMakeCache.txt")
    executable = build / "mhgp8_wspd_q2_proposals_probe"
    manifest = dict(source_sha256={name: "0" * 64 for name in SOURCE_PATHS},
                    artifact_sha256={str(relative / name): "1" * 64 for name in
                                     {*ARTIFACT_NAMES, "CMakeCache.txt"}},
                    campaign="qualification", build=str(build), planned_commands=[["measure", [str(executable)]]])
    validate_pins(manifest)
    source = sorted(SOURCE_PATHS)[0]
    mutants = []
    missing_source = deepcopy(manifest)
    del missing_source["source_sha256"][source]
    mutants.append((missing_source, "missing source pin"))
    extra_source = deepcopy(manifest)
    extra_source["source_sha256"]["morsehgp3D_v8/src/unknown_source.cpp"] = "0" * 64
    mutants.append((extra_source, "unknown source pin"))
    wrong_hash = deepcopy(manifest)
    wrong_hash["source_sha256"][source] = "A" * 64
    mutants.append((wrong_hash, "noncanonical source hash"))
    missing_cache = deepcopy(manifest)
    del missing_cache["artifact_sha256"][cache]
    mutants.append((missing_cache, "missing cache pin"))
    missing_indirect_executable = deepcopy(manifest)
    del missing_indirect_executable["artifact_sha256"][str(relative / "mhgp8_axis_q2_gate")]
    mutants.append((missing_indirect_executable, "missing indirectly executed CTest binary"))
    escaped_artifact = deepcopy(manifest)
    escaped_artifact["artifact_sha256"]["build/elsewhere/mhgp8_probe"] = "3" * 64
    mutants.append((escaped_artifact, "artifact outside model build"))
    unpinned_command = deepcopy(manifest)
    unpinned_command["planned_commands"][0][1][0] = str(build / "mhgp8_unpinned_probe")
    mutants.append((unpinned_command, "unpinned command executable"))
    for mutant, label in mutants:
        count += rejected_call(lambda: validate_pins(mutant), label)
    require(count == 12, "unit mutation inventory changed")
    return count


def check_mutants(row, command, exhaustive):
    count = 0
    extension = row["extension_work"]
    edits = [
        (("schema",), "mhgp8_wspd_q2_parallel_probe_v1"),
        (("scope",), "full"), (("public_status",), "qualified"), (("gcp_used",), True),
        (("execution",), "mono_reference" if row["threads"] else "parallel_front"),
        (("window_factor",), 3), (("window_factor",), True), (("small_factor_limit",), 0),
        (("small_factor_limit",), "all"),
        (("candidate_pairs",), row["candidate_pairs"] + 1),
        (("census_work", "input_descriptors"), row["input_rectangles"] + 1),
        (("census_work", "query_cover_visits"), 1),
        (("census_work", "frontier_restarts"), 1),
        (("parallel_work", "completed_jobs"), row["parallel_work"]["jobs"] + 1),
        (("workers",), []), (("workers",), tuple(row["workers"])),
        (("workers", 0, "jobs"), row["workers"][0]["jobs"] + 1),
        (("workers", 0, "front_products"), row["workers"][0]["front_products"] + 1),
        (("timings", "pipeline_wall_ms"), row["timings"]["total_ms"] + 1),
        (("extension_work", "extended_proposals_in_factors"), extension["extended_proposals"] + 1),
        (("extension_work", "extended_credits"), row["front_work"]["witness_lane_credits"] + 1),
        (("extension_work", "extended_products"), row["front_work"]["witness_searches"] + 1),
        (("extension_work", "extended_rejections"), extension["extended_products"] + 1),
        (("extension_work", "extended_rejections"), [extension["extended_rejections"], 0, 0]),
    ]
    for path in (("timings", "total_ms"), ("workers", 0, "elapsed_ms"), ("pool_work", "preparation_ms_sum")):
        for value in (-1, float("nan"), float("inf"), True, "0"):
            edits.append((path, value))
    count += sum(replace(row, command, path, value) for path, value in edits)

    # Compensated mutants: each keeps every total consistent, so that ONE theorem of the
    # extension ledger is the only possible cause of rejection (message checked).
    def compensated(mutate, message, label):
        mutant = deepcopy(row)
        mutate(mutant)
        try:
            validate_row(mutant, command)
        except REJECTIONS as cause:
            require(any(text in str(cause) for text in message), f"mutant {label} rejected for another reason: {cause}")
            return 1
        raise RuntimeError(f"proposals reader accepted compensated mutant: {label}")

    kmax, n, factor = row["kmax"], row["n"], row["window_factor"]
    products = extension["extended_products"]

    def unbounded(m):
        delta = (min(kmax * factor, n) - min(kmax, n)) * products + 1 - m["extension_work"]["extended_proposals"]
        m["extension_work"]["extended_proposals"] += delta
        m["front_work"]["proposed_sites"] += delta
        m["front_work"]["h_bound_tests"] += delta
    count += compensated(unbounded, ("extension work outside its widened-window bound",), "unbounded extension proposals")
    if factor != 1 and products > 0:
        def inflated_products(m):  # (a) more extended products than extra proposals allow a full historical window for
            m["extension_work"]["extended_products"] = m["front_work"]["witness_searches"]
        # Detectable only when the historical window rejected at least one product: those searches
        # cannot have been extended. A self-declared ledger cannot expose it otherwise.
        if row["front_work"]["fully_rejected_products"] > extension["extended_rejections"]:
            count += compensated(inflated_products, ("extension work outside its widened-window bound",
                                                    "historical-window part of the proposal ledger is impossible"),
                                 "extended products inflated to the searches")

        def creditless_rejections(m):  # (b) rejections without any extension credit
            m["front_work"]["witness_lane_credits"] -= m["extension_work"]["extended_credits"]
            m["extension_work"]["extended_credits"] = 0
        if extension["extended_rejections"] > 0:
            count += compensated(creditless_rejections, ("extension work outside its widened-window bound",),
                                 "extension rejections without extension credits")

        def relabelled_history(m):  # (d) historical proposals renamed extension proposals
            move = m["front_work"]["proposed_sites"] - m["extension_work"]["extended_proposals"] - \
                min(kmax, n) * products - (m["front_work"]["witness_searches"] - products) + 1
            m["extension_work"]["extended_proposals"] += move
        # Several theorems overlap on this fault (widened bound, historical bound at K = 1,
        # totals); each is a legitimate cause, none may let it through.
        count += compensated(relabelled_history, ("historical-window part of the proposal ledger is impossible",
                                                 "witness proposal or certification work outside its declared bound",
                                                 "extension counters exceed the totals that include them",
                                                 "extension work outside its widened-window bound"),
                             "historical proposals relabelled as extension")
        if row["small_factor_limit"] is None:
            def hidden_survivor(m):  # exact identity of the unlimited policy
                m["extension_work"]["extended_products"] -= 1
            # When every extended product used all its extra ranks, the widened bound sees it first.
            count += compensated(hidden_survivor, ("an unlimited widened window left a surviving product",
                                                  "extension work outside its widened-window bound"),
                                 "unlimited window with an unextended survivor")
    if factor == 1:
        for name in ("extended_products", "extended_proposals"):
            mutant = deepcopy(row)
            mutant["extension_work"][name] = 1
            if name == "extended_proposals":
                mutant["front_work"]["proposed_sites"] += 1
                mutant["front_work"]["h_bound_tests"] += 1
            count += reject(mutant, command, "extension work at factor 1: " + name)

    for position, value in enumerate(command):
        wrong = command.copy()
        if position == 1:
            wrong[position] = "terrain" if value != "terrain" else "uniform"
        elif position == 8:
            wrong[position] = {"1": "2", "2": "4", "4": "1"}[value]
        elif position == 9:
            wrong[position] = "17" if value != "17" else "18"
        else:
            wrong[position] = str(int(value) + 1)
        count += reject(deepcopy(row), wrong, f"command[{position}]")
    for wrong in (command[:-1], command + ["extra"]):
        count += reject(deepcopy(row), wrong, "command arity")

    if exhaustive:
        for path, obj in dictionaries(row):
            for name, value in obj.items():
                mutant = deepcopy(row)
                del at(mutant, path)[name]
                count += reject(mutant, command, f"missing {(*path, name)}")
                if type(value) is int:
                    for wrong in (True, -1, 2**64, str(value)):
                        count += replace(row, command, (*path, name), wrong)
            mutant = deepcopy(row)
            at(mutant, path)["unexpected"] = 0
            count += reject(mutant, command, f"unknown field in {path}")
    return count


def check_pair_mutants(rows):
    """Every mutant is applied to ALL rows sharing the widened key (both worker counts), so that the
    inter-thread signature cannot reject it first; the expected rejection message is checked."""
    def rejected(mutate, message, label):
        mutated = deepcopy(rows)
        mutate(mutated)
        try:
            validate_pairs(mutated, "smoke")
        except REJECTIONS as cause:
            require(message in str(cause), f"pair mutant {label} rejected for another reason: {cause}")
            return 1
        raise RuntimeError(f"pair reader accepted mutant: {label}")

    # The comparison group on which EVERY mutant applies: its reference rejects pair mass at the
    # front and keeps strictly more candidates than each of its widened windows.
    def applicable(reference):
        members = [r for r in rows if "window_factor" in r and r["window_factor"] != 1 and
                   all(r[key] == reference[key] for key in ("family", "n", "kmax", "s", "pool_min_factor"))]
        return reference["front_work"]["rejected_pair_mass"][0] > 0 and members and \
            all(r["candidate_pairs"] < reference["candidate_pairs"] for r in members)
    first = next(r for r in rows if "window_factor" not in r and applicable(r))
    config = {key: first[key] for key in ("family", "n", "kmax", "s", "pool_min_factor")}

    def same_config(row):
        return all(row[key] == value for key, value in config.items())

    def select(rows_, factor, limit):
        return [r for r in rows_ if same_config(r) and r.get("window_factor") == factor and
                r.get("small_factor_limit", "reference") == limit]

    def references(rows_):
        return [r for r in rows_ if same_config(r) and "window_factor" not in r]

    count = 0

    def changed_supports(m):
        for row in select(m, 2, None):
            row["digest"]["sum"] = "0" if row["digest"]["sum"] != "0" else "1"
    count += rejected(changed_supports, "canonical complete supports changed", "widened supports changed")

    def drifting_default(m):
        for row in select(m, 1, None):
            row["front_work"]["product_visits"] += 1
    count += rejected(drifting_default, "integer geometric/callback work changed", "factor 1 left the historical signature")

    def missing_variant(m):
        for row in select(m, 2, None):
            m.remove(row)
    count += rejected(missing_variant, "incomplete proposal comparison group", "incomplete comparison group")

    def added_candidates(m):
        top = max(r["candidate_pairs"] for r in references(m))
        for row in select(m, 2, None):
            row["candidate_pairs"] = top + 1
    count += rejected(added_candidates, "widened window added census candidates", "widened window added candidates")

    def changed_accepted(m):
        for row in select(m, 4, None):
            row["accepted_pairs"] += 1
    count += rejected(changed_accepted, "widened window changed the accepted supports", "accepted supports changed")

    def less_rejected_mass(m):
        for row in select(m, 4, 16):
            row["front_work"]["rejected_pair_mass"][0] = 0
    count += rejected(less_rejected_mass, "widened window rejected less pair mass", "rejected mass decreased")

    def more_rectangles(m):
        top = max(r["input_rectangles"] for r in references(m))
        for row in select(m, 2, 16):
            row["input_rectangles"] = top + 1
    count += rejected(more_rectangles, "widened window emitted more rectangles", "input rectangles increased")

    def limit_not_monotone(m):
        top = max(r["candidate_pairs"] for r in select(m, 2, 16))
        for row in select(m, 2, None):
            row["candidate_pairs"] = top + 1
    count += rejected(limit_not_monotone, "unlimited window kept more candidates", "limit monotony broken")

    def factor_not_monotone(m):  # Both 4K rows, so that the limit monotony cannot see it first.
        top = max(r["candidate_pairs"] for r in (*select(m, 2, None), *select(m, 2, 16)))
        for row in (*select(m, 4, None), *select(m, 4, 16)):
            row["candidate_pairs"] = top + 1
    count += rejected(factor_not_monotone, "window 4K kept more candidates", "factor monotony broken")

    def vacuous(m):
        for row in m:
            if "extension_work" in row:
                row["extension_work"]["extended_rejections"] = 0
    count += rejected(vacuous, "vacuous capture", "vacuous capture without any extension rejection")
    require(count == 10, "pair mutation inventory changed")
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--reference", required=True)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    calls, groups, mutants, invalid = 0, 0, 0, 0
    unit_mutants = check_unit_contracts()
    mutants += unit_mutants
    covered_sizes, covered_pool, covered_k = set(), set(), set()
    pool_rows, extended_rows, rejecting_rows = 0, 0, 0
    rows = []
    for family_number, family in enumerate(("uniform", "terrain", "clusters", "rows")):
        for separation_number, separation in enumerate((8, 10, 12)):
            n = (32, 64, 128)[separation_number]
            k = (1, 5, 10)[(family_number + separation_number) % 3]
            pool = (0, 16, 64)[(family_number + 2 * separation_number) % 3]
            if family == "clusters" and n == 128:
                k, pool = 1, 2  # Positive filtered bands, not only Pool passthrough.
            for workers in (1, 4):
                common = [str(n), family, str(k), str(separation), "3", str(workers), "1", str(pool)]
                capture = subprocess.run([args.reference, *common], capture_output=True, text=True, timeout=30)
                require(capture.returncode == 0 and not capture.stderr, f"reference probe failed: {capture.stderr}")
                reference = strict_json(capture.stdout)
                validate_reference(reference, [args.reference, *common])
                rows.append(reference)
                for factor, limit in VARIANTS:
                    command = [*common, factor, limit]
                    capture = subprocess.run([args.probe, *command], capture_output=True, text=True, timeout=30)
                    require(capture.returncode == 0 and not capture.stderr,
                            f"proposals probe failed: {capture.stdout}\n{capture.stderr}")
                    row = strict_json(capture.stdout)
                    validate_row(row, command)
                    mutants += check_mutants(row, command, calls == 0)
                    calls += 1
                    pool_rows += row["pool_work"]["bands"] > 0
                    extended_rows += row["extension_work"]["extended_products"] > 0
                    rejecting_rows += row["extension_work"]["extended_rejections"] > 0
                    rows.append(row)
                groups += 1
                covered_sizes.add(n)
                covered_pool.add(pool)
                covered_k.add(k)
    validate_pairs(rows, "smoke")
    pair_mutants = check_pair_mutants(rows)
    mutants += pair_mutants
    require(calls == 120 and groups == 24 and covered_sizes == {32, 64, 128} and
            covered_pool == {0, 2, 16, 64} and covered_k == {1, 5, 10} and
            pool_rows > 0 and extended_rows > 0 and rejecting_rows > 0,
            "tiny proposals matrix incomplete or vacuous")

    valid = ["32", "uniform", "5", "8", "3", "4", "1", "0", "2", "16"]
    invalid_commands = [[], valid[:-1], valid + ["extra"]]
    for position in (0, 2, 3, 6, 8, 9):
        for value in ("0", "-1"):
            command = valid.copy()
            command[position] = value
            invalid_commands.append(command)
    for position, value in ((0, "1"), (1, "wrong"), (2, "11"), (4, "-1"), (4, "18446744073709551616"),
                            (5, "-1"), (7, "-1"), (8, "3"), (8, "8"), (9, "none"),
                            (9, "18446744073709551615")):
        command = valid.copy()
        command[position] = value
        invalid_commands.append(command)
    invalid_commands.append(["33", "rows", *valid[2:]])
    for command in invalid_commands:
        capture = subprocess.run([args.probe, *command], capture_output=True, text=True, timeout=30)
        require(capture.returncode == 2 and not capture.stdout and capture.stderr,
                f"invalid proposals CLI accepted: {command}")
        invalid += 1
    require(invalid == 27, "invalid CLI inventory changed")
    print(json.dumps(dict(status="passed", real_captures=calls, comparison_groups=groups,
                          rejected_mutants=mutants, pair_mutants=pair_mutants,
                          pool_captures=pool_rows, extended_captures=extended_rows,
                          rejecting_captures=rejecting_rows,
                          unit_contract_mutants=unit_mutants, invalid_cli=invalid,
                          full_contract_qualified=False), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
