"""Replay existing Python judges on archived outputs; no compiler or C++ run.

Usage: python3 [-O] morsehgp3D_v7/audits/replay_compiled_front.py
Only replay_normal.json or replay_optimized.json is written under audits/.
"""

from __future__ import annotations

from datetime import datetime, timezone
from fractions import Fraction
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
import time
from typing import Any


sys.dont_write_bytecode = True
AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parent.parent
RECEIPTS = AUDIT / "receipts_front_compiled_20260905"
INPUT_HASHES: dict[str, str] = {}


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise RuntimeError(reason)


def dot_exact(first: Any, second: Any) -> int:
    require(len(first) == len(second) == 3, "cell.basis_dimension")
    return sum(x * y for x, y in zip(first, second))


def read(path: Path) -> str:
    data = path.read_bytes()
    INPUT_HASHES[str(path.relative_to(AUDIT))] = hashlib.sha256(data).hexdigest()
    return data.decode("utf-8")


def load(path: Path) -> Any:
    return json.loads(read(path))


def load_judge(name: str) -> Any:
    path = AUDIT / (name + ".py")
    read(path)
    spec = importlib.util.spec_from_file_location("front_replay_" + name, path)
    require(spec is not None and spec.loader is not None, "judge.import_spec")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check_judge_pins(pins: dict[str, str], names: list[str]) -> None:
    for name in names:
        path = AUDIT / name
        read(path)
        require(INPUT_HASHES[name] == pins[str(path.relative_to(ROOT))], "judge.changed:" + name)


def archived_rows(record: dict[str, Any]) -> list[Any]:
    require(record["exit_code"] == 0 and not record["stderr"], "archived.bridge_failed")
    return [json.loads(line) for line in record["stdout"].splitlines()]


def expect_rejection(action: Any, exception: type[Exception], reason: str) -> None:
    try:
        action()
    except exception as error:
        require(str(error) == reason, "wrong_mutant_rejection:" + str(error))
        return
    raise RuntimeError("mutant_not_rejected:" + reason)


def replay_sector() -> dict[str, Any]:
    base = RECEIPTS / "secteur_corde"
    receipt = load(base / "receipt.json")
    require(receipt["status"] == "passed", "sector.receipt_status")
    check_judge_pins(receipt["sources"], ["secteur_corde_compiled_runner.py", "meb_rational_oracle_20260905.py"])
    judge = load_judge("secteur_corde_compiled_runner")
    commands, metadata = judge.corpus()
    result = {}
    for label in ("o2", "ubsan"):
        record = load(base / (label + ".json"))
        require(record["status"] == "passed" and record["compile_exit_code"] == 0, "sector.build_status")
        require(record["commands"] == commands, "sector.archived_input_matches_corpus")
        rows = archived_rows(record["nominal"])
        require(len(rows) == len(metadata), "sector.response_count")
        totals = judge.judge(rows, metadata)
        require(totals == record["counts"] and all(value > 0 for value in totals.values()), "sector.nonvacuity_counts")
        require(totals["basis_cases"] == 736 and totals["sqrt_cases"] == 212 and
                totals["affine_cases"] == 96 and totals["sector_cases"] == 3 and
                totals["chord_cases"] == 156, "sector.documented_floor")
        require(record["product_mutants_killed"] == ["sector-kill-nonstrict"] and
                record["audit_parameter_faults_killed"] == ["chord-nonstrict-parameter"], "sector.mutation_kind")
        sector_mutant = archived_rows(record["sector_product_mutant"])
        require(len(sector_mutant) == 1 and metadata[-2][0] == "T", "sector.mutant_cardinality")
        altered = rows.copy()
        altered[-2] = sector_mutant[0]
        expect_rejection(lambda: judge.judge(altered, metadata), RuntimeError, "sector.direct_ball_reference")
        chord_base = archived_rows(record["chord_parameter_nominal"])
        chord_bad = archived_rows(record["chord_parameter_fault"])
        require(len(chord_base) == len(chord_bad) == 1, "chord.fault_cardinality")
        judge.judge_chord(chord_base[0], 8, 1, -12, totals.copy())
        expect_rejection(lambda: judge.judge_chord(chord_bad[0], 8, 1, -12, totals.copy()),
                         RuntimeError, "chord.endpoint_counts")
        result[label] = {"counts": totals, "product_mutants_rejected": 1,
                         "audit_parameter_faults_rejected": 1}
    require(result["o2"] == result["ubsan"], "sector.builds_differ")
    return result


