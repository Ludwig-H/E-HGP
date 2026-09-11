"""Read-only receipt reader; no model, geometry or parallel execution."""

import hashlib
import json
from pathlib import Path
import subprocess


PACKET = Path(__file__).resolve().parent
ROOT = PACKET.parents[2]
FILES = {"README.md", "msf_model.py", "commands.json", "result.json",
         "context_pins.json", "verify.py"}


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise SystemExit(reason)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    sealed = {}
    for line in (PACKET / "SHA256SUMS").read_text().splitlines():
        digest, separator, name = line.partition("  ")
        require(separator == "  " and name in FILES and name not in sealed, "invalid seal")
        require(sha((PACKET / name).read_bytes()) == digest, "changed receipt: " + name)
        sealed[name] = digest
    require(set(sealed) == FILES, "incomplete seal")
    context = json.loads((PACKET / "context_pins.json").read_text())
    for name, expected in context["historical_source_pins"].items():
        raw = subprocess.check_output(["git", "show", context["source_commit"] + ":" + name], cwd=ROOT)
        require(sha(raw) == expected, "historical source differs: " + name)
    capture = json.loads((PACKET / "commands.json").read_text())
    model = PACKET / "msf_model.py"
    expected_sources = {str(model.relative_to(ROOT)): sha(model.read_bytes())}
    require(capture["source_hashes_before"] == expected_sources == capture["source_hashes_after"],
            "consumed source differs")
    commands = capture["commands"]
    require(len(commands) == 2, "missing model mode")
    result = json.loads((PACKET / "result.json").read_text())
    for item, flags in zip(commands, [[], ["-O"]]):
        expected = ["python3", "-B", *flags, str(model.relative_to(ROOT))]
        require(item["argv"] == expected and item["exit_code"] == 0 and item["stderr"] == "",
                "model command failed or differs")
        require(json.loads(item["stdout"]) == result, "captured result differs")
    require(commands[0]["stdout"] == commands[1]["stdout"], "normal/-O differ")
    for name, value in {"corpus_cases": 10, "composition_runs": 198,
                        "partition_comparisons": 4720, "reference_partition_comparisons": 168,
                        "projection_cases_count": 3, "projection_source_partition_comparisons": 66,
                        "projection_partition_comparisons": 198, "mutants_rejected": 5}.items():
        require(result[name] == value, "counter or non-vacuity differs: " + name)
    require(len(result["cases"]) == 10 and len(result["projection_cases"]) == 3,
            "missing cases")
    require(set(result["strategy_totals"]) == {"balanced", "fold", "binary_flush"},
            "missing composition strategy")
    for totals in result["strategy_totals"].values():
        require(totals["runs"] == 66 and totals["merge_input_edges_sum"] > 0 and
                totals["max_resident_edge_slots"] > 0, "vacuous composition strategy")
    require([item["name"] for item in result["mutants"]] == [
        "arbitrary_local_maximal_forest", "discard_isolated_vertices",
        "backdate_certificate_weights", "publish_local_fusions_as_global",
        "discard_late_lighter_edge"], "mutants differ")
    counterexample = result["projection_cases"][0]
    require(counterexample["name"] == "projection_changes_canonical_certificate" and
            counterexample["same_canonical_certificate"] is False and
            counterexample["sparse_projection_certificate_ids"] == ["e0", "e2"] and
            counterexample["full_projection_certificate_ids"] == ["e3", "e0"] and
            counterexample["global_events"] == [[1, [["0"], ["1"], ["2"]]]],
            "projection counterexample missing or differs")
    require(result["status"] == "passed_bounded_sequential_model" and result["not_claimed"] == [
        "parallel backend", "product integration", "runtime speedup", "RAM bound in bytes",
        "identical certificates across endpoint projection"], "scope differs")
    print(json.dumps({"status": "passed_composable_msf_receipt", "composition_cases": 10,
                      "projection_cases": 3, "compositions": 198,
                      "partition_comparisons_total": 5152, "mutants": 5,
                      "historical_sources": len(context["historical_source_pins"]),
                      "product_executed": False, "parallel_threads_executed": False,
                      "performance_claim": False, "public_status": "not_claimed",
                      "gcp_used": False}, sort_keys=True))


if __name__ == "__main__":
    main()
