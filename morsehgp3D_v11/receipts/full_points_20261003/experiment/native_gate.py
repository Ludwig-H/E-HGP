#!/usr/bin/env python3
"""Bounded G4 gate for the audit points export; no native compilation here.

The frozen A oracle is exhaustive Gamma_k, B is the constructive catalogue.
Native node numbers are mapped by exact birth balls, never by a Morton/ID
convention. All strong populations, their closed owners, dynamic covers and
core entries are checked. The first canonical cover is only one allowed choice.
Success fixtures have at most eight sites, except the explicit twelve-site
cuboctahedron at K=10 (at most 4096 subsets in the definition oracle).
"""
from __future__ import annotations

import argparse
from collections import Counter
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
FROZEN = HERE.parent / "projection" / "source"
sys.path.insert(0, str(FROZEN))
from hgp11_ref import Definition, Reference, judge
from hgp11_ref.families import Cloud, fixtures
from qualified import NONE, load

WORKERS = 48
BUDGET = 1 << 30
MAX_SECONDS = 270
EXACT = [(1, 1, 2), (1, 2, 1), (2, 2, 2), (3, 3, 2), (4, 4, 2), (4, 3, 3)]
BOUNDARY = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
CUBOCTAHEDRON = ([(x+2, y+2, 2) for x in (-1, 1) for y in (-1, 1)] +
                [(x+2, 2, z+2) for x in (-1, 1) for z in (-1, 1)] +
                [(2, y+2, z+2) for y in (-1, 1) for z in (-1, 1)])
CUBOCTAHEDRON_NAME = "cuboctahedron12_k10"
QUICK = {"e5", "line024", "square", "tetra_center", "double_collision",
         "two_triangles_1998", "equilateral_exact", "shared_boundary",
         "pair_weighted", "all_equal", CUBOCTAHEDRON_NAME}


class Checks:
    def __init__(self):
        self.counts = Counter()

    def require(self, condition, domain, message):
        self.counts[domain] += 1
        if not condition:
            raise ValueError(domain + ": " + message)


def digest(path):
    value = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            value.update(block)
    return value.hexdigest()


def source_hashes(probe=None):
    paths = [Path(__file__).resolve(), HERE / "qualified.py"]
    paths += sorted((FROZEN / "hgp11_ref").glob("*.py"))
    if probe is not None:
        paths.append(probe)
    return {str(path): digest(path) for path in paths}


def bitmask(indices):
    return sum(1 << i for i in indices)


