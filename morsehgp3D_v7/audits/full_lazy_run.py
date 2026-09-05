"""Exercise captured lazy FULL sources and private mutants, only under audits."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

import full_producer_run as runner

AUDIT = Path(__file__).resolve().parent
RECEIPT = AUDIT / "receipts_full_lazy_20260905"
WORK = AUDIT / ".work_full_lazy_20260905"
runner.BRIDGE = AUDIT / "full_lazy_bridge.cpp"
runner.MUTANTS = {
    "reject_j1": (
        "if (count == 1) {",
        'if (count == 1) return invariant("audit_mutant_missing_j1");\n'
        "    if (count == 1) {",
    ),
    "collapse_minima": (
        "minima[minimum_order[i]].token = prior_count + static_cast<FullNodeId>(i - mb);",
        "minima[minimum_order[i]].token = prior_count;  // Private mutant.",
    ),
    "ignore_capacity": (
        "if (aliases.size() >= cache_caps->max_entries) {",
        "if (false && aliases.size() >= cache_caps->max_entries) {  // Private mutant.",
    ),
}
original_command = runner.command


def pinned_cpu_command(argv: list[str], receipt: Path, prefix: str,
                       stdin: Path | None = None,
                       overrides: dict[str, str] | None = None) -> dict:
    return original_command(["taskset", "-c", "0", *argv], receipt, prefix, stdin, overrides)


runner.command = pinned_cpu_command


def unique_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("duplicate transport key: " + key)
        result[key] = value
    return result


def transport(name: str, fixture: dict) -> list[dict]:
    """A broken process or JSON stream is not a scientific mutant refutation."""
    path = RECEIPT / (name + "_output.json")
    run = json.loads((RECEIPT / (name + "_run.json")).read_text())
    if (run["exit_code"] != 0 or runner.sha(path) != run["stdout_sha256"]
            or (RECEIPT / run["stderr"]).read_bytes() != b""):
        raise ValueError("engine transport failure: " + name)
    rows = json.loads(path.read_text(), object_pairs_hook=unique_object)["records"]
    if len(rows) != len(fixture["records"]) or any(r["id"] != i for i, r in enumerate(rows)):
        raise ValueError("record transport failure: " + name)
    return rows


def mutant_witness(name: str, rows: list[dict], nominal: list[dict], fixture: dict) -> dict:
    if any(a != b for a, b, r in zip(rows, nominal, fixture["records"]) if not r["lazy"]):
        raise ValueError("mutant altered EAGER arm: " + name)
    witnesses = []
    for row, r in zip(rows, fixture["records"]):
        if not r["lazy"]:
            continue
        if name == "reject_j1":
            hit = row["status"] == 4 and row["reason"] == "audit_mutant_missing_j1" and row["order"] == 0
        elif name == "collapse_minima":
            hit = row["status"] == 0 and row["nodes"] != r["expected"]["nodes"] and any(
                (a["first"], a["parent_count"]) != (b["first"], b["parent_count"])
                for a, b in zip(row["nodes"], r["expected"]["nodes"]))
        else:
            hit = row["status"] == 0 and row["stats"]["cache_inserts"] > r["capacity"]
        if hit:
            witnesses.append({"id": r["id"], "case": fixture["cases"][r["case_index"]]["name"],
                              "order": r["order"], "capacity": r["capacity"],
                              "status": row["status"], "reason": row["reason"]})
    if not witnesses:
        raise ValueError("missing causal mutant witness: " + name)
    return {"unchanged_eager_representations": 218, "witness_count": len(witnesses),
            "first_witnesses": witnesses[:8]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "build", "run", "judge"))
    parser.add_argument("--name", choices=("O2", "sanitized", *runner.MUTANTS), default="O2")
    args = parser.parse_args()
    RECEIPT.mkdir(exist_ok=True)
    WORK.mkdir(exist_ok=True)
    if args.action == "prepare":
        expected = "13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627"
        if runner.sha(runner.ROOT / runner.HEADER) != expected:
            raise ValueError("frozen producer changed before capture")
        runner.prepare(RECEIPT, WORK)
        captured = RECEIPT / "source" / runner.HEADER
        if runner.sha(captured) != expected:
            raise ValueError("capture does not match frozen producer")
    elif args.action == "build":
        runner.build(RECEIPT, WORK, args.name)
    elif args.action == "run":
        runner.run(RECEIPT, WORK, args.name)
    else:
        import full_lazy_audit as judge

        fixture = judge.load_fixtures()
        judge.verify_fixtures(fixture)
        mode = "optimized" if sys.flags.optimize else "normal"
        outcomes = []
        nominal_rows = transport("O2", fixture)
        expected_reasons = {"reject_j1": "lazy.identity", "collapse_minima": "lazy.node_structure",
                            "ignore_capacity": "lazy.first_c_admission"}
        for name in ("O2", "sanitized", *runner.MUTANTS):
            path = RECEIPT / (name + "_output.json")
            rows = transport(name, fixture)
            witness = mutant_witness(name, rows, nominal_rows, fixture) if name in runner.MUTANTS else None
            try:
                result = judge.check_output(path, fixture)
                code = 0
            except ValueError as error:
                if name not in expected_reasons or str(error) != expected_reasons[name]:
                    raise ValueError("unexpected scientific rejection: " + name + ": " + str(error)) from error
                result = {"status": "rejected", "reason": str(error),
                          "output_sha256": runner.sha(path), "python_optimized": bool(sys.flags.optimize),
                          "causal_witness": witness, "checker_sha256": runner.sha(Path(judge.__file__))}
                code = 1
            runner.write(RECEIPT / (name + "_" + mode + ".json"), result)
            expected = int(name not in ("O2", "sanitized"))
            outcomes.append({"name": name, "code": code, "expected_code": expected,
                             "reason": result.get("reason"), "counts": result.get("counts")})
        runner.write(RECEIPT / ("judgments_" + mode + ".json"), {
            "outcomes": outcomes, "python_optimized": bool(sys.flags.optimize),
            "runner_sha256": runner.sha(Path(__file__)),
            "shared_runner_sha256": runner.sha(Path(runner.__file__)),
            "checker_sha256": runner.sha(Path(judge.__file__)), "public_status": "not_claimed"})
        print(json.dumps(outcomes))
        if any(r["code"] != r["expected_code"] for r in outcomes):
            raise ValueError("unexpected judgment; inspect retained outputs")


if __name__ == "__main__":
    main()
