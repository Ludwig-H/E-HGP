"""Fresh independent replay of the singleton-lot delta, confined to audits/."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

import full_lazy_audit as judge
import full_producer_run as runner


AUDIT = Path(__file__).resolve().parent
OLD = AUDIT / "receipts_full_lazy_20260905"
RECEIPT = AUDIT / "receipts_full_singleton_20260905"
WORK = AUDIT / ".work_full_singleton_20260905"
EXPECTED = "21b77d29a4ba2bca453b602a8faa4564a978f4ba71af5167c164faae4ef0e1a5"
runner.BRIDGE = AUDIT / "full_lazy_bridge.cpp"
runner.MUTANTS = {}
original_command = runner.command


def command(argv: list[str], receipt: Path, prefix: str,
            stdin: Path | None = None, overrides: dict[str, str] | None = None) -> dict:
    return original_command(["taskset", "-c", "0", *argv], receipt, prefix, stdin, overrides)


runner.command = command


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "build", "run", "judge"))
    parser.add_argument("--name", choices=("O2", "sanitized"), default="O2")
    args = parser.parse_args()
    if args.action == "prepare":
        if RECEIPT.exists() or WORK.exists():
            raise ValueError("create-only source and build directories already exist")
        if runner.sha(runner.ROOT / runner.HEADER) != EXPECTED:
            raise ValueError("source changed before capture")
        judge.verify_fixtures(judge.load_fixtures())
        RECEIPT.mkdir()
        WORK.mkdir()
        runner.prepare(RECEIPT, WORK)
        if runner.sha(RECEIPT / "source" / runner.HEADER) != EXPECTED:
            raise ValueError("captured source differs")
        previous = json.loads((OLD / "source_pins.json").read_text())["pins"]
        current = json.loads((RECEIPT / "source_pins.json").read_text())["pins"]
        changed = [p for p in previous if previous[p] != current.get(p)]
        if set(previous) != set(current) or changed != [str(runner.HEADER)]:
            raise ValueError("unexpected dependency delta")
        runner.write(RECEIPT / "input_binding.json", {
            "source_header": EXPECTED, "changed_dependencies": changed,
            "prior_header": previous[str(runner.HEADER)],
            "reused_inputs": {str(p.relative_to(runner.ROOT)): runner.sha(p)
                              for p in [OLD / "fixtures.json", OLD / "fixtures.txt",
                                        OLD / "O2_output.json", runner.BRIDGE,
                                        Path(judge.__file__), Path(runner.__file__)]},
            "scope": "Existing 109 orders reused as inputs, never as new execution results.",
            "public_status": "not_claimed"})
    elif args.action == "build":
        runner.build(RECEIPT, WORK, args.name)
    elif args.action == "run":
        binary = WORK / (args.name + ".bin")
        overrides = {"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                     "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"} if args.name == "sanitized" else {}
        result = command([str(binary)], RECEIPT, args.name, OLD / "fixtures.txt", overrides)
        result.update(binary_sha256=runner.sha(binary),
                      input_sha256=runner.sha(OLD / "fixtures.txt"))
        runner.write(RECEIPT / (args.name + "_run.json"), result)
        print(json.dumps({"name": args.name, "exit_code": result["exit_code"]}))
        if result["exit_code"] != 0:
            raise ValueError("engine transport failed; failure retained")
    else:
        fixture = judge.load_fixtures()
        judge.verify_fixtures(fixture)
        old_bytes = (OLD / "O2_output.json").read_bytes()
        mode = "optimized" if sys.flags.optimize else "normal"
        outcomes = []
        for name in ("O2", "sanitized"):
            path = RECEIPT / (name + "_output.json")
            run = json.loads((RECEIPT / (name + "_run.json")).read_text())
            if run["exit_code"] != 0 or runner.sha(path) != run["stdout_sha256"] or (
                    RECEIPT / run["stderr"]).read_bytes():
                raise ValueError("invalid transport: " + name)
            result = judge.check_output(path, fixture)
            same = path.read_bytes() == old_bytes
            if not same:
                raise ValueError("literal forest/stats/refusal delta versus 13c6: " + name)
            result.update(reference_output=str((OLD / "O2_output.json").relative_to(runner.ROOT)),
                          reference_sha256=runner.sha(OLD / "O2_output.json"),
                          byte_identical_to_13c6=True, source_header=EXPECTED,
                          scope="Fresh 109-order singleton replay; all forest, counter and budget-trial bytes match 13c6. No allocation-fault, constructor-suite or performance qualification.")
            runner.write(RECEIPT / (name + "_" + mode + ".json"), result)
            outcomes.append({"name": name, "counts": result["counts"], "byte_identical": same})
        runner.write(RECEIPT / ("judgments_" + mode + ".json"), {
            "outcomes": outcomes, "runner_sha256": runner.sha(Path(__file__)),
            "python_optimized": bool(sys.flags.optimize), "public_status": "not_claimed"})
        print(json.dumps(outcomes))


if __name__ == "__main__":
    main()
