#!/usr/bin/env python3
"""Read-only review of CLOSED FULL lazy paired/scale observations, ROOT GO only.

Default invocation is inert. --execute requires a reviewed script SHA256 and
--paired PATH=SHA256. Optional --scale PATH=SHA256 may occur once per scale16/32.
Output is JSON on stdout only: no engine, compiler, file write, Git or GCP call.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import types

ROOT = Path("/workspaces/E-HGP")
BASE = ROOT / "build/v7_full_lazy_20260905_probe_controller"
COMMON = ROOT / "build/v7_full_lazy_20260905_controller/publish.py"
COMMON_SHA = "5c7f18a2577ee388a8f9652c3596ffe9ab9ade6bbc3101ae45f18fc91da6dfba"
CONTROLLER = BASE / "capture.py"
CONTROLLER_SHA = "417ccc3b47bb7591405f3af99bf7591bf2019794aa4535077436ce4889c4adfa"
JUDGE = ROOT / "morsehgp3D_v7/bench/full_gabriel_lazy_probe_audit.py"
JUDGE_SHA = "8d8a612aa973cb79e60e97a6675f63684ddd8892cfc550716c20620c4d6930ef"
POLICY = ROOT / "morsehgp3D_v7/bench/full_gabriel_cache_policy_audit.py"
POLICY_SHA = "8f8aed03755d9c92775566b21d4fdd9dcba31f171adf4b83e9802a988a450370"
MICRO_SHA = "9ce369e2d6085e1e7ac0b95c03a84f1793f42d0107a2b2a15474644d880ce1b2"
FRONT_FIELDS = (
    "frontier_ledger_closed", "rank_window_regular", "rank_relevant_extra_shell",
    "raw_candidates", "unique_candidates", "candidate_capacity_observed",
    "census_balls", "ball_capacity_observed", "generation_cap_refus", "emitted_at_refus",
    "wave_peak_tasks", "alive_peak_rects", "pair_mass_expected_per_lane",
    "ledger_q2_emitted", "ledger_q2_killed", "ledger_q3_emitted", "ledger_q3_killed",
    "ledger_q4_emitted", "ledger_q4_killed", "invariant_jneg", "wspd_witness_nodes",
    "wspd_corner_evals", "q4_core_site_tests", "q4_completions", "prefilter_query_nodes",
    "census_query_nodes", "census_merge_peak_bytes",
)
SAME_ORDER_FIELDS = (
    "k", "certificate_digest", "certificate_minima", "certificate_nodes",
    "certificate_parent_refs", "minimum_catalogue_records", "connection_catalogue_records",
    "terminal_roots", "terminal_coverage_points", "input_records", "no_op_connections",
)
WORK_FIELDS = (
    "aliases", "face_visits", "alias_hits", "portal_requests", "chain_steps",
    "terminal_direct", "max_chain_length", "normalized_anchors", "successor_steps",
    "no_op_connections", "meb_calls", "geometry_meb_calls", "meb_supports",
    "query_nodes", "query_leaves", "query_range_skips", "minimum_lookups",
    "minimum_hits", "cache_lookups", "cache_hits", "cache_inserts", "cache_skips",
    "singleton_intruder_resolutions", "direct_lookups",
)


def load(path, pin):
    import hashlib
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != pin:
        raise ValueError("protocol SHA256 mismatch: " + str(path))
    module = types.ModuleType("inert_aggregate_" + path.stem + "_" + pin[:8])
    module.__file__ = str(path)
    exec(compile(raw, str(path), "exec"), module.__dict__)
    return module


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--expected-script-sha256")
    parser.add_argument("--paired", metavar="PATH=SHA256")
    parser.add_argument("--scale", action="append", default=[], metavar="PATH=SHA256")
    args = parser.parse_args(argv)
    if not args.execute:
        print("prepared_not_executed: no capture read; ROOT GO required")
        return 0
    H, C = load(COMMON, COMMON_SHA), load(CONTROLLER, CONTROLLER_SHA)
    J, P = load(JUDGE, JUDGE_SHA), load(POLICY, POLICY_SHA)
    require, reader, watches = H.require, H.Reader(), []
    require(re.fullmatch("[0-9a-f]{64}", args.expected_script_sha256 or "") is not None,
            "explicit script SHA256 required")
    for path, pin in ((Path(__file__), args.expected_script_sha256),
                      (COMMON, COMMON_SHA), (CONTROLLER, CONTROLLER_SHA),
                      (JUDGE, JUDGE_SHA), (POLICY, POLICY_SHA)):
        reader.pinned(path, pin)

    def identity(argument):
        require(isinstance(argument, str), "PATH=SHA256 required")
        name, sep, pin = argument.rpartition("=")
        require(sep and re.fullmatch("[0-9a-f]{64}", pin), "PATH=SHA256 syntax")
        path = Path(name)
        require(path.is_absolute() and path.name == "receipt.json" and
                path.resolve().is_relative_to(BASE) and not path.is_symlink(), "receipt scope")
        return path, pin

    def attempt(directory, names, plan, position):
        n, separation, policy, cap = plan["planned_sequence"][position]
        label = f"n{n}_s{separation}_k10_{policy}_c{cap}"
        own = sorted(name for name in names if name.startswith(label + "."))
        output = {"attempt_id": label, "n": n, "s": separation, "alias_policy": policy,
                  "cache_entries": cap, "available_artifacts": own, "attempt_success": False}

        def value(suffix):
            return H.json_value(reader.get(directory / (label + suffix)))

        if not own:
            return output | {"status": "not_attempted_in_capture", "exit_code": None}, None
        if label + ".verdict.json" not in names:
            return output | {"status": "interrupted_nonterminal_capture", "exit_code": None,
                             "elapsed_seconds": None, "cause": "unknown"}, None
        verdict = value(".verdict.json")
        if verdict.get("status") != "completed":
            command_name = label + ".command.json"
            command = value(".command.json") if command_name in names else {}
            return output | {"status": "invalid_or_censored_capture_not_promoted", "verdict": verdict,
                             "observed_command": command}, None
        record, intent = value(".receipt.json"), value(".intent.json")
        command, raw = value(".command.json"), reader.get(directory / (label + ".raw.txt"))
        require(all(record[key] == val for key, val in command.items()), "command mirror:" + label)
        require(record["streams"] == {label + ".raw.txt": {
            "bytes": len(raw), "sha256": H.sha(raw)}}, "raw stream binding:" + label)
        judged, policy_judged = J.judge(raw.decode(), record, intent, plan), P.judge(raw, record)
        require(judged["attempt_success"] is policy_judged["attempt_success"] is
                verdict["attempt_success"] and judged["input_digest"] == verdict["input_digest"] and
                judged["certificate_digest"] == verdict["certificate_digest"], "verdict binding:" + label)
        expected_judge = judged | {"raw_sha256": H.sha(raw)}
        for mode in ("normal", "optimized"):
            require(value(".judge_" + mode + ".stdout") == expected_judge,
                    "recorded judge differs:" + label + ":" + mode)
        baseline = H.json_value(reader.get(directory / "sources_before.json"))
        require(value(".sources_before.json") == baseline == value(".sources_after.json"),
                "attempt source snapshots differ:" + label)
        terminal = record["terminal"]
        fields = dict(line.strip().split(": ", 1) for line in raw.decode().splitlines()
                      if not line.startswith("{") and not line.startswith("Command exited"))
        output.update(status="completed_relative" if judged["attempt_success"] else "refused_terminal",
                      attempt_success=judged["attempt_success"], exit_code=record["exit_code"],
                      outcome=terminal["outcome"], reason=terminal["reason"],
                      raw_sha256=H.sha(raw), receipt_sha256=H.sha(reader.get(directory / (label + ".receipt.json"))),
                      input_digest=terminal["input_digest"], certificate_digest=terminal["certificate_digest"],
                      successful_orders_diagnostic=terminal["completed_orders_diagnostic"],
                      complete_elapsed_before_terminal_ms=(terminal["elapsed_before_terminal_ms"]
                                                          if judged["attempt_success"] else None),
                      observed_elapsed_before_terminal_ms_diagnostic=terminal["elapsed_before_terminal_ms"],
                      command_elapsed_seconds=command["elapsed_seconds"],
                      max_rss_kib=int(fields["Maximum resident set size (kbytes)"]),
                      stage_ms=terminal["stage_ms"], front={key: terminal[key] for key in FRONT_FIELDS},
                      first_c_supplement=policy_judged,
                      orders=[{key: row[key] for key in SAME_ORDER_FIELDS + WORK_FIELDS + (
                          "outcome", "expand_ms", "build_ms", "read_ms", "digest_ms", "release_ms",
                          "rss_mib_sample", "hwm_mib_sample")} for row in record["orders"]])
        require(judged["attempt_success"] or output["certificate_digest"] == "", "refusal promoted")
        return output, record

    def campaign(argument, phase, prior=None):
        path, pin = identity(argument)
        if phase == "paired":
            require(path == BASE / "heavy_paired_resume/receipt.json", "only the fresh resumed pair is in scope")
        record = H.json_value(reader.pinned(path, pin))
        directory, names = path.parent, H.files_in(path.parent)
        require(record.get("schema") == C.SCHEMA and record.get("kind") == "heavy" and
                record.get("phase") == phase and record.get("status") in ("completed", "failed") and
                record.get("sources_stable") is True and record.get("ended"), "closed campaign required")
        require(names == set(record["artifacts"]) | {"receipt.json"}, "closed campaign inventory")
        for name, expected in record["artifacts"].items():
            require(H.safe_name(name), "unsafe capture member")
            reader.pinned(directory / name, expected)
        plan = H.json_value(reader.get(directory / "protocol.json"))
        admission = H.json_value(reader.get(directory / "admission.json"))
        sequence = C.PAIRED if phase == "paired" else [[16000 if phase == "scale16" else 32000,
                                                       8, "lazy", 1000000]]
        require(plan["planned_sequence"] == sequence and plan["kmax"] == 10 and
                plan["binary"] == record["binary"] and plan["binary_sha256"] == record["binary_sha256"] and
                plan["threads"] == 1 and plan["cpu_affinity"] == [6] and plan["public_status"] == "not_claimed" and
                plan["scope"] == "horizontal_relative_orders_not_integrated_inter_k_tower" and
                plan["authority"] == C.AUTHORITY and plan["slo_claim"] is False,
                "campaign protocol binding")
        require(admission["phase"] == phase and admission["paired"] == prior, "scale/paired admission binding")
        require(admission["micro_receipt"] == str(BASE / "micro_admission/receipt.json") and
                admission["micro_sha256"] == MICRO_SHA, "fixed micro admission pin")
        _, micro = C.verify_receipt(admission["micro_receipt"], admission["micro_sha256"], "micro")
        require(micro["binary"] == plan["binary"] and micro["binary_sha256"] == plan["binary_sha256"],
                "micro binary differs from measured binary")
        C.qualification(admission["qualification"]["path"], admission["qualification"]["sha256"])
        baseline = H.json_value(reader.get(directory / "sources_before.json"))
        require(H.json_value(reader.get(directory / "sources_after.json")) == baseline and
                C.snapshot(ROOT / record["binary"]) == baseline, "campaign source/binary drift")
        require(plan["probe_sha256"] == baseline["files"][C.PROBE] and
                plan["producer_sha256"] == baseline["files"][C.PRODUCER] and
                plan["digest_header_sha256"] == baseline["files"][C.DIGEST] and
                plan["judge_sha256"] == baseline["files"][C.JUDGE], "declared source binding")
        outputs, attempts, prefix = [], [], []
        prefix_open = True
        for i in range(len(sequence)):
            output, captured = attempt(directory, names, plan, i)
            outputs.append(output)
            attempts.append(captured)
            label = output["attempt_id"]
            if prefix_open and label + ".verdict.json" in names:
                verdict = H.json_value(reader.get(directory / (label + ".verdict.json")))
                prefix.append(verdict)
                prefix_open = verdict.get("status") == "completed"
            else:
                prefix_open = False
        require(record["attempts"] == prefix, "closed verdict prefix mismatch")
        complete = all(row["attempt_success"] for row in outputs)
        require(record["all_successful"] is complete, "global success binding")
        require(record["semantic_digests_equal"] is complete, "global digest availability binding")
        if complete:
            C.verify_receipt(path, pin, "heavy")
        watches.append((directory, names, ROOT / record["binary"], baseline))
        return {"path": str(path), "sha256": pin, "phase": phase, "capture_status": record["status"],
                "all_successful": complete, "attempts": outputs}, attempts

    paired, records = campaign(args.paired, "paired")
    comparisons = []
    for separation in (8, 10, 12):
        selected = [row for row in paired["attempts"] if row["s"] == separation]
        eager, lazy = sorted(selected, key=lambda row: row["alias_policy"])
        if not (eager["attempt_success"] and lazy["attempt_success"]):
            comparisons.append({"s": separation, "status": "unavailable_incomplete_pair", "speedup_ratio": None})
            continue
        require(eager["input_digest"] == lazy["input_digest"] and
                eager["certificate_digest"] == lazy["certificate_digest"], "paired digest disagreement")
        require(eager["front"] == lazy["front"], "paired deterministic front counters disagree")
        compared, unsaturated = 0, 0
        for e, l in zip(eager["orders"], lazy["orders"]):
            require(all(e[key] == l[key] for key in SAME_ORDER_FIELDS), "paired order semantics/catalogues differ")
            require(e["face_visits"] - l["face_visits"] == (e["k"] + 1) * e["connection_catalogue_records"],
                    "paired incidence work identity")
            compared += 1
            if l["cache_skips"] == 0:
                require(l["portal_requests"] - l["singleton_intruder_resolutions"] == e["portal_requests"] and
                        l["chain_steps"] == e["chain_steps"] and
                        l["meb_calls"] == e["meb_calls"] + l["singleton_intruder_resolutions"],
                        "paired unsaturated path identities")
                unsaturated += 1
        require(compared == 10, "paired order nonvacuum")
        require(lazy["complete_elapsed_before_terminal_ms"] > 0 and eager["complete_elapsed_before_terminal_ms"] > 0,
                "zero elapsed successful observation")
        comparisons.append({"s": separation, "status": "paired_observation_only_not_statistical_speedup",
                            "orders_compared": compared, "front_counters_compared": len(FRONT_FIELDS),
                            "unsaturated_orders_path_compared": unsaturated,
                            "saturated_orders_path_not_compared": compared - unsaturated,
                            "eager_over_lazy_elapsed_ratio_observed":
                            eager["complete_elapsed_before_terminal_ms"] / lazy["complete_elapsed_before_terminal_ms"],
                            "eager_minus_lazy_peak_rss_kib_observed": eager["max_rss_kib"] - lazy["max_rss_kib"]})
    all_s = paired["all_successful"]
    if all_s:
        require(len({row["input_digest"] for row in paired["attempts"]}) == 1 and
                len({row["certificate_digest"] for row in paired["attempts"]}) == 1,
                "cross-s input/global digest disagreement")
        require(all(all(a[key] == b[key] for key in SAME_ORDER_FIELDS)
                    for row in paired["attempts"][1:]
                    for a, b in zip(paired["attempts"][0]["orders"], row["orders"])),
                "cross-s per-order semantics/catalogues disagreement")
    scales, phases = [], set()
    for argument in args.scale:
        require(all_s, "scales require independently verified six-success paired admission")
        path, pin = identity(argument)
        phase = H.json_value(reader.pinned(path, pin)).get("phase")
        require(phase in ("scale16", "scale32") and phase not in phases, "scale phase inventory")
        phases.add(phase)
        result, _ = campaign(argument, phase, {"path": paired["path"], "sha256": paired["sha256"]})
        result["input_comparison_to_8k"] = "not_applicable_distinct_cloud_size"
        scales.append(result)
    for directory, names, binary, baseline in watches:
        require(H.files_in(directory) == names and C.snapshot(binary) == baseline, "terminal campaign drift")
    reader.recheck()
    result = {"schema": "mhgp7-full-lazy-mono-readonly-review-v1", "status": "closed_evidence_reviewed",
              "script_sha256": args.expected_script_sha256, "controller_sha256": CONTROLLER_SHA,
              "v2_judge_sha256": JUDGE_SHA, "first_c_supplement_sha256": POLICY_SHA,
              "public_status": "not_claimed", "scope": "horizontal_relative_orders_not_integrated_inter_k_tower",
              "paired": paired, "paired_comparisons": comparisons, "cross_s_semantics_checked": all_s,
              "scales": scales, "captured_files_read": len(reader.files),
              "rss_scope": "GNU_time_peak_is_process_global; per_order_RSS_sample_is_after_Builder_destruction; HWM_cumulative",
              "timing_scope": "elapsed_before_terminal_includes_digest_and_provisional_output; no_subtracted_SLO",
              "geometry_or_catalogue_completeness_proved": False, "statistical_speedup_qualified": False,
              "integrated_tower_qualified": False, "slo_claim": False, "engine_runs": 0, "gcp_used": False}
    print(H.encode(result).decode(), end="")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, KeyError, TypeError, IndexError) as error:
        print("Review refused:", error)
        raise SystemExit(1)