def audit_dump(data, points, ids, kmax, checks, expected_bits=21):
    """Geometric gate independent of the adapter's structural validation."""
    require = checks.require
    n = len(points)
    require(data.bits == expected_bits and data.kmax == kmax and data.sites == n,
            "input", "profile/K/site count")
    rows = {point_id: i for i, point_id in enumerate(ids)}
    require(len(rows) == n and set(data.points[3::4]) == set(rows), "input", "PointId bijection")
    to_input = [rows[point_id] for point_id in data.points[3::4]]
    for site, original in enumerate(to_input):
        require(tuple(data.points[4*site:4*site+3]) == tuple(points[original]),
                "input", "XYZ/PointId correspondence")
    definition, reference = Definition(points), Reference(points, kmax)
    require(data.levels == sorted({ball.level for ball in reference.balls}),
            "levels", "complete exact admitted catalogue levels")
    details, previous_a, previous_b = [], None, None
    for k in range(1, kmax+1):
        a, b = definition.order(k), reference.order(k)
        require(judge.coherence(a, n, previous_a) is None, "oracle", "A coherence")
        require(judge.coherence(b, n, previous_b) is None, "oracle", "B coherence")
        require(not judge.compare_orders(a, b), "oracle", "A/B disagreement")
        previous_a, previous_b = a, b
        native = data.orders[k]
        parents = judge.parents(a.nodes)
        births = {(node.level, node.center): i for i, node in enumerate(a.nodes)
                  if not node.children}
        require(native.births == len(births) and native.count == len(a.nodes),
                "forest", "birth/node count")
        expected = Counter()
        for ball in reference.balls:
            if ball.p + ball.qmin <= k <= ball.p + ball.m:
                population = tuple(sorted(reference.inp[i] for i in ball.inner + ball.shell))
                expected[(ball.level, bitmask(population))] += 1
        records = []
        mapping = {}
        actual = Counter()
        for node, rank, sites in native.records():
            population = tuple(sorted(to_input[site] for site in sites))
            level = data.levels[rank]
            mask = bitmask(population)
            require(len(population) >= k, "records", "population below K")
            beta, center, closed = definition.meb(population)
            require(beta == level and closed == mask, "records", "MEB level or complete I/U")
            records.append((node, level, mask, population))
            actual[(level, mask)] += 1
            # An extended shell can give a birth with population > k (square,
            # k=3). Its strong record at its birth date identifies the same
            # unique birth ball; requiring cardinality == k would miss it.
            if node < native.births and data.levels[native.rank[node]] == level:
                key = (level, center)
                require(key in births, "forest", "birth ball absent in Gamma")
                require(node not in mapping or mapping[node] == births[key],
                        "forest", "birth has two balls")
                mapping[node] = births[key]
        require(actual == expected, "records", "all strong records exactly once, full population")
        require(len(mapping) == native.births, "forest", "every birth has its strong witness at birth")
        merges = {(node.level, node.children): i for i, node in enumerate(a.nodes) if node.children}
        for node in range(native.births, native.count):
            children = tuple(sorted(mapping[child] for child in native.children(node)))
            key = (data.levels[native.rank[node]], children)
            require(key in merges, "forest", "exact multifusion plateau/children")
            mapping[node] = merges[key]
        require(len(set(mapping.values())) == native.count, "forest", "node bijection")
        require(mapping[native.root] == next(i for i, parent in enumerate(parents) if parent == -1),
                "forest", "root correspondence")
        for node in range(native.count):
            parent = native.parent[node]
            require((-1 if parent == NONE else mapping[parent]) == parents[mapping[node]],
                    "forest", "exact parent")
            require(data.levels[native.rank[node]] == a.nodes[mapping[node]].level,
                    "forest", "exact node level")
        for node, level, _mask, population in records:
            owner = definition.node_at(k, population[:k], level)
            require(mapping[node] == owner, "records", "closed Gamma owner of complete population")
        for site, original in enumerate(to_input):
            distances = sorted(sum((x-y)**2 for x, y in zip(points[original], other))
                               for other in points)
            require(native.core_date[site] == distances[k-1] == a.core[original].level,
                    "core", "integer kth distance including self")
            require(mapping[native.core_node[site]] == a.core[original].nodes,
                    "core", "core owner at exact dk squared, possibly outside catalogue ranks")
            first_date = data.levels[native.first_rank[site]]
            require(first_date == a.cover[original].level, "first_cover", "first date")
            tied = {mapping[node] for node, date, mask, _population in records
                    if date == first_date and mask & (1 << original)}
            require(tied == a.cover[original].nodes, "first_cover", "all simultaneous covering owners")
            require(mapping[native.first_node[site]] in tied,
                    "first_cover", "canonical choice must belong to full set, no unique-truth claim")

        # Complete records accumulate on their current CLOSED ancestor. This is
        # the dynamic X intersect dilation relation, never a static first attach.
        dates = sorted(set(data.levels) | {cut.level for cut in a.cuts} |
                       {entry.level for entry in a.core})
        for date in dates:
            cover = {node: 0 for node in range(native.count)
                     if data.levels[native.rank[node]] <= date and
                     (native.parent[node] == NONE or
                      data.levels[native.rank[native.parent[node]]] > date)}

            def ancestor(node):
                while native.parent[node] != NONE and data.levels[native.rank[native.parent[node]]] <= date:
                    node = native.parent[node]
                return node

            core = dict.fromkeys(cover, 0)
            for node, level, mask, _population in records:
                if level <= date:
                    cover[ancestor(node)] |= mask
            for site, original in enumerate(to_input):
                if native.core_date[site] <= date:
                    core[ancestor(native.core_node[site])] |= 1 << original
            actual_cut = {mapping[node]: (mask, core[node]) for node, mask in cover.items()}
            expected_cut = {node: (cov, cor) for node, cov, cor in judge.cut_at(a, date)[1]}
            require(actual_cut == expected_cut, "closed_cuts", "all components/dynamic covers/core")
        if k == 10 and list(points) == CUBOCTAHEDRON:
            require(native.births == native.count == 1 and data.levels[native.rank[native.root]] == 2,
                    "k10_witness", "single extended K10 birth at beta=2")
            require(len(records) == 1 and records[0][2] == (1 << 12)-1,
                    "k10_witness", "entire twelve-site shell retained")
            require(all(date == 6 for date in native.core_date) and Fraction(6) not in data.levels,
                    "k10_witness", "tenth neighbour at dk squared=6 outside catalogue ranks")
        details.append(dict(k=k, nodes=native.count, births=native.births,
                            strong_records=len(records), closed_cuts=len(dates),
                            first_cover_ties=sum(len(entry.nodes) > 1 for entry in a.cover)))
    return details


