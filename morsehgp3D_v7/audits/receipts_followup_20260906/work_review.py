"""Locate the captured K9 MEB refusal from charged control-flow identities.

Read-only: no product engine, build, constructor judge or performance estimate.
The identities below describe this named refusal, not a successful order.
"""
from __future__ import annotations

from copy import deepcopy
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
PREFIX = "morsehgp3D_v7/receipts/full_gabriel_successor_mono_20260905/heavy/"
NAME = "n32000_s8_k10_lazy_c1000000"
PINS = {
    PREFIX + NAME + ".raw.txt": "b3a91ed12340b0b36c84dc7fff5cf0e894105380e431cab7777433a667f6caa7",
    PREFIX + NAME + ".receipt.json": "f0825b258e54336359787844bd651336245cf4180552528fe10f50d8d4df631e",
    "morsehgp3D_v7/src/forest/full_gabriel.hpp": "85c27ab91d7f159520a8db3098629447b0a213a134c5c042a86c585416847fad",
    "morsehgp3D_v7/src/forest/silent_incidence.hpp": "f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76",
}


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def diagnose(row: dict, terminal: dict) -> dict:
    require(row["k"] == terminal["last_order"] == 9
            and row["outcome"] == terminal["outcome"] == "resource_exhausted"
            and row["reason"] == terminal["reason"] == "full_gabriel_meb_call_budget",
            "named_refusal")
    require(terminal["terminal_status"] == "failed" and terminal["exit_code"] == 2
            and terminal["completed_orders_diagnostic"] == 8
            and not terminal["complete_requested_horizontal_orders"]
            and not row["certificate_digest"] and not terminal["certificate_digest"],
            "no_refusal_promotion")
    require(row["meb_calls"] == row["geometry_meb_calls"] == 4_000_000,
            "call_cap_and_geometry_mirror")
    p, c, m = row["portal_requests"], row["chain_steps"], row["meb_calls"]
    j, lookups = row["singleton_intruder_resolutions"], row["direct_lookups"]
    require(p + c == m + 1, "one_uncharged_invocation")
    require(row["terminal_direct"] == p - 1, "one_pending_portal")
    require(lookups == j + c, "all_charged_chain_mebs_reached_direct_lookup")
    require(row["cache_inserts"] + row["cache_skips"] == row["terminal_direct"],
            "completed_cache_dispositions")
    require(row["minimum_hits"] + row["cache_hits"] + row["terminal_direct"]
            == row["face_visits"] - 1, "one_pending_face")
    require(row["successor_steps"] < 128_000_000, "successor_cap_not_first")
    terminations = row["terminal_direct"] - j
    return {
        "failed_site": "initial_K_site_MEB_of_new_portal",
        "charged_meb_calls": m,
        "pending_invocation_ordinal": m + 1,
        "portal_requests": p,
        "chain_steps": c,
        "completed_chain_terminations": terminations,
        "successor_budget_remaining_at_this_prefix": 128_000_000 - row["successor_steps"],
        "possible_repeated_chain_terminals": {
            "lower_bound": 0,
            "upper_bound": max(0, terminations - 1),
            "distinct_labels_U": "not_recorded",
            "formula": "R=T-U, only for the already completed prefix and first-use certification",
        },
        "prediction_of_completed_K9_or_K10": False,
    }


def main() -> dict:
    for name, pin in PINS.items():
        require(sha(ROOT / name) == pin, "input_pin:" + name)
    raw = (ROOT / (PREFIX + NAME + ".raw.txt")).read_text()
    rows = [json.loads(line) for line in raw.splitlines() if line.startswith("{")]
    receipt = json.loads((ROOT / (PREFIX + NAME + ".receipt.json")).read_text())
    require(rows[1:-1] == receipt["orders"] and rows[-1] == receipt["terminal"],
            "raw_receipt_binding")
    configuration, row, terminal = rows[0], rows[-2], rows[-1]
    require(configuration["n"] == 32000 and configuration["s"] == 8, "input_identity")
    result = diagnose(row, terminal)
    mutations = []
    for mutation, expected in (
        ("extra_charged_call", "call_cap_and_geometry_mirror"),
        ("promoted_terminal", "no_refusal_promotion"),
        ("missing_direct_lookup", "all_charged_chain_mebs_reached_direct_lookup"),
        ("missing_cache_disposition", "completed_cache_dispositions"),
    ):
        changed, end = deepcopy(row), deepcopy(terminal)
        if mutation == "extra_charged_call":
            changed["meb_calls"] += 1
            changed["geometry_meb_calls"] += 1
        elif mutation == "promoted_terminal":
            end["complete_requested_horizontal_orders"] = True
        elif mutation == "missing_direct_lookup":
            changed["direct_lookups"] -= 1
        else:
            changed["cache_skips"] -= 1
        try:
            diagnose(changed, end)
        except ValueError as error:
            require(str(error) == expected, "mutation_rejection_reason")
            mutations.append({"mutation": mutation, "rejected_by": str(error)})
        else:
            raise ValueError("unrefuted_mutation:" + mutation)
    return {
        "status": "passed", "inputs": PINS, "diagnosis": result,
        "data_corruptions_refuted": mutations, "script_sha256": sha(Path(__file__)),
        "scope": "Bounded control-flow inference on one captured failure; no engine or timing claim",
        "public_status": "not_claimed", "gcp": "not_used",
    }


if __name__ == "__main__":
    print(json.dumps(main(), indent=2, sort_keys=True))