def replay_cell() -> dict[str, Any]:
    base = RECEIPTS / "cell"
    receipt = load(base / "summary.json")
    require(receipt["sources_stable"] and receipt["cases"] == 60, "cell.receipt_status")
    check_judge_pins(receipt["source_pins"], ["cell_compiled_oracle.py", "meb_rational_oracle_20260905.py"])
    judge = load_judge("cell_compiled_oracle")
    dataset = judge.cases()
    require(len(dataset) == 60 and read(base / "input.txt") ==
            "\n".join(judge.command(case) for case in dataset) + "\n", "cell.archived_input_matches_corpus")
    result = {}
    reasons = {"cell-kill-nonstrict": "grid.counts", "cell-kill-h-minus-one": "grid.metadata",
               "cell-locate-eps-zero": "locate.origin_closed"}
    for label in ("o2", "ubsan"):
        directory = base / label
        record = load(directory / "receipt.json")
        require(record["compile_exit"] == 0 and len(record["runs"]) == 4, "cell.build_status")
        results = {}
        nominal_outputs = None
        basis_cases = 0
        for run in record["runs"]:
            mutant = run["mutant"]
            name = mutant or "nominal"
            require(run["bridge_exit"] == 0 and not read(directory / (name + ".stderr")), "cell.archived_bridge_failed")
            outputs = [json.loads(line) for line in read(directory / (name + ".stdout")).splitlines()]
            require(len(outputs) == len(dataset), "cell.response_count")
            totals: dict[str, int] = {}
            failures = []
            for index, (case, output) in enumerate(zip(dataset, outputs)):
                try:
                    stats = judge.judge(case, output)
                except ValueError as error:
                    failures.append({"case": index, "reason": str(error)})
                    continue
                for key, value in stats.items():
                    totals[key] = max(totals.get(key, 0), value) if key.startswith("max_") else totals.get(key, 0) + value
            require(totals == run["totals"] and failures == run["divergences"], "cell.replay_differs_from_record")
            if mutant:
                require(mutant in reasons and run["verdict"] == "detected" and
                        any(failure["reason"] == reasons[mutant] for failure in failures), "cell.mutant_survived")
                results[name] = {"rejections": len(failures), "target_reason": reasons[mutant]}
                if mutant == "cell-kill-h-minus-one":
                    require(nominal_outputs is not None, "cell.nominal_must_precede_threshold_mutant")
                    changed_deaths = [
                        {"case": index, "nominal_dead_all": original["metadata"][2:4],
                         "mutant_dead_all": changed["metadata"][2:4]}
                        for index, (case, original, changed) in enumerate(zip(dataset, nominal_outputs, outputs))
                        if case["kind"] == "cloud" and original["metadata"][2:4] != changed["metadata"][2:4]
                    ]
                    require(bool(changed_deaths), "cell.threshold_mutant_no_actual_death_change")
                    results[name]["actual_death_changes"] = len(changed_deaths)
                    results[name]["first_actual_death_change"] = changed_deaths[0]
            else:
                require(not failures and run["verdict"] == "pass" and
                        all(value > 0 for value in totals.values()), "cell.nominal_nonvacuity")
                require(totals["cells"] == 38400 and totals["seeds"] == 32 and
                        totals["max_coordinate_bits"] == 98, "cell.documented_floor")
                nominal_outputs = outputs
                for case, output in zip(dataset, outputs):
                    if case["kind"] != "cloud":
                        continue
                    a, b = case["points"][:2]
                    d = tuple(bb - aa for aa, bb in zip(a, b))
                    u, v = output["u"], output["v"]
                    require(dot_exact(d, u) == dot_exact(d, v) == 0, "cell.basis_bisector_plane")
                    require(dot_exact(u, u) * dot_exact(v, v) - dot_exact(u, v) ** 2 > 0, "cell.basis_Gram_positive")
                    for sign in (-1, 1):
                        edge = tuple(sign * vv - uu for uu, vv in zip(u, v))
                        # Minimum |u+t*edge|² on the whole supporting line.
                        radius2 = Fraction(dot_exact(u, u)) - Fraction(dot_exact(u, edge) ** 2, dot_exact(edge, edge))
                        require(radius2 >= Fraction(dot_exact(d, d), case["rho"]), "cell.basis_inscribed_radius")
                    basis_cases += 1
                require(basis_cases == 32, "cell.basis_nonvacuity")
                totals["independent_basis_cases"] = basis_cases
                results[name] = totals
        require(set(results) == {"nominal", *reasons}, "cell.required_mutants")
        result[label] = results
    require(result["o2"] == result["ubsan"], "cell.builds_differ")
    return result