def cases(quick=False):
    result = fixtures()
    result += [Cloud("equilateral_exact", EXACT, 3), Cloud("shared_boundary", BOUNDARY, 3)]
    result.append(Cloud(CUBOCTAHEDRON_NAME, CUBOCTAHEDRON, 10))
    return [case for case in result if not quick or case.name in QUICK]


def invoke(probe, directory, points, kmax, deadline, budget=BUDGET, existing=False, duplicate_ids=False):
    directory.mkdir(parents=True, exist_ok=False)
    xyz, id_path, output = (directory / name for name in ("sites.u32le", "ids.u32le", "points.bin"))
    # UINT32_MAX is a legal external PointId, distinct from the dense SiteIdx sentinel.
    ids = [NONE] + [17 + 104729*i for i in range(1, len(points))]
    if duplicate_ids:
        ids[1] = ids[0]
    xyz.write_bytes(b"".join(struct.pack("<III", *point) for point in points))
    id_path.write_bytes(b"".join(struct.pack("<I", point_id) for point_id in ids))
    marker = b"existing-output-must-survive\n"
    if existing:
        output.write_bytes(marker)
    command = [str(probe), str(xyz), str(id_path), str(output), str(kmax), str(WORKERS), str(budget)]
    remaining = deadline - time.monotonic()
    if remaining <= 0:
        raise TimeoutError("bounded gate deadline")
    began = time.monotonic()
    try:
        result = subprocess.run(command, capture_output=True, timeout=min(20, remaining))
    except subprocess.TimeoutExpired as error:
        (directory / "native.stdout").write_bytes(error.stdout or b"")
        (directory / "native.stderr").write_bytes(error.stderr or b"")
        raise
    (directory / "native.stdout").write_bytes(result.stdout)
    (directory / "native.stderr").write_bytes(result.stderr)
    stats = json.loads(result.stdout)
    if not isinstance(stats, dict):
        raise ValueError("native JSON is not an object")
    return result, stats, output, ids, marker, time.monotonic()-began


def require_rejected_payload(payload, checks, message):
    try:
        data = load(payload)
    except (ValueError, RuntimeError, struct.error):
        checks.require(True, "decoder_refusal", message)
    else:
        data.close()
        checks.require(False, "decoder_refusal", message)


