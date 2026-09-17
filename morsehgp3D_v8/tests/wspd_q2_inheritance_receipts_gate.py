#!/usr/bin/env python3
"""Bounded real captures of the inherited front witnesses and hostile receipt mutations.

Explicit port of the proposals receipt mutations at 8190e7ab. The new row and
pair readers alone are imported; geometric independence is supplied by the
separate C++ gate (q2 front replay with inheritance, brute-force safety and
support oracles, independent Python model constants), not by a historical
receipt or by the checks in this Python file.
"""

import argparse
from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bench"))
from run_wspd_q2_inheritance_checks import (  # noqa: E402
    ARTIFACT_NAMES, SOURCE_PATHS, VARIANTS, strict_json, validate_kind, validate_pairs, validate_pins,
    validate_reference, validate_row)

# Every compensated mutant must be applied at least once over the matrix: a label that never
# applies would let its theorem go unjudged without any failure.
COMPENSATED = {}


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
    raise RuntimeError(f"inheritance reader accepted mutant: {label}")


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
    build = root / "build" / "inheritance_receipts_gate_model"
    relative = build.relative_to(root)
    cache = str(relative / "CMakeCache.txt")
    executable = build / "mhgp8_wspd_q2_inheritance_probe"
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
    extension, inheritance = row["extension_work"], row["inheritance_work"]
    edits = [
        (("schema",), "mhgp8_wspd_q2_parallel_probe_v1"),
        (("scope",), "full"), (("public_status",), "qualified"), (("gcp_used",), True),
        (("execution",), "mono_reference" if row["threads"] else "parallel_front"),
        (("window_factor",), 3), (("window_factor",), True), (("small_factor_limit",), 0),
        (("small_factor_limit",), "all"),
        (("inherit_witnesses",), int(row["inherit_witnesses"])), (("inherit_witnesses",), "1"),
        (("inherit_witnesses",), not row["inherit_witnesses"]),
        (("inheritance_work", "inherited_duplicates"), inheritance["inherited_credits"] + 1),
        (("inheritance_work", "extended_inherited_duplicates"), inheritance["inherited_duplicates"] + 1),
        (("inheritance_work", "inherited_rejections"), row["front_work"]["fully_rejected_products"] + 1),
        (("inheritance_work", "inherited_credits"), [inheritance["inherited_credits"]]),
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
            COMPENSATED[label] = COMPENSATED.get(label, 0) + 1
            return 1
        raise RuntimeError(f"inheritance reader accepted compensated mutant: {label}")

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
            # Under inheritance the new credits also enter the credit ledger, which answers first.
            count += compensated(creditless_rejections, ("extension work outside its widened-window bound",
                                                        "inheritance credit ledger does not close"),
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
    # Inheritance ledger. Each mutant keeps every other total consistent.
    ledger = ("inheritance credit ledger does not close",)
    searches, rectangles = row["front_work"]["witness_searches"], row["front_work"]["emitted_rectangles"]
    if row["inherit_witnesses"]:
        def beyond_searches(m):  # Two received ranks and one emitted credit at a time: the ledger still closes.
            d = ((kmax - 1) * searches - m["inheritance_work"]["inherited_credits"]) // 2 + 1
            m["inheritance_work"]["inherited_credits"] += 2 * d
            m["inheritance_work"]["emitted_witness_credits"] += d
        count += compensated(beyond_searches, ("a search received more than Kmax-1 ranks",),
                             "more received ranks than Kmax-1 per search")

        def beyond_rectangles(m):  # One new credit per emitted credit: the ledger still closes.
            d = (kmax - 1) * rectangles - m["inheritance_work"]["emitted_witness_credits"] + 1
            m["inheritance_work"]["emitted_witness_credits"] += d
            m["front_work"]["witness_lane_credits"] += d
        if row["front_work"]["witness_lane_credits"] + (kmax - 1) * rectangles - inheritance["emitted_witness_credits"] + 1 <= \
                row["front_work"]["h_bound_tests"] - (extension["extended_proposals"] - extension["extended_proposals_in_factors"]):
            count += compensated(beyond_rectangles, ("an emitted rectangle kept more than Kmax-1 credits",),
                                 "more emitted credits than Kmax-1 per rectangle")

        def untracked_rejections(m):
            m["inheritance_work"]["inherited_rejections"] = m["front_work"]["fully_rejected_products"] + 1
        count += compensated(untracked_rejections, ("more inherited rejections than rejected products",),
                             "more inherited rejections than rejected products")

        def credited_duplicates(m):  # Extension credits inside ]extension tests, tests + duplicates].
            e, i = m["extension_work"], m["inheritance_work"]
            e["extended_credits"] = e["extended_proposals"] - e["extended_proposals_in_factors"] - \
                i["extended_inherited_duplicates"] + 1
        if inheritance["extended_inherited_duplicates"] > 0:
            count += compensated(credited_duplicates, ("extension counters exceed the totals that include them",),
                                 "extension credits granted to received ranks")

        room = (kmax - 1) * searches - inheritance["inherited_credits"]
        if room >= 2:  # Otherwise the per-search bound, judged by its own mutant, would answer first.
            def odd_received(m):  # A list is received by BOTH children of a split: the sum is even.
                m["inheritance_work"]["inherited_credits"] += 1
            count += compensated(odd_received, ledger, "odd number of received ranks")

            def open_ledger(m):  # Two more received ranks that no product ever closed.
                m["inheritance_work"]["inherited_credits"] += 2
            count += compensated(open_ledger, ledger, "received ranks outside the credit ledger")

        if inheritance["emitted_witness_credits"] < (kmax - 1) * rectangles:  # Same remark for the rectangle bound.
            def forgotten_emission(m):
                m["inheritance_work"]["emitted_witness_credits"] += 1
            count += compensated(forgotten_emission, ledger, "emitted credits outside the credit ledger")

        def untested_duplicates(m):  # Tests relabelled as received ranks: the partition of proposals still closes.
            move = m["inheritance_work"]["inherited_credits"] - m["inheritance_work"]["inherited_duplicates"] + 1
            m["inheritance_work"]["inherited_duplicates"] += move
            m["front_work"]["h_bound_tests"] -= move
        if row["front_work"]["h_bound_tests"] - row["front_work"]["witness_lane_credits"] > \
                inheritance["inherited_credits"] - inheritance["inherited_duplicates"]:
            count += compensated(untested_duplicates, ("more received ranks proposed again than received ranks",),
                                 "more duplicates than received ranks")
    else:
        for name in inheritance:
            mutant = deepcopy(row)
            mutant["inheritance_work"][name] = 2
            if name == "inherited_duplicates":
                mutant["front_work"]["proposed_sites"] += 2
            count += reject(mutant, command, "inheritance work of a plain front: " + name)

    def untested_rank(m):  # A proposed rank that is neither in a factor, nor received, nor tested.
        m["front_work"]["proposed_sites"] += 1
    count += compensated(untested_rank, ("a proposed rank is neither in a factor, nor received, nor tested",),
                         "proposed rank outside the partition")
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
        elif position == 10:
            wrong[position] = "1" if value == "0" else "0"
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
    """Every mutant is applied to ALL rows sharing the option key (both worker counts), so that the
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

    keys = ("family", "n", "kmax", "s", "pool_min_factor")

    def members_of(rows_, reference):
        return [r for r in rows_ if "window_factor" in r and all(r[key] == reference[key] for key in keys)]

    def pick(members, factor, limit, inherit):
        return [r for r in members if (r["window_factor"], r["small_factor_limit"], r["inherit_witnesses"]) ==
                (factor, limit, inherit)]

    # The comparison group on which EVERY mutant applies: strict gains everywhere, so that each
    # mutant can move one quantity past ONE bound without crossing another bound first.
    def applicable(reference):
        members = members_of(rows, reference)
        candidates = {label: min(r["candidate_pairs"] for r in pick(members, *label))
                      for label in ((1, None, True), (2, 16, False), (2, 16, True), (4, None, False), (4, None, True))}
        visits = {label: min(r["front_work"]["product_visits"] for r in pick(members, *label))
                  for label in ((2, 16, False), (2, 16, True))}
        mass = {label: min(r["front_work"]["rejected_pair_mass"][0] for r in pick(members, *label))
                for label in ((2, 16, False), (2, 16, True))}
        return (candidates[(1, None, True)] < reference["candidate_pairs"] and
                candidates[(2, 16, True)] < candidates[(2, 16, False)] < reference["candidate_pairs"] and
                candidates[(4, None, False)] < candidates[(2, 16, False)] and
                visits[(2, 16, True)] < visits[(2, 16, False)] and
                mass[(2, 16, True)] > mass[(2, 16, False)] > reference["front_work"]["rejected_pair_mass"][0])
    eligible = [r for r in rows if "window_factor" not in r and applicable(r)]
    require(eligible, "no comparison group of the tiny matrix has the strict gains that the pair mutants need")
    first = eligible[0]

    def select(rows_, factor, limit, inherit):
        return pick(members_of(rows_, first), factor, limit, inherit)

    count = 0

    def changed_supports(m):
        for row in select(m, 2, 16, True):
            row["digest"]["sum"] = "0" if row["digest"]["sum"] != "0" else "1"
    count += rejected(changed_supports, "canonical complete supports changed", "supports changed with inheritance")

    def drifting_default(m):
        for row in select(m, 1, None, False):
            row["front_work"]["product_visits"] += 1
    count += rejected(drifting_default, "integer geometric/callback work changed", "plain window left the historical signature")

    def missing_variant(m):
        for row in select(m, 2, 16, True):
            m.remove(row)
    count += rejected(missing_variant, "incomplete inheritance comparison group", "incomplete comparison group")

    def added_candidates(m):
        top = max(r["candidate_pairs"] for r in select(m, 2, 16, False))
        for row in select(m, 2, 16, True):
            row["candidate_pairs"] = top + 1
    count += rejected(added_candidates, "inherited witnesses kept more than the same window alone: census candidates",
                      "inheritance added candidates")

    def more_rectangles(m):
        top = max(r["input_rectangles"] for r in select(m, 2, 16, False))
        for row in select(m, 2, 16, True):
            row["input_rectangles"] = top + 1
    count += rejected(more_rectangles, "inherited witnesses kept more than the same window alone: emitted rectangles",
                      "inheritance emitted more rectangles")

    def more_searches(m):
        top = max(r["front_work"]["witness_searches"] for r in select(m, 2, 16, False))
        for row in select(m, 2, 16, True):
            row["front_work"]["witness_searches"] = top + 1
    count += rejected(more_searches, "inherited witnesses kept more than the same window alone: witness searches",
                      "inheritance searched more products")

    def more_products(m):
        top = max(r["front_work"]["product_visits"] for r in select(m, 2, 16, False))
        for row in select(m, 2, 16, True):
            row["front_work"]["product_visits"] = top + 1
    count += rejected(more_products, "inherited witnesses kept more than the same window alone: visited products",
                      "inheritance visited more products")

    def less_rejected_mass(m):
        floor = min(r["front_work"]["rejected_pair_mass"][0] for r in select(m, 2, 16, False))
        for row in select(m, 2, 16, True):
            row["front_work"]["rejected_pair_mass"][0] = floor - 1
    count += rejected(less_rejected_mass, "inherited witnesses kept more than the same window alone: rejected pair mass",
                      "inheritance rejected less pair mass")

    def changed_accepted(m):
        for row in select(m, 4, None, True):
            row["accepted_pairs"] += 1
    count += rejected(changed_accepted, "a proposal option kept more than the default front: accepted supports",
                      "accepted supports changed")

    def beyond_default(m):
        for row in select(m, 1, None, True):
            row["candidate_pairs"] = first["candidate_pairs"] + 1
    count += rejected(beyond_default, "a proposal option kept more than the default front: census candidates",
                      "inheritance kept more candidates than the default front")

    def window_not_monotone(m):  # Both 2K rows, so that the twin comparison cannot see it first.
        top = max(r["candidate_pairs"] for r in select(m, 1, None, True))
        for row in (*select(m, 2, 16, False), *select(m, 2, 16, True)):
            row["candidate_pairs"] = top + 1
    count += rejected(window_not_monotone, "window 2K kept more than the historical window",
                      "window monotony broken under inheritance")

    def factor_not_monotone(m):  # The plain 4K row alone: its inheriting twin stays below it.
        top = max(r["candidate_pairs"] for r in select(m, 2, 16, False))
        for row in select(m, 4, None, False):
            row["candidate_pairs"] = top + 1
    count += rejected(factor_not_monotone, "unlimited window 4K kept more candidates than limited window 2K",
                      "factor monotony broken")

    def factor_not_monotone_inheriting(m):  # Both 4K rows: the twin comparison and the plain monotony still hold.
        top = max(r["candidate_pairs"] for r in select(m, 2, 16, True))
        for row in (*select(m, 4, None, False), *select(m, 4, None, True)):
            row["candidate_pairs"] = top + 1
    count += rejected(factor_not_monotone_inheriting,
                      "unlimited window 4K kept more candidates than limited window 2K under inheritance",
                      "factor monotony broken under inheritance")

    def thread_dependent(m):
        select(m, 2, 16, True)[0]["inheritance_work"]["inherited_duplicates"] += 1
    count += rejected(thread_dependent, "integer work changed across threads or repeat",
                      "inheritance work depends on the worker count")

    def vacuous(m):  # Every inheriting row equal to its plain twin; the upper-bound counter is kept positive.
        for row in m:
            if row.get("inherit_witnesses"):
                twin = next(r for r in m if r.get("inherit_witnesses") is False and
                            all(r[key] == row[key] for key in (*keys, "threads", "window_factor", "small_factor_limit")))
                row["candidate_pairs"] = twin["candidate_pairs"]
    count += rejected(vacuous, "no inherited witness removed any candidate: vacuous capture",
                      "vacuous capture although the rejection counter is positive")
    require(count == 15, "pair mutation inventory changed")
    return count


def capture_row(executable, command, kind):
    capture = subprocess.run([executable, *command], capture_output=True, text=True, timeout=60)
    require(capture.returncode == 0 and not capture.stderr, f"{kind} probe failed: {capture.stdout}\n{capture.stderr}")
    row = strict_json(capture.stdout)
    validate_kind(kind, row, [executable, *command])
    return row


def check_generic_floor(args):
    """The floor of the sizes of interest, never reached by a smoke capture: real rows at n = 256,
    judged as a scale capture. An inheriting row made equal to its twin must fall on that floor."""
    rows = []
    for family in ("uniform", "terrain", "clusters"):
        common = ["256", family, "5", "8", "3", "1", "1", "0"]
        rows.append(capture_row(args.reference, common, "reference"))
        rows.extend(capture_row(args.probe, [*common, *variant], "measure") for variant in VARIANTS)
    validate_pairs(rows, "scale")
    count = 0
    for mutate, label in (
            (lambda row, twin: row.update(candidate_pairs=twin["candidate_pairs"]), "twin without any removed candidate"),
            (lambda row, twin: row["inheritance_work"].update(inherited_duplicates=0), "twin without any received rank seen again"),
            (lambda row, twin: row["inheritance_work"].update(inherited_rejections=0), "twin without any inherited rejection")):
        mutated = deepcopy(rows)
        row = next(r for r in mutated if r.get("inherit_witnesses") and r["window_factor"] == 2)
        twin = next(r for r in mutated if r.get("inherit_witnesses") is False and r["window_factor"] == 2 and
                    r["family"] == row["family"])
        mutate(row, twin)
        try:
            validate_pairs(mutated, "scale")
        except REJECTIONS as cause:
            require("inherited witnesses removed no candidate on a generic cloud" in str(cause),
                    f"generic mutant {label} rejected for another reason: {cause}")
            count += 1
            continue
        raise RuntimeError(f"pair reader accepted generic mutant: {label}")
    try:
        validate_pairs(rows, "smoke")  # The same rows stay valid below the sizes of interest.
    except REJECTIONS as cause:
        raise RuntimeError(f"generic rows rejected as a smoke capture: {cause}")
    require(count == 3, "generic floor mutation inventory changed")
    return count


def check_differential(args):
    """The differential reader on REAL septuplets. The probes of the measured build stand in for the
    pinned tranche-20 build: what is judged here is the reader, not the previous engine."""
    rows = []
    for family, kmax in (("uniform", "10"), ("clusters", "5")):
        common = ["128", family, kmax, "8", "3", "1", "16", "64"]
        rows += [capture_row(args.reference, common, "pinned"), capture_row(args.reference, common, "reference"),
                 capture_row(args.probe, [*common, "1", "all", "0"], "measure"),
                 capture_row(args.proposals_probe, [*common, "2", "16"], "pinned_proposals"),
                 capture_row(args.probe, [*common, "2", "16", "0"], "measure"),
                 capture_row(args.proposals_probe, [*common, "4", "all"], "pinned_proposals"),
                 capture_row(args.probe, [*common, "4", "all", "0"], "measure")]
    validate_pairs(rows, "differential")
    require(rows[4]["extension_work"]["extended_rejections"] > 0 and rows[6]["extension_work"]["extended_rejections"] > 0,
            "differential septuplet without any widened rejection")

    def drifted(index, path, delta=1):
        def mutate(m):
            at(m[index], path[:-1])[path[-1]] += delta
        return mutate
    mutants = (
        (drifted(2, ("front_work", "product_visits")), "integer geometric/callback work changed", "default front drift"),
        (drifted(2, ("parallel_work", "prefix_product_visits")), "job plan of the default differs", "default job plan drift"),
        (drifted(4, ("census_work", "count_node_visits")), "pinned tranche-20 engine: census_work", "window 2K census drift"),
        (drifted(4, ("parallel_work", "jobs")), "pinned tranche-20 engine: parallel_work", "window 2K job plan drift"),
        (drifted(6, ("extension_work", "extended_credits")), "pinned tranche-20 engine: extension_work", "window 4K extension drift"),
        (drifted(6, ("pool_work", "bands")), "pinned tranche-20 engine: pool_work", "window 4K Pool drift"),
        (lambda m: m[3].pop("digest"), "proposals view lost or gained a field", "field lost by the proposals view"),
        (lambda m: m.pop(), "incomplete differential septuplets", "incomplete septuplet"),
        (lambda m: m.__setitem__(slice(3, 7), [m[5], m[6], m[3], m[4]]), "unexpected differential septuplet", "windows in the wrong order"),
        (lambda m: m[1].update(schema="mhgp8_wspd_q2_inheritance_probe_v1"), "unexpected differential septuplet", "reference replaced by a measure"))
    count = 0
    for mutate, message, label in mutants:
        mutated = deepcopy(rows)
        mutate(mutated)
        try:
            validate_pairs(mutated, "differential")
        except REJECTIONS as cause:
            require(message in str(cause), f"differential mutant {label} rejected for another reason: {cause}")
            count += 1
            continue
        raise RuntimeError(f"differential reader accepted mutant: {label}")
    require(count == 10, "differential mutation inventory changed")
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--reference", required=True)
    parser.add_argument("--proposals-probe", required=True,
                        help="tranche-20 proposals probe of the SAME build: stand-in of the pinned probe for the reader")
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    calls, groups, mutants, invalid = 0, 0, 0, 0
    unit_mutants = check_unit_contracts()
    mutants += unit_mutants
    covered_sizes, covered_pool, covered_k = set(), set(), set()
    pool_rows, extended_rows, rejecting_rows, duplicate_rows, gaining_twins = 0, 0, 0, 0, 0
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
                for factor, limit, inherit in VARIANTS:
                    command = [*common, factor, limit, inherit]
                    capture = subprocess.run([args.probe, *command], capture_output=True, text=True, timeout=30)
                    require(capture.returncode == 0 and not capture.stderr,
                            f"inheritance probe failed: {capture.stdout}\n{capture.stderr}")
                    row = strict_json(capture.stdout)
                    validate_row(row, command)
                    mutants += check_mutants(row, command, calls == 0)
                    calls += 1
                    pool_rows += row["pool_work"]["bands"] > 0
                    extended_rows += row["extension_work"]["extended_products"] > 0
                    rejecting_rows += row["inheritance_work"]["inherited_rejections"] > 0
                    # A measured effect against the plain twin captured just before.
                    gaining_twins += inherit == "1" and row["candidate_pairs"] < rows[-1]["candidate_pairs"]
                    duplicate_rows += row["inheritance_work"]["extended_inherited_duplicates"] > 0
                    rows.append(row)
                groups += 1
                covered_sizes.add(n)
                covered_pool.add(pool)
                covered_k.add(k)
    validate_pairs(rows, "smoke")
    pair_mutants = check_pair_mutants(rows)
    generic_mutants = check_generic_floor(args)
    differential_mutants = check_differential(args)
    mutants += pair_mutants + generic_mutants + differential_mutants
    labels = ("more received ranks than Kmax-1 per search", "more emitted credits than Kmax-1 per rectangle",
              "more inherited rejections than rejected products", "extension credits granted to received ranks",
              "odd number of received ranks", "received ranks outside the credit ledger",
              "emitted credits outside the credit ledger", "more duplicates than received ranks",
              "proposed rank outside the partition", "unbounded extension proposals",
              "historical proposals relabelled as extension")
    require(all(COMPENSATED.get(label, 0) > 0 for label in labels),
            "a compensated mutant never applied: " + repr({label: COMPENSATED.get(label, 0) for label in labels}))
    require(calls == 144 and groups == 24 and covered_sizes == {32, 64, 128} and
            covered_pool == {0, 2, 16, 64} and covered_k == {1, 5, 10} and
            pool_rows > 0 and extended_rows > 0 and rejecting_rows > 0 and duplicate_rows > 0 and gaining_twins > 0,
            "tiny inheritance matrix incomplete or vacuous")

    valid = ["32", "uniform", "5", "8", "3", "4", "1", "0", "2", "16", "1"]
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
    for value in ("2", "-1", "true", ""):
        command = valid.copy()
        command[10] = value
        invalid_commands.append(command)
    invalid_commands.append(["33", "rows", *valid[2:]])
    for command in invalid_commands:
        capture = subprocess.run([args.probe, *command], capture_output=True, text=True, timeout=30)
        require(capture.returncode == 2 and not capture.stdout and capture.stderr,
                f"invalid inheritance CLI accepted: {command}")
        invalid += 1
    require(invalid == 31, "invalid CLI inventory changed")
    print(json.dumps(dict(status="passed", real_captures=calls, comparison_groups=groups,
                          rejected_mutants=mutants, pair_mutants=pair_mutants, generic_floor_mutants=generic_mutants,
                          differential_mutants=differential_mutants, compensated_labels=len(COMPENSATED),
                          pool_captures=pool_rows, extended_captures=extended_rows,
                          rejecting_captures=rejecting_rows, extension_duplicate_captures=duplicate_rows,
                          gaining_twin_captures=gaining_twins,
                          unit_contract_mutants=unit_mutants, invalid_cli=invalid,
                          full_contract_qualified=False), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
