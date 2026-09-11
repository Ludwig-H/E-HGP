"""Read sealed captures and historical context; do not execute the model."""

import hashlib
import json
from pathlib import Path
import subprocess


PACKET = Path(__file__).resolve().parent
ROOT = PACKET.parents[2]
FILES = {"README.md", "model.py", "commands.json", "result.json",
         "context_pins.json", "verify.py"}


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise SystemExit(reason)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    seals = {}
    for line in (PACKET / "SHA256SUMS").read_text().splitlines():
        digest, separator, name = line.partition("  ")
        require(separator == "  " and name in FILES and name not in seals, "invalid seal")
        require(sha((PACKET / name).read_bytes()) == digest, "changed receipt: " + name)
        seals[name] = digest
    require(set(seals) == FILES, "incomplete seal")
    context = json.loads((PACKET / "context_pins.json").read_text())
    historical = {}
    for name, expected in context["historical_source_pins"].items():
        raw = subprocess.check_output(["git", "show", context["source_commit"] + ":" + name], cwd=ROOT)
        require(sha(raw) == expected, "historical context differs: " + name)
        historical[name] = raw
    prefix = context["streaming_packet"] + "/"
    manifest = json.loads(historical[prefix + "MANIFEST.json"])
    for name, entry in context["logical_context_objects"].items():
        require(manifest["files"][name] == entry and entry["provider"] == "object",
                "context object mapping differs")
        raw = historical[prefix + entry["path"]]
        require(sha(raw) == entry["sha256"] and len(raw) == entry["size"],
                "context object differs")
    logical = context["logical_context_objects"]
    streaming_name = "build/v7_streaming_graph_20260911/bench_n8000_streaming/results.json"
    observed = json.loads(historical[prefix + logical[streaming_name]["path"]])
    bench = observed["bench"]
    require((bench["n"], bench["s"], bench["requested_K"]) == (8000, 8, 10), "counter scope differs")
    stream = bench["streaming"]
    orders = stream["per_order"]
    A = sum(row["hub_msf_work"]["domain_vertices"] for row in orders)
    L, R = stream["native_births"], stream["occurrences"]
    pivots = sum(row["pivots"] for row in orders)
    require(pivots == A - L, "observed pivot accounting differs")
    derived = {
        "A": A, "L": L, "R": R, "C": L - stream["retained_certificate_edges"],
        "pivots": pivots, "direct_union_attempts_derived_not_measured": R - pivots,
        "old_hub_union_attempts_observed": sum(row["hub_msf_work"]["dsu_attempts"] for row in orders),
        "old_native_union_attempts_observed": sum(row["native_msf_work"]["dsu_attempts"] for row in orders),
        "retained_edges": stream["retained_certificate_edges"],
    }
    require(derived == context["n8000_counter_derivation"], "counter derivation differs")
    require(derived["direct_union_attempts_derived_not_measured"] == 7342931, "counter witness differs")

    capture = json.loads((PACKET / "commands.json").read_text())
    model = str((PACKET / "model.py").relative_to(ROOT))
    expected_sources = {model: sha((PACKET / "model.py").read_bytes())}
    require(capture["source_hashes_before"] == expected_sources == capture["source_hashes_after"],
            "consumed model differs")
    require(capture["python_before"] == capture["python_after"], "interpreter changed during capture")
    result = json.loads((PACKET / "result.json").read_text())
    commands = capture["commands"]
    require(len(commands) == 6, "missing command or mode")
    for i, flags in enumerate([[], ["-O"]]):
        for j, argument in enumerate([["--selftest"], [], ["--unknown"]]):
            item = commands[3 * i + j]
            require(item["argv"] == ["python3", "-B", *flags, model, *argument], "command differs")
            require(item["exit_code"] == (0 if j == 0 else 2) and item["stderr"] == "",
                    "command status differs")
            if j == 0:
                require(json.loads(item["stdout"]) == result, "model result differs")
            else:
                require(item["stdout"] == "", "CLI rejection differs")
    require(commands[0]["stdout"] == commands[3]["stdout"], "normal/-O differ")
    require(result["status"] == "passed_bounded_sequential_birth_stream_model", "scope differs")
    require(result["window_widths"] == [1, 7, 31], "window coverage differs")
    require(result["corpus_cases"] == 14 and result["window_runs"] == 42,
            "vacuous model corpus")
    require(result["partition_comparisons"] == 3276 and result["causal_rejections"] == 9,
            "vacuous cuts or rejections")
    require(len(result["cases"]) == result["corpus_cases"] and
            len(result["rejections"]) == result["causal_rejections"], "missing cases")
    totals = result["cost_totals_once_per_case"]
    require(totals["pivots"] == totals["A"] - totals["L"] and
            totals["nonpivot_tests"] == totals["R"] - totals["A"] + totals["L"] and
            totals["retained"] == totals["L"] - totals["C"], "model work differs")
    require(sum(run["boundaries_after_pivot"] for case in result["cases"] for run in case["runs"]) > 0,
            "no window splits after pivot")
    by_name = {row["name"]: row for row in result["rejections"]}
    identity = by_name["replace_native_phi_by_find"]
    require(identity["expected_native"] != identity["wrong_native"] and
            identity["equal_open_closed_partitions"] > 0,
            "identity counterfixture no longer stronger than partitions")
    require("omit_future_isolated_birth" in by_name and "discard_future_birth_date" in by_name,
            "birth mutants missing")
    print(json.dumps({"status": "passed_birth_stream_receipt", "cases": result["corpus_cases"],
                      "window_runs": result["window_runs"], "cuts_compared": result["partition_comparisons"],
                      "causal_rejections": result["causal_rejections"],
                      "historical_context_pins": len(historical),
                      "n8000_direct_attempts": "7342931_derived_not_executed",
                      "product_executed": False, "performance_claim": False,
                      "public_status": "not_claimed", "gcp_used": False}, sort_keys=True))


if __name__ == "__main__":
    main()
