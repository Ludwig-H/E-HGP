#!/usr/bin/env python3
"""Sequential mono P0 measurements with checked receipts, never FULL/GPU."""

from __future__ import annotations

import argparse
import base64
import hashlib
import itertools
import json
import math
import os
import platform
import re
import signal
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


WORK_FIELDS = (
    "validation_points", "uniqueness_comparisons", "pool_selection_tests",
    "pool_selected", "tree_nodes", "tree_point_visits", "dual_tasks",
    "credited_blocks", "noncredit_blocks", "leaf_pairs", "saturated_tasks",
    "max_tree_depth", "max_task_depth", "tube_records", "tube_cells",
    "tube_sort_comparisons", "tube_sweep_tests", "tube_credited_sites",
    "tube_separation_fallbacks",
)
PREDICATE_FIELDS = (
    "point_tests", "universal_queries", "q2_axis_terms", "corner_tests",
    "block_bound_tests", "negative_probes",
)
TIME_FIELDS = ("generation_ms", "prepare_ms", "plan_ms", "total_component_ms")


class InvalidReceipt(ValueError):
    """A completed process did not produce an admissible measurement."""


class CampaignInterrupted(KeyboardInterrupt):
    def __init__(self, signum: int) -> None:
        super().__init__(f"interrupted by signal {signum}")
        self.signum = signum


def require(condition: bool, message: str) -> None:
    if not condition:
        raise InvalidReceipt(message)


def uint(value: Any, name: str) -> int:
    require(type(value) is int and 0 <= value <= (1 << 64) - 1,
            f"{name}: expected u64 integer, not boolean/float")
    return value


def unique_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON key: {key}")
        result[key] = value
    return result


def reject_constant(value: str) -> None:
    raise InvalidReceipt(f"non-finite JSON constant: {value}")


def finite_float(value: str) -> float:
    number = float(value)
    require(math.isfinite(number), f"non-finite JSON number: {value}")
    return number


def parse_result(stdout: bytes) -> dict[str, Any]:
    result = json.loads(stdout.decode("utf-8"), object_pairs_hook=unique_object,
                        parse_constant=reject_constant, parse_float=finite_float)
    require(type(result) is dict, "result must be a JSON object")
    return result


def validate_work(work: Any, name: str) -> None:
    require(type(work) is dict and set(work) == {*WORK_FIELDS, "predicates"},
            f"{name}: missing or unknown work fields")
    for key in WORK_FIELDS:
        uint(work[key], f"{name}.{key}")
    predicates = work["predicates"]
    require(type(predicates) is dict and set(predicates) == set(PREDICATE_FIELDS),
            f"{name}: missing or unknown predicate fields")
    for key in PREDICATE_FIELDS:
        uint(predicates[key], f"{name}.predicates.{key}")
    require(work["max_tree_depth"] <= 48 and work["max_task_depth"] <= 96,
            f"{name}: depth exceeds the u16 representation bound")
    require(work["tube_cells"] <= work["tube_records"] and
            work["tube_sweep_tests"] <= 2 * work["tube_records"],
            f"{name}: inconsistent tube records/cells/sweeps")


