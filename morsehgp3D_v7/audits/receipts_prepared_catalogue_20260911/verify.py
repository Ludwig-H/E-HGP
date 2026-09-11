"""Verify a sealed mathematical receipt; no producer or model is executed."""

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
    prefix = context["ordered_packet"] + "/"
    manifest = json.loads(historical[prefix + "MANIFEST.json"])
    for name, entry in context["logical_context_objects"].items():
        require(manifest["files"][name] == entry and entry["provider"] == "object",
                "context object mapping differs")
        raw = historical[prefix + entry["path"]]
        require(sha(raw) == entry["sha256"] and len(raw) == entry["size"], "context object differs")

    capture = json.loads((PACKET / "commands.json").read_text())
    model = str((PACKET / "model.py").relative_to(ROOT))
    sources = {model: sha((PACKET / "model.py").read_bytes())}
    require(capture["source_hashes_before"] == sources == capture["source_hashes_after"],
            "consumed model differs")
    require(capture["python_before"] == capture["python_after"], "interpreter changed")
    result = json.loads((PACKET / "result.json").read_text())
    commands = capture["commands"]
    require(len(commands) == 6, "missing mode or command")
    for i, flags in enumerate([[], ["-O"]]):
        for j, args in enumerate([["--selftest"], [], ["--unknown"]]):
            item = commands[3 * i + j]
            require(item["argv"] == ["python3", "-B", *flags, model, *args], "command differs")
            require(item["exit_code"] == (0 if j == 0 else 2) and item["stderr"] == "",
                    "command status differs")
            if j == 0:
                require(json.loads(item["stdout"]) == result, "captured result differs")
            else:
                require(item["stdout"] == "", "CLI rejection differs")
    require(commands[0]["stdout"] == commands[3]["stdout"], "normal/-O differ")
    require(result["status"] == "passed_bounded_exact_model" and result["causal_mutants"] == 5,
            "scope or mutants differ")
    ranks = result["rank_binding"]
    require((ranks["catalogue_variants"], ranks["pair_sign_comparisons"], ranks["program_comparisons"])
            == (6, 84, 18) and ranks["equivalent_raw_representation_accepted"] is True,
            "rank witness differs")
    require([(r["mutant"], r["cause"]) for r in ranks["causal_rejections"]] == [
        ("unbound_integer_rank", "rank_unbound_to_exact_level"),
        ("nonincreasing_representatives", "representatives_not_strictly_increasing"),
        ("equal_level_key_inversion", "program_not_ordered"),
        ("duplicate_with_omission", "program_not_permutation")], "rank causal witnesses differ")
    cache = result["cache_before"]
    expected = {
        "source_u16": [0, 2, 3, 4], "K": 2, "initial_pair": [0, 4],
        "initial_center": "2", "initial_radius_squared": "4", "initial_interior": [2, 3],
        "initial_shell": [0, 4], "initial_window": [3, 4], "terminal_pair": [2, 4],
        "terminal_center": "3", "terminal_radius_squared": "1", "terminal_interior": [3],
        "terminal_shell": [2, 4], "terminal_window": [2, 3], "old_before_squared": "5",
        "new_before_squared": "2", "old_fresh_accepted": True, "new_fresh_accepted": False,
        "mutant_terminal_only_accepted": True, "stored_before_guard_rejected": True,
        "independent_power_classifications": 8,
        "fallback_before_squared": "9/2", "fallback_fresh_accepted": True,
        "fallback_direct_reuse_allowed": False,
    }
    for name, value in expected.items():
        require(cache[name] == value, "cache witness differs: " + name)
    require(cache["safe_bounds_tested"] == ["5", "11/2", "6", "10"], "safe cache bounds differ")
    require(result["not_claimed"] == ["geometric catalogue validation", "prepared owner implementation",
        "authentic FULL request", "product execution", "performance"], "scope differs")
    print(json.dumps({"status": "passed_prepared_catalogue_receipt", "rank_variants": 6,
                      "pair_sign_comparisons": 84, "programs_compared": 18,
                      "causal_mutants": 5, "cache_counterfixture_points": 4,
                      "valid_lower_threshold_requires_fallback": True,
                      "historical_context_pins": len(historical), "product_executed": False,
                      "performance_claim": False, "public_status": "not_claimed",
                      "gcp_used": False}, sort_keys=True))


if __name__ == "__main__":
    main()