def gate(probe, work, quick, checks):
    rows = []
    deadline = time.monotonic() + MAX_SECONDS
    for fixture in cases(quick):
        row = dict(name=fixture.name, returns=len(fixture.points), kmax=fixture.kmax)
        try:
            weighted = len(set(fixture.points)) != len(fixture.points)
            if not weighted:
                explicit_k10 = (fixture.name == CUBOCTAHEDRON_NAME and
                                fixture.points == CUBOCTAHEDRON and fixture.kmax == 10)
                checks.require(len(fixture.points) <= 8 or explicit_k10,
                               "scope", "bounded exact-oracle fixture or explicit twelve-site K10")
            result, stats, output, ids, _marker, seconds = invoke(
                probe, work / fixture.name, fixture.points, fixture.kmax, deadline)
            row.update(native_exit=result.returncode, native_wall_seconds=seconds,
                       native_status=stats.get("status"), native_reason=stats.get("reason"))
            if weighted:
                checks.require(result.returncode != 0 and stats.get("status") != "ok" and
                               stats.get("reason") == "multiplicity_unsupported",
                               "native_refusal", "weighted cloud typed refusal")
                checks.require(not output.exists() and not Path(str(output)+".pending").exists(),
                               "transaction", "no partial/final dump on refusal")
                row.update(status="pass", scope="multiplicity refusal only; no weighted FULL")
            else:
                checks.require(result.returncode == 0 and stats.get("status") == "ok",
                               "native", "positive fixture success")
                checks.require(stats.get("coord_bits") == 21 and stats.get("workers") == WORKERS and
                               stats.get("optimizations") == 16379 and stats.get("kmax") == fixture.kmax,
                               "native", "qualified reference profile/options")
                checks.require(output.is_file() and not Path(str(output)+".pending").exists(),
                               "transaction", "only final closed dump")
                data = load(output)
                try:
                    row["orders"] = audit_dump(data, fixture.points, ids, fixture.kmax, checks)
                finally:
                    data.close()
                payload = output.read_bytes()
                require_rejected_payload(payload[:-1], checks, "truncated final population")
                require_rejected_payload(payload+b"x", checks, "trailing output")
                row.update(status="pass", dump_bytes=len(payload), dump_sha256=digest(output),
                           scope="native FULL, all strong populations, dynamic cover/core, first-cover ties")
        except Exception as error:
            row.update(status="fail", error=type(error).__name__ + ": " + str(error))
        rows.append(row)
    checks.require(len(rows) == len(cases(quick)), "scope", "all selected fixtures attempted")
    for name, budget, existing, duplicate_ids, reason in (
            ("budget_refusal", 1, False, False, "memory_budget"),
            ("output_conflict", BUDGET, True, False, "output_conflict"),
            ("duplicate_ids", BUDGET, False, True, "duplicate_point_id")):
        row = dict(name=name, scope="transactional refusal")
        try:
            result, stats, output, _ids, marker, seconds = invoke(
                probe, work / name, [(0, 0, 0), (2, 0, 0)], 2, deadline,
                budget, existing, duplicate_ids)
            row.update(native_exit=result.returncode, native_wall_seconds=seconds,
                       native_status=stats.get("status"), native_reason=stats.get("reason"))
            checks.require(result.returncode != 0 and stats.get("status") != "ok",
                           "native_refusal", name)
            if reason is not None:
                checks.require(stats.get("reason") == reason, "native_refusal", name + " typed reason")
            checks.require((output.read_bytes() == marker if existing else not output.exists()) and
                           not Path(str(output)+".pending").exists(), "transaction", name)
            row["status"] = "pass"
        except Exception as error:
            row.update(status="fail", error=type(error).__name__ + ": " + str(error))
        rows.append(row)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--quick", action="store_true", help="eleven fixtures including K10, plus three refusals")
    args = parser.parse_args()
    began = time.monotonic()
    answer = dict(schema="mhgp11.points.native_gate.v1", status="fail", quick=args.quick,
                  coord_bits=21, workers=WORKERS, optimizations=16379,
                  budget_bytes=BUDGET, timeout_budget_seconds=MAX_SECONDS,
                  scope="bounded synthetic exact gate; no Zoltan data, FULL massifs or statistical qualification",
                  frozen_fixture_count=34, weighted_scope="six rejection cases, including 14-return octahedron",
                  selected_fixture_count=len(cases(args.quick)),
                  success_oracle_domain="n<=8, except cuboctahedron12_k10: n=12, K=10, <=4096 subsets",
                  errors=[], cases=[])
    checks = Checks()
    before = None
    try:
        probe = args.probe.resolve(strict=True)
        checks.require(probe.is_file(), "setup", "probe executable file")
        checks.require(not args.work.exists(), "setup", "fresh private work directory required")
        checks.require(not args.out.exists(), "setup", "fresh result file required")
        before = source_hashes(probe)
        args.work.mkdir(parents=True, exist_ok=False)
        answer["cases"] = gate(probe, args.work, args.quick, checks)
        after = source_hashes(probe)
        answer.update(sources_before=before, sources_after=after)
        checks.require(before == after, "provenance", "gate/decoder/oracle/probe unchanged during run")
        answer["errors"] = [row["name"] + ": " + row.get("error", "failed")
                            for row in answer["cases"] if row["status"] != "pass"]
        answer["status"] = "pass" if not answer["errors"] else "fail"
    except Exception as error:
        answer["errors"].append(type(error).__name__ + ": " + str(error))
        if before is not None:
            answer["sources_before"] = before
    answer.update(checks=dict(sorted(checks.counts.items())), check_count=sum(checks.counts.values()),
                  wall_seconds=time.monotonic()-began)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    # No half-written result publication. Preserve any previous result instead
    # of replacing it when the fresh-output precondition itself was violated.
    if args.out.exists():
        print(json.dumps(dict(status="fail", errors=answer["errors"]), sort_keys=True))
        return 1
    pending = args.out.with_name(args.out.name + ".pending")
    with pending.open("x") as stream:
        json.dump(answer, stream, sort_keys=True, indent=2, allow_nan=False)
        stream.write("\n")
    pending.replace(args.out)
    # {out} is private to this worker command; later commands see the shared
    # build directory. A failed gate is copied too and explicitly refused.
    if args.work.is_dir():
        shared = args.work / "gate.json"
        with shared.open("x") as stream:
            json.dump(answer, stream, sort_keys=True, indent=2, allow_nan=False)
            stream.write("\n")
    print(json.dumps(dict(status=answer["status"], cases=len(answer["cases"]),
                          checks=answer["check_count"], errors=answer["errors"]), sort_keys=True))
    return 0 if answer["status"] == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