def validate_result(row: Any, command: list[str]) -> None:
    """Check receipt consistency, not geometry or producer completeness."""
    require(type(row) is dict, "result must be a JSON object")
    fixed = {
        "schema": "mhgp8_p0_probe_v1", "status": "completed",
        "scope": "single_separated_rectangle_credits",
        "backend": "cpu_reference", "profile": "quantized_u16_input_only",
        "public_status": "not_claimed",
        "s_role": "rectangle_precondition_not_wspd_generation",
    }
    for key, expected in fixed.items():
        require(row.get(key) == expected, f"{key}: unexpected status/schema/scope")
    require(row.get("seed", 0) is None, "fixture must have no random seed")
    require(row.get("candidates_expanded") is False and
            row.get("downstream_measured") is False, "unexpected downstream claim")
    for key, expected in (("threads", 1), ("fixture_version", 1)):
        require(uint(row.get(key), key) == expected, f"{key}: incorrect value")
    expected_tuple = {
        "n": int(command[1]), "strategy": command[2], "lane": int(command[3]),
        "family": command[4], "kmax": int(command[5]),
        "separation_s": int(command[6]),
    }
    for key, expected in expected_tuple.items():
        if type(expected) is int:
            uint(row.get(key), key)
        require(row.get(key) == expected, f"command/result mismatch: {key}")
    n = row["n"]
    expected_b = max(1, n // 16) if row["family"] == "skew" else n - n // 2
    for key in ("n_a", "n_b", "total_pairs", "candidate_pairs", "rejected_pairs",
                "candidate_descriptors", "threshold", "core_credit"):
        uint(row.get(key), key)
    require(row["n_a"] == n - expected_b and row["n_b"] == expected_b,
            "fixture factor identities/cardinalities disagree with the recipe")
    require(row["n_a"] > 0 and row["n_b"] > 0 and
            row["total_pairs"] == row["n_a"] * row["n_b"], "invalid pair total")
    require(row["candidate_pairs"] <= row["total_pairs"] and
            row["candidate_pairs"] + row["rejected_pairs"] == row["total_pairs"],
            "inconsistent candidate/rejected counts")
    threshold = max(0, row["kmax"] + 2 - row["lane"])
    require(row["threshold"] == threshold and row["core_credit"] == 0,
            "incorrect active threshold or impossible fixture core credit")
    descriptors = row["candidate_descriptors"]
    require(descriptors <= threshold * (threshold + 1) // 2 and
            (descriptors == 0) == (row["candidate_pairs"] == 0),
            "inconsistent candidate descriptors")
    fraction = row.get("rejected_fraction")
    require(type(fraction) in (int, float) and math.isfinite(fraction) and
            math.isclose(fraction, row["rejected_pairs"] / row["total_pairs"],
                         rel_tol=1e-14, abs_tol=1e-15), "incorrect rejected fraction")
    for key in TIME_FIELDS:
        value = row.get(key)
        require(type(value) in (int, float) and math.isfinite(value) and value >= 0,
                f"{key}: expected finite nonnegative time")
    require(math.isclose(sum(row[key] for key in TIME_FIELDS[:3]),
                         row["total_component_ms"], rel_tol=1e-12, abs_tol=1e-6),
            "timing partition does not match total_component_ms")
    fingerprint = row.get("input_fnv1a64_le_u16_xyz")
    require(type(fingerprint) is str and
            re.fullmatch(r"[0-9a-f]{1,16}", fingerprint) is not None,
            "invalid input fingerprint")
    for key in ("preparation_work", "plan_work"):
        validate_work(row.get(key), key)
    prep, work = row["preparation_work"], row["plan_work"]
    require(prep["validation_points"] == n and work["validation_points"] == 0 and
            work["uniqueness_comparisons"] == 0, "incorrect validation accounting")
    require(all(prep[key] == 0 for key in WORK_FIELDS
                if key not in ("validation_points", "uniqueness_comparisons")) and
            all(value == 0 for value in prep["predicates"].values()),
            "unexpected preparation work for a no-core fixture")
    require(work["tube_credited_sites"] <= n * threshold,
            "tube credit sum exceeds saturated anchor credits")
    if threshold == 0:
        require(row["candidate_pairs"] == 0 and
                all(work[key] == 0 for key in WORK_FIELDS) and
                all(value == 0 for value in work["predicates"].values()),
                "inactive lane performed work or emitted candidates")
    if row["strategy"] != "tubes":
        require(all(work[key] == 0 for key in WORK_FIELDS if key.startswith("tube_")),
                "tube work attributed to a different strategy")
    if row["strategy"] != "pool":
        require(work["pool_selection_tests"] == work["pool_selected"] == 0,
                "pool work attributed to a different strategy")
    if row["strategy"] != "dual":
        require(all(work[key] == 0 for key in (
            "tree_nodes", "tree_point_visits", "dual_tasks", "credited_blocks",
            "noncredit_blocks", "leaf_pairs", "saturated_tasks", "max_tree_depth",
            "max_task_depth")), "dual work attributed to a different strategy")


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def utc_stamp() -> str:
    return datetime.now(timezone.utc).isoformat()


def write_json(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n")


def on_signal(signum: int, _frame: Any) -> None:
    raise CampaignInterrupted(signum)


def invoke(command: list[str], environment: dict[str, str], root: Path,
           record: dict[str, Any], *, new_session: bool = False) -> None:
    # A fast child may flush and signal its parent before Popen returns. Do
    # not let the campaign's raising handler escape before we own the child
    # object and can drain its pipes. Deferring Python callbacks changes no
    # OS signal mask and therefore passes no blocked mask to the child.
    handlers = {signum: signal.getsignal(signum) for signum in (signal.SIGINT, signal.SIGTERM)}
    pending: list[tuple[int, Any]] = []

    def defer(signum: int, frame: Any) -> None:
        pending.append((signum, frame))

    def restore() -> None:
        for signum, handler in handlers.items():
            signal.signal(signum, handler)

    interrupted: KeyboardInterrupt | None = None
    try:
        for signum, handler in handlers.items():
            # Preserve SIG_IGN/SIG_DFL semantics; all campaign callers install
            # on_signal. Other callable handlers are replayed, not replaced.
            if callable(handler):
                signal.signal(signum, defer)
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   env=environment, cwd=root, start_new_session=new_session)
        try:
            # Restoring/replaying is itself inside the collection handler;
            # the process is now installed even if replay raises immediately.
            restore()
            for signum, frame in pending:
                handlers[signum](signum, frame)
            stdout, stderr = process.communicate()
        except KeyboardInterrupt as error:
            interrupted = error
            if new_session:
                # This process was made leader of its own session/group.
                # Cancel descendants even if the group leader already exited.
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
            elif process.poll() is None:
                process.terminate()
            try:
                # Bound cancellation cleanup, not the duration of a benchmark.
                stdout, stderr = process.communicate(timeout=2)
            except subprocess.TimeoutExpired:
                if new_session:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                else:
                    process.kill()
                stdout, stderr = process.communicate()
        record.update(exit_code=process.returncode,
                      stdout=stdout.decode("utf-8", errors="replace"),
                      stderr=stderr.decode("utf-8", errors="replace"),
                      stdout_base64=base64.b64encode(stdout).decode("ascii"),
                      stderr_base64=base64.b64encode(stderr).decode("ascii"))
        if interrupted is not None:
            raise interrupted
    finally:
        # Also restore when Popen fails before returning a process object.
        restore()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--probe-kind", choices=["single", "batch", "axis", "additive"], default="single")
    parser.add_argument("--orders", nargs="+", choices=["baseline-first", "batch-first", "axis-first",
                                                       "independent-first", "variant-first"])
    parser.add_argument("--variants", nargs="+", choices=["additive", "intersection"])
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--sizes", type=int, nargs="+", default=[8000, 16000, 32000])
    parser.add_argument("--families", nargs="+",
                        choices=["grid", "sheet", "sheet_full", "skew", "tube", "rails"],
                        default=["grid", "sheet", "skew"])
    parser.add_argument("--strategies", nargs="+", choices=["pool", "dual", "tubes"],
                        default=["pool", "dual", "tubes"])
    parser.add_argument("--lanes", type=int, nargs="+", choices=[2, 3, 4],
                        default=[2, 3, 4])
    parser.add_argument("--kmax", type=int, nargs="+", default=[10])
    parser.add_argument("--s", type=int, nargs="+", default=[8])
    args = parser.parse_args()
    default_orders = (["independent-first", "variant-first"] if args.probe_kind == "additive" else
                      ["baseline-first", "axis-first" if args.probe_kind == "axis" else "batch-first"])
    if args.orders is None:
        args.orders = default_orders
    allowed_orders = set(default_orders)
    if not set(args.orders) <= allowed_orders:
        parser.error("execution order does not match the selected probe kind")
    if "sheet_full" in args.families and args.probe_kind not in ("axis", "additive"):
        parser.error("sheet_full belongs to the axis/additive probes only")
    if args.variants is not None and args.probe_kind != "additive":
        parser.error("variants belong to the additive probe only")
    variants = args.variants or (["additive", "intersection"] if args.probe_kind == "additive" else [None])
    if len(set(variants)) != len(variants):
        parser.error("duplicate matrix values: variants")
    if (args.repeats < 1 or min(args.sizes) < 2 or min(args.kmax) < 1 or
            max(args.kmax) > 10 or min(args.s) < 1):
        parser.error("require repeats >= 1, sizes >= 2, 1 <= kmax <= 10, s >= 1")
    for field in ("sizes", "families", "strategies", "lanes", "kmax", "s", "orders"):
        values = getattr(args, field)
        if len(set(values)) != len(values):
            parser.error(f"duplicate matrix values: {field}")
    root = Path(__file__).resolve().parents[2]
    source_root = root / "morsehgp3D_v8"
    binary = args.probe.resolve()
    try:
        args.output.mkdir(parents=True, exist_ok=False)
    except OSError as error:
        parser.error(f"cannot create a fresh campaign directory: {error}")
    sources = [source_root / "CMakeLists.txt", *sorted((source_root / "src").rglob("*.hpp")),
               *sorted((source_root / "src").rglob("*.cpp")),
               *sorted((source_root / "bench").glob("*.hpp")),
               source_root / "bench/p0_probe.cpp", Path(__file__).resolve()]
    if args.probe_kind != "single":
        sources += [source_root / f"bench/{args.probe_kind}_probe.cpp",
                    source_root / "bench/paired_receipts.py"]
        from paired_receipts import (additive_invariants, stable_signature, validate_additive,
                                     validate_axis, validate_batch)
    validator = (validate_result if args.probe_kind == "single" else
                 validate_batch if args.probe_kind == "batch" else
                 validate_axis if args.probe_kind == "axis" else validate_additive)
    hashes: dict[str, str] = {}
    binary_hash: str | None = None
    completed = attempts = 0
    status, error_text, exit_code = "failed", "campaign did not start", 1
    previous_handlers = {signum: signal.signal(signum, on_signal)
                         for signum in (signal.SIGINT, signal.SIGTERM)}
    try:
        hashes = {str(path.relative_to(root)): digest(path) for path in sources}
        binary_hash = digest(binary)
        cache = (binary.parent / "CMakeCache.txt").read_text()
        compilers = [line.split("=", 1)[1] for line in cache.splitlines()
                     if line.startswith("CMAKE_CXX_COMPILER:") and "=" in line]
        require(len(compilers) == 1 and bool(compilers[0]),
                "CMake cache must name exactly one nonempty CXX compiler")
        compiler = compilers[0]
        metadata = {
            "schema": "mhgp8_p0_campaign_v1", "receipt_validation_version": 2,
            "started_utc": utc_stamp(), "probe_kind": args.probe_kind,
            "scope": ("single_separated_rectangle_credits" if args.probe_kind == "single" else
                      "single_rectangle_three_lane_credit_batch" if args.probe_kind == "batch" else
                      "single_rectangle_axis_q2_residual" if args.probe_kind == "axis" else
                      "single_rectangle_additive_axis_q2_residual"),
            "public_status": "not_claimed", "downstream_measured": False,
            "gcp_used": False, "threads": 1,
            "commit": subprocess.check_output(
                ["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
            "worktree_status": subprocess.check_output(
                ["git", "status", "--short"], cwd=root, text=True),
            "platform": platform.platform(), "logical_cpu_count": os.cpu_count(),
            "cpuinfo": Path("/proc/cpuinfo").read_text().split("\n\n", 1)[0],
            "source_sha256": hashes, "probe": str(binary), "probe_sha256": binary_hash,
            "cmake_cache": cache, "warmup_runs": 0, "processes_sequential": True,
            "runner_command": [sys.executable, *sys.argv], "sizes": args.sizes,
            "families": args.families, "strategies": args.strategies, "lanes": args.lanes,
            "kmax": args.kmax, "separations": args.s, "repeats": args.repeats,
            "orders": args.orders if args.probe_kind != "single" else [],
            "compiler_version": subprocess.check_output([compiler, "--version"], text=True),
            "boost_scope": "test-only headers; no boost in the measured product library",
            "s_role": "precondition_only_not_wspd_generation",
        }
        if args.probe_kind == "additive":
            metadata["variants"] = variants
        write_json(args.output / "MANIFEST.json", metadata)
        environment = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
        identities: dict[tuple[str, int], str] = {}
        signatures: dict[tuple[Any, ...], Any] = {}
        additive_references: dict[tuple[Any, ...], Any] = {}
        selectors = args.lanes if args.probe_kind == "single" else args.orders
        matrix = itertools.product(args.families, args.kmax, args.s, selectors,
                                   args.sizes, args.strategies, range(args.repeats), variants)
        with (args.output / "MEASURES.jsonl").open("x") as stream:
            for family, kmax, separation, selector, n, strategy, repeat, variant in matrix:
                command = ([str(binary), str(n), strategy, str(selector), family,
                            str(kmax), str(separation)] if args.probe_kind == "single" else
                           [str(binary), str(n), strategy, family, str(kmax),
                            str(separation), str(selector)])
                if variant is not None:
                    command.append(variant)
                record: dict[str, Any] = {
                    "command": command, "repeat": repeat, "exit_code": None,
                    "status": "failed", "stdout": "", "stderr": "",
                    "stdout_base64": "", "stderr_base64": "",
                }
                attempts += 1
                try:
                    record["probe_sha256_before"] = digest(binary)
                    require(record["probe_sha256_before"] == binary_hash,
                            "probe binary changed before invocation")
                    print(json.dumps({"starting": command[1:], "repeat": repeat}), flush=True)
                    invoke(command, environment, root, record)
                    record["probe_sha256_after"] = digest(binary)
                    require(record["probe_sha256_after"] == binary_hash,
                            "probe binary changed during invocation")
                    if record["exit_code"] != 0:
                        raise RuntimeError(f"probe exited with code {record['exit_code']}")
                    require(not record["stderr_base64"], "unexpected probe stderr")
                    row = parse_result(base64.b64decode(record["stdout_base64"]))
                    validator(row, command)
                    record["result"] = row
                    identity = (family, n)
                    fingerprint = row["input_fnv1a64_le_u16_xyz"]
                    require(identity not in identities or identities[identity] == fingerprint,
                            "point identities changed between paired configurations")
                    identities[identity] = fingerprint
                    stable_key = (family, n, kmax,
                                  selector if args.probe_kind == "single" else "paired", strategy, variant)
                    signature = ((fingerprint, row["candidate_pairs"],
                                  row["candidate_descriptors"], row["preparation_work"],
                                  row["plan_work"]) if args.probe_kind == "single" else
                                 stable_signature(row, args.probe_kind))
                    require(stable_key not in signatures or
                            signatures[stable_key] == signature,
                            "work or residual changed across repetitions/s preconditions")
                    signatures[stable_key] = signature
                    if args.probe_kind == "additive":
                        for key, value in additive_invariants(row):
                            require(key not in additive_references or additive_references[key] == value,
                                    "unrestricted reference changed across strategies or variants")
                            additive_references[key] = value
                    record["status"] = "completed"
                    completed += 1
                except (InvalidReceipt, ValueError, UnicodeError, KeyError, TypeError) as error:
                    record.update(status="invalid", error=str(error))
                    raise InvalidReceipt(str(error)) from error
                except KeyboardInterrupt as error:
                    record.update(status="interrupted", error=str(error))
                    raise
                except Exception as error:
                    record.update(status="failed", error=str(error))
                    raise
                finally:
                    stream.write(json.dumps(record, allow_nan=False) + "\n")
                    stream.flush()
                progress = {"completed": completed}
                if args.probe_kind == "single":
                    progress.update(plan_ms=row["plan_ms"], candidate_pairs=row["candidate_pairs"])
                elif args.probe_kind == "batch":
                    progress.update(batch_ms=row["batch_ms"])
                elif args.probe_kind == "axis":
                    progress.update(axis_ms=row["axis_ms"], candidate_pairs=row["axis_candidates"])
                else:
                    progress.update(variant_ms=row["variant_ms"], candidate_pairs=row["variant_candidates"])
                print(json.dumps(progress), flush=True)
        status, error_text, exit_code = "completed", "", 0
    except InvalidReceipt as error:
        status, error_text = "invalid", str(error)
    except KeyboardInterrupt as error:
        status, error_text = "interrupted", str(error)
        exit_code = 128 + getattr(error, "signum", signal.SIGINT)
    except Exception as error:
        status, error_text = "failed", str(error)
    finally:
        closing_sources: dict[str, str | None] = {}
        for name in hashes:
            try:
                closing_sources[name] = digest(root / name)
            except OSError:
                closing_sources[name] = None
        try:
            closing_binary = digest(binary)
        except OSError:
            closing_binary = None
        unchanged = bool(hashes) and closing_sources == hashes
        binary_unchanged = binary_hash is not None and closing_binary == binary_hash
        if status == "completed" and not (unchanged and binary_unchanged):
            status, exit_code = "invalid", 1
            error_text = "source or probe binary changed during the campaign"
        completion = {
            "status": status, "runs": completed, "attempts": attempts,
            "finished_utc": utc_stamp(), "source_hashes_unchanged": unchanged,
            "probe_hash_unchanged": binary_unchanged,
            "source_sha256_closing": closing_sources, "probe_sha256_closing": closing_binary,
            "error": error_text,
        }
        write_json(args.output / "COMPLETION.json", completion)
        for signum, previous in previous_handlers.items():
            signal.signal(signum, previous)
    if error_text:
        print(f"P0 campaign {status}: {error_text}", file=sys.stderr)
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
