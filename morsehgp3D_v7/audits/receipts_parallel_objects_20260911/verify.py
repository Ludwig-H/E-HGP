"""Read-only receipt reader; does not execute geometry or parallel algorithms."""

import hashlib
import json
from pathlib import Path
import subprocess


PACKET = Path(__file__).resolve().parent
ROOT = PACKET.parents[2]
FILES = {"README.md", "parallel_model.py", "commands.json", "result.json",
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
    require(set(sealed) == FILES, "incomplete receipt seal")
    context = json.loads((PACKET / "context_pins.json").read_text())
    for name, expected in context["historical_source_pins"].items():
        raw = subprocess.check_output(["git", "show", context["source_commit"] + ":" + name], cwd=ROOT)
        require(sha(raw) == expected, "historical source differs: " + name)
    dependency = (PACKET / context["model_dependency"]["path"]).resolve()
    require(sha(dependency.read_bytes()) == context["model_dependency"]["sha256"], "model dependency differs")
    capture = json.loads((PACKET / "commands.json").read_text())
    expected_sources = {str(f.relative_to(ROOT)): sha(f.read_bytes())
                        for f in [PACKET / "parallel_model.py", dependency]}
    require(capture["source_hashes_before"] == expected_sources == capture["source_hashes_after"],
            "consumed model bytes differ")
    commands = capture["commands"]
    require(len(commands) == 2, "missing model mode")
    result = json.loads((PACKET / "result.json").read_text())
    for item, flags in zip(commands, [[], ["-O"]]):
        expected = ["python3", "-B", *flags, str((PACKET / "parallel_model.py").relative_to(ROOT))]
        require(item["argv"] == expected and item["exit_code"] == 0 and item["stderr"] == "",
                "model command failed or differs")
        require(json.loads(item["stdout"]) == result, "captured result differs")
    require(commands[0]["stdout"] == commands[1]["stdout"], "normal and optimized modes differ")
    for name, value in {"cases": 11, "cut_checks": 234, "leaf_queries": 5178,
                        "heavy_queries": 5178, "dated_occurrence_checks": 10126,
                        "vertical_nodes": 12, "vertical_leaf_witnesses": 16,
                        "vertical_nonfinal_images": 2, "vertical_equal_level_merges": 9,
                        "maximum_tested_depth": 34, "maximum_jump_rounds": 7}.items():
        require(result[name] == value, "non-vacuity or counter differs: " + name)
    require(result["heavy_light_steps"] > 0 and result["heavy_binary_steps"] > 0 and
            result["inactive_leaf_queries"] > 0, "query branches vacuous")
    require(result["mutants_rejected"] == ["replace_open_cut_with_closed",
            "replace_dated_image_with_final_root", "lose_unary_contribution_date",
            "leave_equal_date_binary_nodes_uncontracted",
            "use_lower_leaf_outside_upper_descendants"], "mutants differ")
    require(result["simultaneous_three_parent_fusion"] is True and
            result["overlap_keeps_two_components"] is True, "identity or plateau fixture absent")
    for name in ["parallel_threads_executed", "performance_claim", "product_executed", "gcp_used"]:
        require(result[name] is False, "scope differs: " + name)
    require(result["public_status"] == "not_claimed" and
            result["constructor"] == "sequential_sorted_dsu_test_oracle" and
            result["heavy_chain_construction"] == "sequential_model_not_parallel_product" and
            result["verticals_consume"] == "completed_horizontal_forests_only", "model scope differs")
    print(json.dumps({"status": "passed_parallel_objects_receipt", "cases": 11,
                      "cut_checks": 234, "independent_queries": 5178, "vertical_nodes": 12,
                      "mutants": 5, "historical_sources": len(context["historical_source_pins"]),
                      "product_executed": False, "parallel_threads_executed": False,
                      "public_status": "not_claimed", "gcp_used": False}, sort_keys=True))


if __name__ == "__main__":
    main()