def replay_spindle() -> dict[str, Any]:
    base = RECEIPTS / "spindle"
    receipt = load(base / "summary.json")
    require(receipt["status"] == "completed" and receipt["sources_before"] == receipt["sources_after"],
            "spindle.receipt_status")
    commands = {row["name"]: row for row in receipt["commands"]}
    require(len(commands) == len(receipt["commands"]) == 11, "spindle.command_inventory")
    for row in commands.values():
        require(row["exit_code"] == row["expected_exit_code"], "spindle.expected_return")
        for stream in ("stdout", "stderr"):
            read(base / row[stream])
            key = str((base / row[stream]).relative_to(AUDIT))
            require(INPUT_HASHES[key] == row[stream + "_sha256"], "spindle.stream_digest")
    result = {}
    floors = {"roots": 4116, "spindle": 5184, "H_zero": 338, "q3_equal": 4,
              "q4_equal": 8, "wide_Xi_positive_H": 28, "cores": 432,
              "positive_cores": 194, "empty_cores": 238, "boxes": 5184, "corners": 5184,
              "masks": 560, "collections": 90, "contacts": 6, "orientations": 864}
    for label in ("optimized", "ubsan"):
        require(commands[label]["exit_code"] == 0, "spindle.nominal_return")
        output = read(base / commands[label]["stdout"])
        counters = dict((name, int(value)) for name, value in re.findall(r"(\w+)=(\d+)", output.splitlines()[-1]))
        require(all(counters.get(key) == value for key, value in floors.items()), "spindle.recorded_floor")
        for mutant in ("core-ball-ceil-distance", "witness-no-lane-mask"):
            row = commands[label + "_" + mutant]
            require(row["exit_code"] == row["expected_exit_code"] == 4, "spindle.mutant_return")
            require("mutant=" + mutant + " killed " in read(base / row["stdout"]), "spindle.mutant_marker")
        result[label] = {"recorded_counts": floors, "recorded_product_mutants_rejected": 2}
    return result


def main() -> int:
    start = time.monotonic()
    read(Path(__file__).resolve())
    result: dict[str, Any] = {"status": "running", "python_optimized": bool(sys.flags.optimize),
        "python": sys.version, "utc": datetime.now(timezone.utc).isoformat(),
        "scope": "replay_saved_python_judgements_and_spindle_receipt_only",
        "compilation": False, "cpp_executed": False, "public_status": "not_claimed", "gcp": "not_used"}
    try:
        result["sector_chord"] = replay_sector()
        result["cell"] = replay_cell()
        result["spindle"] = replay_spindle()
        require(all(hashlib.sha256((AUDIT / name).read_bytes()).hexdigest() == value
                    for name, value in INPUT_HASHES.items()), "replay_inputs_changed")
        result["status"] = "completed"
    except Exception as error:
        result["status"] = "failed"
        result["error"] = str(error)
    result["inputs_sha256"] = INPUT_HASHES
    result["seconds"] = time.monotonic() - start
    label = "optimized" if sys.flags.optimize else "normal"
    (RECEIPTS / ("replay_" + label + ".json")).write_text(
        json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"front replay status={result['status']} python_optimized={bool(sys.flags.optimize)} "
          f"seconds={result['seconds']:.3f}")
    if "error" in result:
        print(result["error"], file=sys.stderr)
    return 0 if result["status"] == "completed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
