"""Exact volume accounting of the sealed eager FULL mono observations.

No C++ process is invoked. Cache bounds describe a proposed representation,
not an observed lazy execution, allocator size, RSS reduction or timing gain.
"""
from __future__ import annotations

import argparse
from decimal import Decimal
import hashlib
import json
from pathlib import Path
from typing import Any

AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parents[1]
RECEIPT = AUDIT / "receipts_full_mono_20260905"
SOURCE = ROOT / "morsehgp3D_v7/receipts/full_gabriel_mono_20260905"
STEMS = ("n8000_s8_k10", "n16000_s8_k10", "n32000_s8_k10",
         "n8000_s10_k10", "n8000_s12_k10")


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def unique(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        require(key not in result, "duplicate_key." + key)
        result[key] = value
    return result


def decode(value: str) -> Any:
    return json.loads(value, parse_float=Decimal, object_pairs_hook=unique)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def integer(row: dict[str, Any], name: str) -> int:
    value = row[name]
    require(type(value) is int and value >= 0, "nonnegative_integer." + name)
    return value


def analyse_order(row: dict[str, Any], config: dict[str, Any]) -> dict[str, Any]:
    k = integer(row, "k")
    direct = integer(row, "connection_catalogue_records")
    minimum_source = integer(row, "minimum_catalogue_records")
    leaves = integer(row, "certificate_minima")
    nodes = integer(row, "certificate_nodes")
    parents = integer(row, "certificate_parent_refs")
    aliases = integer(row, "aliases")
    portals = integer(row, "portal_requests")
    steps = integer(row, "chain_steps")
    calls = integer(row, "meb_calls")
    roots = integer(row, "terminal_roots")
    visits = integer(row, "face_visits")
    complete = row["outcome"] == "complete_relative"
    result: dict[str, Any] = {
        "k": k, "complete": complete, "minimum_source": minimum_source,
        "direct": direct, "observed_aliases": aliases,
        "observed_build_ms": str(row["build_ms"]),
    }
    if not complete:
        require(row["reason"] == "full_gabriel_alias_budget", "unexpected_refusal")
        require(leaves == nodes == parents == roots == 0, "partial_refused_forest")
        require(aliases == config["max_aliases_per_order"], "alias_refusal_boundary")
        # Entire input catalogue counts, not completed output. Do not use the
        # success-only equal-facet formula with partial visit counters.
        strict_bound = min(4, k + 1) * direct
        result.update(
            strict_visits_upper_bound_from_complete_input=strict_bound,
            lazy_optional_cache_entries_upper_bound=strict_bound,
            mandatory_minimum_records=minimum_source,
            minimum_plus_cache_keys_upper_bound=minimum_source + strict_bound,
            cache_eight_million_covers_bound=strict_bound <= 8_000_000,
            bound_scope="Input-based admission bound only; no successful forest or lazy run inferred",
        )
        return result
    require(leaves == (config["n"] if k == 1 else minimum_source), "minimum_catalogue_count")
    require(roots == 1 and parents == nodes - roots and roots <= leaves <= nodes,
            "forest_parent_identity")
    require(nodes <= 2 * leaves - roots, "forest_multifusion_bound")
    strict = visits - (k + 1) * direct
    equal = (k + 1) * direct - strict
    require(2 * direct <= strict <= min(4, k + 1) * direct and equal >= 0,
            "support_visit_identity")
    require(aliases == leaves + equal + portals, "eager_alias_partition")
    require(integer(row, "alias_hits") + portals == strict, "strict_resolution_partition")
    require(calls == integer(row, "geometry_meb_calls") == portals + steps,
            "meb_call_identity")
    require(integer(row, "terminal_direct") == portals, "terminal_portal_identity")
    require(integer(row, "meb_supports") >= calls, "support_nonvacuity")
    forest_bytes = (nodes * integer(config, "sizeof_full_node")
                    + leaves * integer(config, "sizeof_facet_key") + parents * 8)
    # Cache excludes mandatory minima, which already have catalogue tokens.
    # It stores a subset of the old nonminimum aliases AND of strict requests.
    cache_bound = min(aliases - leaves, strict)
    conceptual_keys_bound = leaves + cache_bound
    result.update(
        certificate_nodes=nodes, certificate_minima=leaves, certificate_parent_refs=parents,
        internal_nodes=nodes - leaves, strict_visits=strict,
        equal_facets_installed=equal, new_strict_facets_from_portals=portals,
        equal_alias_fraction=str(Decimal(equal) / aliases),
        forest_logical_bytes=forest_bytes,
        catalogue_logical_bytes=(minimum_source + direct) * config["sizeof_forest_event"],
        lazy_optional_cache_entries_upper_bound=cache_bound,
        minimum_plus_cache_keys_upper_bound=conceptual_keys_bound,
        conceptual_key_reduction_lower_bound=aliases - conceptual_keys_bound,
        no_skip_lazy_meb_calls_upper_bound=calls + integer(row, "alias_hits"),
        observed_meb_calls=calls, observed_chain_steps=steps,
        bound_scope="Same catalogues/strict requests; all strict resolutions retained, no skips. "
                    "Conceptual keys include mandatory minima; no allocator/RSS or lazy timing claim",
    )
    return result


def analyse() -> dict[str, Any]:
    pins = json.loads((RECEIPT / "analysis_source_pins.json").read_text())
    for path, expected in pins["pins"].items():
        require(sha(ROOT / path) == expected, "source_changed." + path)
    attempts = []
    successful_orders = 0
    for stem in STEMS:
        receipt = decode((SOURCE / (stem + ".receipt.json")).read_text())
        raw = (SOURCE / (stem + ".raw.txt")).read_text()
        records = [decode(line) for line in raw.splitlines() if line.startswith("{")]
        require(records[0]["type"] == "configuration" and records[-1]["type"] == "terminal",
                "raw_record_boundary")
        config, terminal = records[0], records[-1]
        orders = records[1:-1]
        require(orders == receipt["orders"] and terminal == receipt["terminal"], "raw_receipt_agreement")
        require([r["k"] for r in orders] == list(range(1, len(orders) + 1)), "order_sequence")
        require(terminal["exit_code"] == receipt["exit_code"], "exit_agreement")
        analysed = [analyse_order(row, config) for row in orders]
        successful_orders += sum(row["complete"] for row in analysed)
        elapsed = terminal["elapsed_before_terminal_ms"]
        stages = terminal["stage_ms"]
        stage_total = sum(stages.values(), Decimal(0))
        require(elapsed >= stage_total - Decimal("0.0001"), "elapsed_smaller_than_stages")
        require(abs(terminal["compute_read_release_ms_subtracted_diagnostic"]
                    - (elapsed - terminal["provisional_output_ms"])) <= Decimal("0.000002"),
                "subtracted_time_identity")
        complete = terminal["terminal_status"] == "completed"
        require(complete == (receipt["exit_code"] == 0), "terminal_success")
        metrics: dict[str, Any] = {
            "id": stem, "n": config["n"], "s": config["s"], "complete": complete,
            "exit_code": receipt["exit_code"], "last_order": terminal["last_order"],
            "elapsed_before_terminal_ms": str(elapsed), "stage_ms": {k: str(v) for k, v in stages.items()},
            "census_balls": terminal["census_balls"],
            "census_logical_bytes": terminal["census_balls"] * config["sizeof_ball_data"],
            "raw_candidates": terminal["raw_candidates"], "orders": analysed,
            "source_raw_sha256": sha(SOURCE / (stem + ".raw.txt")),
        }
        if complete:
            metrics.update(
                full_build_fraction=str(stages["full"] / elapsed),
                generation_fraction=str(stages["generation"] / elapsed),
                elapsed_minus_full_build_diagnostic_ms=str(elapsed - stages["full"]),
                zero_full_cost_fixed_remainder_speedup_ceiling=str(elapsed / (elapsed - stages["full"])),
                hypothetical_timing_scope="Algebraic fixed-remainder diagnostic of this 8k observation only. "
                                          "No observed speedup, scaling extrapolation or lazy runtime prediction",
                all_forest_arenas_logical_bytes=sum(r["forest_logical_bytes"] for r in analysed),
                all_minima=sum(r["certificate_minima"] for r in analysed),
                all_nodes=sum(r["certificate_nodes"] for r in analysed),
                all_parent_refs=sum(r["certificate_parent_refs"] for r in analysed),
            )
        attempts.append(metrics)
    require(successful_orders == 44 and sum(len(a["orders"]) for a in attempts) == 46,
            "population_nonvacuity")
    require(sum(a["complete"] for a in attempts) == 3, "attempt_population")
    pairs = [a for a in attempts if a["n"] == 8000]
    signatures = [{key: [{k: v for k, v in row.items() if k != "observed_build_ms"}
                         for row in attempt[key]] for key in ["orders"]} for attempt in pairs]
    require(all(s == signatures[0] for s in signatures[1:]), "s_volume_signature")
    return {
        "schema": "mhgp7-full-mono-independent-volume-analysis-v1",
        "status": "completed_sealed_observation_analysis", "public_status": "not_claimed",
        "python_optimized": not __debug__, "source_policy": "eager_all_incident_facets_v1",
        "source_producer_sha256": "e02d163ced2074d6b91fe810c112fb946aca56a7724c8e2ae586e3baee97c170",
        "successful_orders": successful_orders, "order_rows": 46,
        "attempts": attempts, "s_volume_signatures_equal": True,
        "scope": "Reanalysis of existing CPU captures, no motor/build. Exact record identities and "
                 "representation bounds; no forest digest, lazy success, RSS reduction or SLO claim.",
        "analysis_script_sha256": sha(Path(__file__)),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--result", type=Path, required=True)
    args = parser.parse_args()
    result = analyse()
    args.result.resolve().relative_to(AUDIT)
    args.result.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps({"status": result["status"], "successful_orders": result["successful_orders"],
                      "order_rows": result["order_rows"], "s_volume_signatures_equal": True}))


if __name__ == "__main__":
    main()
