#!/usr/bin/env python3
"""Bounded spatial q3/q4 stream gate, independent rational geometry oracle.

Four regular tetrahedra in four sensor quadrants, seven complete datasets,
Individual/W1 versus LiveOnly/W2. No large scan, native build or cloud use.
The exact oracle enumerates supports only in this sixteen-site test fixture.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
from fractions import Fraction
import hashlib
from itertools import combinations, product
import json
import math
import os
from pathlib import Path
import signal
import struct
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / "morsehgp3D_v8/bench"
sys.path.insert(0, str(BENCH))
import analyze_q34_spatial as analysis
import prepare_lidar_spatial as preparation
from run_p0_matrix import invoke, on_signal, parse_result, utc_stamp, write_json

RUNNER = BENCH / "run_q34_spatial.py"
NAMES = ("full", "half_x_neg", "half_x_nonneg", "quarter_x_neg_y_neg",
         "quarter_x_neg_y_nonneg", "quarter_x_nonneg_y_neg", "quarter_x_nonneg_y_nonneg")
RELATIONS = (("full", "half_x_neg"), ("full", "half_x_nonneg"),
    ("half_x_neg", "quarter_x_neg_y_neg"), ("half_x_neg", "quarter_x_neg_y_nonneg"),
    ("half_x_nonneg", "quarter_x_nonneg_y_neg"), ("half_x_nonneg", "quarter_x_nonneg_y_nonneg"))
SOURCES = (RUNNER, BENCH / "analyze_q34_spatial.py", BENCH / "prepare_lidar_spatial.py",
           Path(__file__).resolve(), BENCH / "run_p0_matrix.py")


def require(condition, message):
    if not condition:
        raise RuntimeError("q34 spatial gate: " + message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pins(paths):
    return {str(path): digest(path) for path in paths}


def dot(a, b):
    return sum(x*y for x, y in zip(a, b, strict=True))


def subtract(a, b):
    return tuple(x-y for x, y in zip(a, b, strict=True))


def solve(matrix):
    """Gaussian elimination over Fraction, independent of native formulas."""
    rows = [[Fraction(value) for value in row] for row in matrix]
    dimension = len(rows)
    for column in range(dimension):
        pivot = next((i for i in range(column, dimension) if rows[i][column] != 0), None)
        if pivot is None:
            return None
        rows[column], rows[pivot] = rows[pivot], rows[column]
        divisor = rows[column][column]
        rows[column] = [value/divisor for value in rows[column]]
        for index in range(dimension):
            if index != column:
                multiple = rows[index][column]
                rows[index] = [a-multiple*b for a, b in zip(rows[index], rows[column], strict=True)]
    return tuple(rows[i][-1] for i in range(dimension))


def positive_ball(points, ids):
    anchor = points[ids[0]]
    directions = [subtract(points[i], anchor) for i in ids[1:]]
    weights = solve([[2*dot(a, b) for b in directions] + [dot(a, a)] for a in directions])
    if weights is None or any(value <= 0 for value in weights) or sum(weights) >= 1:
        return None
    center = tuple(Fraction(anchor[axis]) + sum(value*d[axis] for value, d in
        zip(weights, directions, strict=True)) for axis in range(3))
    relative = subtract(center, anchor)
    radius = dot(relative, relative)
    rational = (Fraction(1), *(-2*c for c in center), dot(center, center)-radius)
    denominator = math.lcm(*(value.denominator for value in rational))
    integers = tuple(value.numerator*(denominator//value.denominator) for value in rational)
    common = math.gcd(*integers)
    coefficients = tuple(value//common for value in integers)
    require(coefficients[0] > 0, "oracle key orientation")
    return center, radius, coefficients


def normalization(record):
    return (record["arity"], tuple(record["support"]), tuple(map(int, record["coefficients"])),
            record["depth"], tuple(record["shell"]))


def oracle(points, kmax, counts):
    triangles, payloads, result, groups = {}, {}, [], {}

    def payload(ball, ids):
        center, radius, coefficients = ball
        if coefficients not in payloads:
            powers = [dot(subtract(p, center), subtract(p, center))-radius for p in points]
            payloads[coefficients] = (sum(value < 0 for value in powers),
                                      [i for i, value in enumerate(powers) if value == 0])
            counts["census_sites"] += len(points)
            counts["balls"] += 1
        depth, shell = payloads[coefficients]
        require(set(ids) <= set(shell), "oracle support absent from shell")
        return dict(arity=len(ids), support=list(ids), coefficients=list(map(str, coefficients)),
                    depth=depth, shell=list(shell))

    for ids in combinations(range(len(points)), 3):
        counts["triangles"] += 1
        ball = positive_ball(points, ids)
        triangles[ids] = ball is not None
        if ball is not None:
            counts["positive_triangles"] += 1
            result.append(payload(ball, ids))
    for ids in combinations(range(len(points)), 4):
        counts["tetrahedra"] += 1
        ball = positive_ball(points, ids)
        if ball is None:
            continue
        counts["positive_tetrahedra"] += 1
        # Longest edge, ties by original local IDs. No product predicate or
        # native key/canonicalization helper is used by this oracle.
        owner = min(combinations(ids, 2), key=lambda edge:
            (-dot(subtract(points[edge[0]], points[edge[1]]),
                  subtract(points[edge[0]], points[edge[1]])), edge))
        possible = [x for x in ids if x not in owner and triangles[tuple(sorted((*owner, x)))]]
        require(possible, "positive tetrahedron lacks an acute face on longest edge")
        seed = min(possible)
        completion = next(x for x in ids if x not in owner and x != seed)
        key = (owner, seed, ball[2])
        if key not in groups or completion < groups[key][0]:
            groups[key] = (completion, payload(ball, ids))
    counts["canonical_q4_groups"] += len(groups)
    result.extend(value[1] for value in groups.values())
    result = [record for record in result if record["depth"] < kmax+2-record["arity"]]
    return sorted(result, key=normalization)


def raw_fixture():
    offsets = ((0.5, 0.5, 0.5), (0.5, -0.5, -0.5),
               (-0.5, 0.5, -0.5), (-0.5, -0.5, 0.5))
    rows = [(cx+x, cy+y, z, 0.5) for cx, cy in product((-4.0, 4.0), repeat=2)
            for x, y, z in offsets]
    return b"".join(struct.pack("<ffff", *row) for row in rows)


def independent_partitions(raw):
    def quantize(value):
        exact = 50*Fraction.from_float(value) + Fraction(65537, 2)
        return exact.numerator // exact.denominator
    full = sorted(set(tuple(quantize(value) for value in row[:3])
                      for row in struct.iter_unpack("<ffff", raw)))
    require(len(full) == 16, "fixture must have sixteen distinct quantized sites")
    result = {"full": full}
    for xside, xname in ((False, "neg"), (True, "nonneg")):
        half = [p for p in full if (p[0] >= 32768) == xside]
        result[f"half_x_{xname}"] = half
        for yside, yname in ((False, "neg"), (True, "nonneg")):
            quarter = [p for p in half if (p[1] >= 32768) == yside]
            result[f"quarter_x_{xname}_y_{yname}"] = quarter
            distances = {dot(subtract(a, b), subtract(a, b)) for a, b in combinations(quarter, 2)}
            require(len(quarter) == 4 and len(distances) == 1, "quarter is not one regular tetrahedron")
    return result


def analysis_checks():
    checks = 0

    def check(value, why):
        nonlocal checks
        require(value, "analysis: " + why)
        checks += 1

    def rejected(function, why):
        nonlocal checks
        try:
            function()
        except (ValueError, TypeError, KeyError):
            checks += 1
            return
        raise RuntimeError("q34 spatial gate: analyzer accepted " + why)

    exact = analysis.growth(7, 3, 49, 9)
    check(exact["size_ratio"] == 7/3 and exact["quadratic_ratio"] == 49/9 and
          exact["work_ratio"] == 49/9 and exact["quadratic_relation"] == "equal" and
          exact["below_quadratic"] is False and math.isclose(exact["empirical_exponent"], 2.0),
          "real non-doubling exact quadratic ratio")
    check(analysis.growth(7, 3, 50, 9)["quadratic_relation"] == "above", "above quadratic")
    check(analysis.growth(7, 3, 48, 9)["quadratic_relation"] == "below", "below quadratic")
    # Compare the integer boundary exactly even if float ratios coincide.
    large = 1 << 60
    check(analysis.growth(7, 3, 49*large+1, 9*large)["quadratic_relation"] == "above", "integer precision")
    for args, reason in (((7, 0, 49, 0), "empty_child"), ((3, 3, 9, 9), "equal_sizes"),
                         ((7, 3, 0, 9), "zero_work"), ((7, 3, 49, 0), "zero_work")):
        value = analysis.growth(*args)
        check(value["non_estimable"] == reason and value["empirical_exponent"] is None,
              "non-estimable case " + reason)
    for args in ((True, 0, 1, 1), (7, False, 1, 1), (7, 3, True, 1), (7, 3, 1, False),
                 (-1, 0, 1, 1), (3, 7, 1, 1), (7.0, 3, 1, 1), (7, 3, -1, 1),
                 (7, 3, float("inf"), 1), (7, 3, 1, float("nan"))):
        rejected(lambda args=args: analysis.growth(*args), "invalid growth arguments")
    check(tuple(analysis.RELATIONS) == RELATIONS, "six original tree links")
    sizes = dict(zip(NAMES, (16, 8, 8, 4, 4, 4, 4), strict=True))
    configuration = dict(kmax=5, s=8, mask=6, q4_backend=28, workers=1, front_mode="samples",
        output_mode="records", witness_mode="rectangle-pair", q3_census_mode="boxes",
        witness_bounds_mode="affine", q4_seed_mode="individual", q4_seed_block_size=64)
    entries = []
    for name in NAMES:
        n = sizes[name]
        row = dict(configuration, n=n, cloud_work={"visits": n}, index_work={}, work={"tests": n*n},
            parallel={}, memory={"retained_bytes": 10*n}, front={"work": {}},
            output=dict(callbacks=n, q3=n, q4=0, support_ids=3*n, shell_ids=3*n), timings_ms={"phase": float(n)})
        entries.append(dict(dataset=name, repeat=0, status="completed", row=row))
    result = analysis.analyze(entries)
    check(len(result["comparisons"]) == 6 and {(v["parent"], v["child"]) for v in result["comparisons"]} ==
          set(RELATIONS), "all six links once")
    check(all(v["metrics"]["work.tests"]["quadratic_relation"] == "equal" for v in result["comparisons"]),
          "quadratic comparisons on seven pieces")
    levels = result["level_sums"][0]["levels"]
    check([levels[level]["work.tests"] for level in ("full", "halves", "quarters")] == [256, 128, 64],
          "three level sums are not a sorted size series")
    check(all("memory.retained_bytes" not in values for values in levels.values()), "capacity not summed as work")
    rejected(lambda: analysis.analyze(entries[:-1]), "missing seventh piece")
    rejected(lambda: analysis.analyze(entries + [entries[0]]), "duplicate piece")
    changed = deepcopy(entries)
    changed[-1]["row"]["n"] = 3
    rejected(lambda: analysis.analyze(changed), "invalid cardinality partition")
    changed = deepcopy(entries)
    changed[-1]["row"]["s"] = 10
    rejected(lambda: analysis.analyze(changed), "different configurations")
    changed = deepcopy(entries)
    changed[-1]["status"] = "failed"
    rejected(lambda: analysis.analyze(changed), "unfinished observation")
    rejected(lambda: analysis.analyze([]), "empty observation set")
    return checks


def run_gate(build, output):
    build = build.resolve()
    binaries = tuple(build / name for name in ("mhgp8_wspd_q34_probe", "libmhgp8_p0.a", "CMakeCache.txt"))
    output.mkdir(parents=True, exist_ok=True)
    capture = Path(tempfile.mkdtemp(prefix="q34_spatial_gate_", dir=output.resolve()))
    environment = dict(os.environ)
    environment["PYTHONDONTWRITEBYTECODE"] = "1"
    python = [sys.executable, "-B", *(["-O"] if sys.flags.optimize else [])]
    state = dict(schema="mhgp8_q34_spatial_gate_capture_v1", status="running", error=None,
        scope="sixteen_site_exhaustive_spatial_stream_oracle_not_full_or_performance",
        started_utc=utc_stamp(), launch=[sys.executable, *sys.argv], build=str(build),
        optimized=bool(sys.flags.optimize), commands=[], captures=[], results=[],
        oracle=dict(clouds=0, triangles=0, positive_triangles=0, tetrahedra=0,
                    positive_tetrahedra=0, canonical_q4_groups=0, balls=0, census_sites=0),
        native_calls=0, compared_records=0, analysis_checks=0,
        source_sha256={}, artifact_sha256={}, closing_errors=[], gcp_used=False,
        full_contract_qualified=False)
    handlers = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}

    def command(arguments):
        record = dict(command=list(map(str, arguments)), cwd=str(ROOT), started_utc=utc_stamp(),
            status="failed", exit_code=None, stdout="", stderr="", stdout_base64="", stderr_base64="")
        state["commands"].append(record)
        try:
            invoke(record["command"], environment, ROOT, record, new_session=True)
            require(record["exit_code"] == 0 and not record["stderr"], "runner command failed")
            record["status"] = "passed"
            return record["stdout"]
        finally:
            record["finished_utc"] = utc_stamp()
            write_json(capture / f"command_{len(state['commands'])-1:02}.json", record)

    try:
        state["source_sha256"] = pins(SOURCES)
        state["artifact_sha256"] = pins(binaries)
        snapshot = capture / "sources"
        snapshot.mkdir()
        for source in SOURCES:
            data = source.read_bytes()
            require(hashlib.sha256(data).hexdigest() == state["source_sha256"][str(source)], "source changed before snapshot")
            (snapshot / source.name).write_bytes(data)
        write_json(capture / "MANIFEST.json", state)
        state["analysis_checks"] = analysis_checks()
        raw = raw_fixture()
        raw_path = capture / "scan.bin"
        raw_path.write_bytes(raw)
        state["input_sha256"] = hashlib.sha256(raw).hexdigest()
        expected_points = independent_partitions(raw)
        prepared = capture / "prepared"
        state["preparation"] = preparation.prepare(raw_path, prepared)
        state["preparation_read"] = preparation.read(prepared)
        expected = {}
        for name in NAMES:
            points = list(struct.iter_unpack("<HHH", (prepared / (name + ".u16le")).read_bytes()))
            require(points == expected_points[name], "prepared piece differs from independent exact quantization/partition")
            state["oracle"]["clouds"] += 1
            expected[name] = oracle(points, 5, state["oracle"])
            require(any(row["arity"] == 3 for row in expected[name]) and
                    any(row["arity"] == 4 for row in expected[name]), "vacuous q3/q4 piece oracle")
            if name.startswith("quarter_"):
                require(len(expected[name]) == 5 and sum(row["arity"] == 4 for row in expected[name]) == 1,
                        "regular tetrahedron must emit four faces and one q4 support")
        state["oracle_records"] = expected
        first_records = {}
        for workers, mode in ((1, "individual"), (2, "live")):
            run_command = [*python, RUNNER, "run", "--prepared", prepared, "--build", build,
                "--output", capture / "native", "--kmax", "5", "--s", "8", "--workers", str(workers),
                "--q4-seed-mode", mode, "--repeats", "1", "--payload", "records"]
            lines = [json.loads(line) for line in command(run_command).splitlines() if line.strip()]
            require(lines and lines[-1].get("status") == "passed" and isinstance(lines[-1].get("path"), str),
                    "runner launch did not report a closed passing capture")
            native = Path(lines[-1]["path"]).resolve()
            require(native.is_relative_to(capture / "native") and native.is_dir(), "native capture escaped fixture")
            state["captures"].append(str(native))
            read_command = [*python, RUNNER, "read", "--path", native, "--check-live"]
            proof = parse_result(command(read_command).encode())
            require(proof["status"] == "passed" and len(proof["records"]) == 7, "reader did not close all seven pieces")
            seen = set()
            for entry in proof["records"]:
                name = entry["dataset"]
                require(name in NAMES and name not in seen and entry["repeat"] == 0 and
                        entry["status"] == "completed" and entry["row"] is not None,
                        "native dataset/repetition inventory differs")
                seen.add(name)
                row = entry["row"]
                require(row["n"] == row["source_n"] == len(expected_points[name]) and
                        row["workers"] == workers and row["q4_seed_mode"] == mode and row["kmax"] == 5 and
                        row["s"] == 8 and row["output_mode"] == "records", "native command identity differs")
                require(row["records"] == expected[name],
                        "spatial stream differs from independent rational support/depth/shell oracle: " + name)
                if mode == "individual":
                    first_records[name] = row["records"]
                else:
                    require(row["records"] == first_records[name], "Individual/LiveOnly streams differ")
                state["native_calls"] += 1
                state["compared_records"] += len(row["records"])
            require(seen == set(NAMES), "missing spatial piece")
            growth = analysis.analyze(proof["records"])
            require(len(growth["comparisons"]) == 6 and
                    {(v["parent"], v["child"]) for v in growth["comparisons"]} == set(RELATIONS),
                    "actual capture does not yield exactly six parent/child relations")
            state["results"].append(dict(workers=workers, mode=mode, capture=str(native),
                records=sum(len(entry["row"]["records"]) for entry in proof["records"]),
                growth_comparisons=len(growth["comparisons"])))
        require(state["native_calls"] == 14 and state["oracle"]["clouds"] == 7 and
                state["compared_records"] > 0, "incomplete/vacuous bounded gate")
        require(digest(raw_path) == state["input_sha256"], "fixture changed during gate")
        state["status"] = "passed"
    except BaseException as error:
        state["status"] = "failed"
        state["error"] = f"{type(error).__name__}: {error}"
        raise
    finally:
        for sig in handlers:
            signal.signal(sig, signal.SIG_IGN)
        for key in ("source_sha256", "artifact_sha256"):
            after = {}
            for name, original in state[key].items():
                try:
                    after[name] = digest(Path(name))
                    if after[name] != original:
                        state["closing_errors"].append("changed pin: " + name)
                except OSError as error:
                    state["closing_errors"].append(f"{name}: {error}")
            state[key + "_after"] = after
        state["capture_artifacts"] = {}
        for path in sorted(capture.rglob("*")):
            if path.is_file() and path != capture / "COMPLETION.json":
                try:
                    state["capture_artifacts"][str(path.relative_to(capture))] = digest(path)
                except OSError as error:
                    state["closing_errors"].append(f"{path}: {error}")
        if state["closing_errors"]:
            state["status"] = "failed"
            state["error"] = state["error"] or "source/artifact closure changed"
        state["finished_utc"] = utc_stamp()
        write_json(capture / "COMPLETION.json", state)
        print(json.dumps(dict(schema="mhgp8_q34_spatial_gate_v1", status=state["status"], path=str(capture),
            native_calls=state["native_calls"], compared_records=state["compared_records"],
            analysis_checks=state["analysis_checks"], oracle=state["oracle"], error=state["error"],
            optimized=state["optimized"], gcp_used=False, full_contract_qualified=False), sort_keys=True), flush=True)
        for sig, handler in handlers.items():
            signal.signal(sig, handler)
    require(state["status"] == "passed", "gate did not close")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    run_gate(args.build, args.output)


if __name__ == "__main__":
    main()
