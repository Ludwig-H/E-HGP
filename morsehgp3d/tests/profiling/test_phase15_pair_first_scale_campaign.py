#!/usr/bin/env python3
"""Contract tests for the future complete pair-first scale campaign."""

from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import textwrap
import time
import unittest

DIRECTORY = Path(__file__).resolve().parent
HARNESS = DIRECTORY / "phase15_pair_first_scale_campaign.py"
PLAN = DIRECTORY / "phase15_pair_first_scale_campaign_v1.json"
SPEC = importlib.util.spec_from_file_location(
    "phase15_pair_first_scale_campaign", HARNESS
)
assert SPEC is not None and SPEC.loader is not None
campaign = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(campaign)


def make_complete_run(request: dict[str, object]) -> dict[str, object]:
    point_count = request["point_count"]
    assert isinstance(point_count, int)
    total = point_count * (point_count - 1) // 2
    candidate = min(point_count * 4, total)
    accepted = min(candidate, point_count * 2)
    binary_sha256 = "b" * 64
    return {
        "algorithm_scope": campaign.ALGORITHM_SCOPE,
        "backend": campaign.BACKEND,
        "binary_sha256": binary_sha256,
        "caps": copy.deepcopy(request["caps"]),
        "censor_reason": None,
        "cloud": {
            "canonical_binary64_sha256": hashlib.sha256(
                str(request["run_id"]).encode("ascii")
            ).hexdigest(),
            "construction_count": 1,
            "fresh_for_measured_run": True,
            "request_sha256": request["cloud_request_sha256"],
        },
        "closure": {
            "accepted_pair_count": accepted,
            "candidate_pair_mass": candidate,
            "certified_pruned_pair_mass": total - candidate,
            "checkpoint_closed": True,
            "classifier_session_count": candidate,
            "exact_replay_closed": True,
            "frontier_empty": True,
            "payload_closed": True,
            "rejected_pair_count": candidate - accepted,
            "total_unordered_pair_mass": total,
            "unique_candidate_pair_count": candidate,
            "unresolved_pair_mass": 0,
        },
        "family": copy.deepcopy(request["family"]),
        "git_sha": "a" * 40,
        "implementation": {
            "complete_pair_frontier": True,
            "component_only": False,
            "dense_pair_scan": False,
            "delaunay": False,
            "emst": False,
            "global_pair_matrix": False,
            "offline_oracle_in_timed_path": False,
            "proxy": False,
            "uses_boruvka": False,
        },
        "mode": campaign.MODE,
        "point_count": point_count,
        "profile": campaign.PROFILE,
        "requested_order_max": request["requested_order_max"],
        "run_id": request["run_id"],
        "schema": campaign.RUN_SCHEMA,
        "seed": request["seed"],
        "status": "complete",
        "timings_ns": {
            "certified_frontier": 3_000_000,
            "cold_e2e": 20_000_000,
            "compaction": 500_000,
            "exact_pair_classification": 2_000_000,
            "morton_lbvh": 2_000_000,
            "persistence": 500_000,
            "resident_replay": 5_000_000,
            "warm_e2e_fresh_cloud": 10_000_000,
            "yao48_bank_fill": 1_000_000,
        },
        "work": {
            "bank_updates": point_count * 10,
            "classified_pairs": candidate,
            "d2h_intermediate_candidate_batches": 0,
            "dense_fallback_pair_visits": 0,
            "device_capacity_bytes": 1_000_000_000,
            "device_peak_bytes": 100_000_000,
            "frontier_peak_records": 1024,
            "host_callbacks_per_pair": 0,
            "host_capacity_bytes": 1_000_000_000,
            "host_peak_bytes": 100_000_000,
            "node_visits": point_count * 20,
            "output_pairs": accepted,
            "radial_receipts": point_count * 3,
        },
    }


def make_node_visit_censored_run(request: dict[str, object]) -> dict[str, object]:
    run = make_complete_run(request)
    closure = run["closure"]
    work = run["work"]
    caps = run["caps"]
    assert (
        isinstance(closure, dict) and isinstance(work, dict) and isinstance(caps, dict)
    )
    unresolved = 1000
    closure["certified_pruned_pair_mass"] -= unresolved
    closure["unresolved_pair_mass"] = unresolved
    closure["frontier_empty"] = False
    closure["payload_closed"] = False
    work["node_visits"] = caps["node_visits"]
    run["status"] = "censored"
    run["censor_reason"] = "node_visit_cap"
    return run


def capability_record() -> dict[str, object]:
    return {
        "algorithm_scope": campaign.ALGORITHM_SCOPE,
        "backend": campaign.BACKEND,
        "complete_pair_frontier": True,
        "component_only": False,
        "dense_fallback_available": False,
        "dense_pair_scan": False,
        "delaunay": False,
        "emst": False,
        "exact_pair_classifier": True,
        "global_pair_matrix": False,
        "maximum_requested_order": 10,
        "mode": campaign.MODE,
        "multi_order_single_pass": True,
        "pair_mass_receipts": True,
        "profile": campaign.PROFILE,
        "proxy": False,
        "resident_resumable_frontier": True,
        "run_schema": campaign.RUN_SCHEMA,
        "schema": campaign.CAPABILITY_SCHEMA,
        "supported_families": list(campaign.FAMILY_IDS),
        "uses_boruvka": False,
    }


def gate_metrics(gate: str) -> dict[str, object]:
    if gate == "bounded_cpu_differential_k1_k2_k10":
        return {"case_count": 12, "mismatch_count": 0, "orders": [1, 2, 10]}
    if gate == "offline_k1_direct_pair_reduction_equals_emst_fusion_times":
        return {
            "case_count": 12,
            "closed_cut_mismatch_count": 0,
            "fusion_time_mismatch_count": 0,
            "multifusion_mismatch_count": 0,
            "strict_cut_mismatch_count": 0,
        }
    if gate == "offline_k2_fusions_no_later_than_two_delaunay_edge_triangle_deadlines":
        return {"comparison_count": 12, "deadline_violation_count": 0}
    if gate == "tile_schedule_resume_equivalence":
        return {"case_count": 12, "transcript_mismatch_count": 0}
    if gate == "compute_sanitizer_memcheck_racecheck":
        return {
            "instrumented_run_count": 2,
            "leak_count": 0,
            "memcheck_error_count": 0,
            "racecheck_error_count": 0,
        }
    if gate == "n12500_growth_falsifier":
        return {
            "dense_fallback_pair_visits": 0,
            "frontier_empty": True,
            "growth_within_registered_caps": True,
            "point_count": 12_500,
            "unresolved_pair_mass": 0,
        }
    raise AssertionError(f"unknown test gate: {gate}")


def preflight_record(
    binary_sha256: str, git_sha: str, directory: Path
) -> dict[str, object]:
    now = int(time.time())
    environment = copy.deepcopy(campaign.PREFLIGHT_ENVIRONMENT_TEMPLATE)
    environment.update(
        {
            "gce_deadline_epoch": now + 28_800,
            "guest_deadline_epoch": now + 27_000,
            "guest_shutdown_delay_seconds": 27_000,
            "instance_generation": "test-generation-1",
            "instance_name": "ehgp-blackwell-spot-test",
            "max_run_duration_seconds": 28_800,
            "preflight_epoch": now,
            "zone": "europe-west4-a",
        }
    )
    gates: dict[str, object] = {}
    for index, gate in enumerate(campaign.PREFLIGHT_GATES):
        artifact_file = f"gate-{index}.json"
        artifact = {
            "binary_sha256": binary_sha256,
            "execution_scope": "offline_preflight",
            "gate": gate,
            "git_sha": git_sha,
            "metrics": gate_metrics(gate),
            "schema": campaign.GATE_ARTIFACT_SCHEMA,
            "status": "passed",
        }
        payload = (campaign.canonical_json(artifact) + "\n").encode("ascii")
        (directory / artifact_file).write_bytes(payload)
        gates[gate] = {
            "artifact_file": artifact_file,
            "artifact_sha256": hashlib.sha256(payload).hexdigest(),
            "gate": gate,
            "git_sha": git_sha,
            "status": "passed",
        }
    return {
        "backend": campaign.BACKEND,
        "binary_sha256": binary_sha256,
        "component_measurement_used_as_gate": False,
        "environment": environment,
        "gates": gates,
        "git_sha": git_sha,
        "mode": campaign.MODE,
        "profile": campaign.PROFILE,
        "proxy_measurement_used_as_gate": False,
        "schema": campaign.PREFLIGHT_SCHEMA,
    }


class PairFirstScaleCampaignTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.plan = campaign.validate_plan(campaign.read_json(PLAN, "test plan"))
        cls.requests = campaign.expected_run_requests(cls.plan)

    def test_registered_matrix_is_50k_then_millions(self) -> None:
        counts: dict[int, int] = {}
        for request in self.requests:
            point_count = request["point_count"]
            counts[point_count] = counts.get(point_count, 0) + 1
        self.assertEqual(
            counts,
            {50_000: 30, 1_000_000: 9, 10_000_000: 6, 30_000_000: 3},
        )
        self.assertEqual(len(self.requests), 48)
        self.assertEqual(
            tuple(item["id"] for item in self.plan["families"]),
            campaign.FAMILY_IDS,
        )

    def test_caps_are_linear_and_frontier_is_tile_bounded(self) -> None:
        caps_50k = campaign.computed_caps(50_000, self.plan)
        caps_1m = campaign.computed_caps(1_000_000, self.plan)
        for field in (
            "bank_updates",
            "classified_pairs",
            "node_visits",
            "output_pairs",
            "radial_receipts",
        ):
            self.assertEqual(caps_1m[field], caps_50k[field] * 20)
        self.assertEqual(caps_1m["frontier_records"], caps_50k["frontier_records"])
        self.assertEqual(caps_50k["frontier_records"], 4096 * 640)
        self.assertGreaterEqual(caps_50k["classified_pairs"], 50_000 * 48 * 10)

    def test_plan_explicitly_blocks_component_and_proxy_claims(self) -> None:
        self.assertEqual(
            self.plan["execution_status"],
            "blocked_until_complete_pair_first_binary",
        )
        policy = self.plan["benchmark_claim_policy"]
        self.assertFalse(policy["component_measurements_are_results"])
        self.assertFalse(policy["proxy_measurements_are_results"])
        self.assertFalse(policy["dense_oracle_measurements_are_results"])

    def test_low_order_guardrails_are_offline_preflight_only(self) -> None:
        self.assertIn(
            "offline_k1_direct_pair_reduction_equals_emst_fusion_times",
            self.plan["preflight_gates"],
        )
        self.assertIn(
            "offline_k2_fusions_no_later_than_two_delaunay_edge_triangle_deadlines",
            self.plan["preflight_gates"],
        )
        capabilities = capability_record()
        self.assertFalse(capabilities["emst"])
        self.assertFalse(capabilities["delaunay"])
        run = make_complete_run(self.requests[0])
        self.assertFalse(run["implementation"]["emst"])
        self.assertFalse(run["implementation"]["delaunay"])
        self.assertFalse(run["implementation"]["offline_oracle_in_timed_path"])

    def test_missing_complete_binary_blocks_before_output(self) -> None:
        with tempfile.TemporaryDirectory(prefix="pair-first-missing-") as raw:
            directory = Path(raw)
            output = directory / "result.json"
            with self.assertRaisesRegex(
                campaign.ContractError,
                "complete pair-first binary is absent",
            ):
                campaign.execute_campaign(
                    plan=self.plan,
                    plan_path=PLAN,
                    binary=directory / "missing-complete-binary",
                    preflight_path=directory / "missing-preflight.json",
                    git_sha="a" * 40,
                    output=output,
                )
            self.assertFalse(output.exists())

    def test_cli_missing_binary_is_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory(prefix="pair-first-cli-") as raw:
            directory = Path(raw)
            output = directory / "result.json"
            completed = subprocess.run(
                [
                    sys.executable,
                    str(HARNESS),
                    "run",
                    "--binary",
                    str(directory / "missing"),
                    "--preflight",
                    str(directory / "missing-preflight"),
                    "--git-sha",
                    "a" * 40,
                    "--output",
                    str(output),
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
                text=True,
            )
            self.assertEqual(completed.returncode, 2)
            self.assertEqual(completed.stdout, "")
            self.assertIn("complete pair-first binary is absent", completed.stderr)
            self.assertFalse(output.exists())

    def test_binary_cannot_change_during_capability_handshake(self) -> None:
        with tempfile.TemporaryDirectory(prefix="pair-first-mutating-binary-") as raw:
            directory = Path(raw)
            binary = directory / "self-mutating-complete-binary"
            binary.write_text(
                textwrap.dedent(f"""\
                    #!/usr/bin/env python3
                    import json
                    from pathlib import Path

                    with Path(__file__).open("a", encoding="ascii") as stream:
                        stream.write("# self mutation\\n")
                    print(json.dumps({capability_record()!r}, ensure_ascii=True, sort_keys=True, separators=(",", ":")))
                    """),
                encoding="ascii",
            )
            binary.chmod(binary.stat().st_mode | stat.S_IXUSR)
            binary_sha256 = hashlib.sha256(binary.read_bytes()).hexdigest()
            preflight_path = directory / "preflight.json"
            preflight_path.write_text(
                campaign.canonical_json(
                    preflight_record(binary_sha256, "a" * 40, directory)
                )
                + "\n",
                encoding="ascii",
            )
            with self.assertRaisesRegex(
                campaign.ContractError, "changed during capability handshake"
            ):
                campaign.execute_campaign(
                    plan=self.plan,
                    plan_path=PLAN,
                    binary=binary,
                    preflight_path=preflight_path,
                    git_sha="a" * 40,
                    output=directory / "result.json",
                )

    def test_component_capability_cannot_impersonate_complete_binary(self) -> None:
        capabilities = capability_record()
        capabilities["complete_pair_frontier"] = False
        capabilities["component_only"] = True
        with self.assertRaisesRegex(campaign.ContractError, "complete non-proxy"):
            campaign.validate_capabilities(capabilities, self.plan)

    def test_proxy_and_industrial_delaunay_capabilities_are_rejected(self) -> None:
        for field in ("proxy", "delaunay"):
            capabilities = capability_record()
            capabilities[field] = True
            with self.subTest(field=field), self.assertRaisesRegex(
                campaign.ContractError,
                "complete non-proxy",
            ):
                campaign.validate_capabilities(capabilities, self.plan)

    def test_complete_run_closes_mass_and_one_to_one_classification(self) -> None:
        request = self.requests[0]
        run = make_complete_run(request)
        validated = campaign.validate_run(run, request)
        self.assertEqual(validated["status"], "complete")
        closure = validated["closure"]
        self.assertEqual(
            closure["candidate_pair_mass"]
            + closure["certified_pruned_pair_mass"]
            + closure["unresolved_pair_mass"],
            closure["total_unordered_pair_mass"],
        )
        self.assertEqual(
            closure["classifier_session_count"],
            closure["candidate_pair_mass"],
        )
        self.assertEqual(
            closure["accepted_pair_count"] + closure["rejected_pair_count"],
            closure["classifier_session_count"],
        )

    def test_candidate_mass_must_be_unique_even_when_censored(self) -> None:
        request = self.requests[0]
        run = make_node_visit_censored_run(request)
        run["closure"]["unique_candidate_pair_count"] -= 1
        with self.assertRaisesRegex(campaign.ContractError, "mass is not unique"):
            campaign.validate_run(run, request)

    def test_output_is_exactly_the_accepted_classifier_partition(self) -> None:
        request = self.requests[0]
        run = make_complete_run(request)
        run["work"]["output_pairs"] -= 1
        with self.assertRaisesRegex(campaign.ContractError, "output differs"):
            campaign.validate_run(run, request)

    def test_broken_mass_identity_is_rejected(self) -> None:
        request = self.requests[0]
        run = make_complete_run(request)
        run["closure"]["certified_pruned_pair_mass"] -= 1
        with self.assertRaisesRegex(campaign.ContractError, "mass identity is open"):
            campaign.validate_run(run, request)

    def test_complete_run_cannot_retain_unresolved_mass(self) -> None:
        request = self.requests[0]
        run = make_complete_run(request)
        run["closure"]["certified_pruned_pair_mass"] -= 1
        run["closure"]["unresolved_pair_mass"] = 1
        with self.assertRaisesRegex(campaign.ContractError, "retains unresolved"):
            campaign.validate_run(run, request)

    def test_budget_censure_preserves_mass_and_checkpoint(self) -> None:
        request = self.requests[0]
        run = make_node_visit_censored_run(request)
        validated = campaign.validate_run(run, request)
        self.assertEqual(validated["status"], "censored")
        self.assertGreater(validated["closure"]["unresolved_pair_mass"], 0)
        self.assertTrue(validated["closure"]["checkpoint_closed"])
        self.assertEqual(validated["work"]["dense_fallback_pair_visits"], 0)

    def test_false_censor_reason_is_rejected(self) -> None:
        request = self.requests[0]
        run = make_node_visit_censored_run(request)
        run["work"]["node_visits"] -= 1
        with self.assertRaisesRegex(campaign.ContractError, "did not reach its cap"):
            campaign.validate_run(run, request)

    def test_nontriggering_time_and_memory_caps_still_apply(self) -> None:
        request = self.requests[0]
        run = make_node_visit_censored_run(request)
        run["timings_ns"]["warm_e2e_fresh_cloud"] = request["caps"]["wall_time"] + 1
        with self.assertRaisesRegex(campaign.ContractError, "non-triggering wall-time"):
            campaign.validate_run(run, request)

        run = make_node_visit_censored_run(request)
        run["work"]["device_peak_bytes"] = 800_000_000
        with self.assertRaisesRegex(
            campaign.ContractError, "non-triggering device-memory"
        ):
            campaign.validate_run(run, request)

    def test_timed_stage_sum_must_fit_inside_warm_e2e(self) -> None:
        request = self.requests[0]
        run = make_complete_run(request)
        for stage in (
            "morton_lbvh",
            "yao48_bank_fill",
            "certified_frontier",
            "exact_pair_classification",
            "compaction",
            "persistence",
        ):
            run["timings_ns"][stage] = 9_000_000
        with self.assertRaisesRegex(campaign.ContractError, "do not fit"):
            campaign.validate_run(run, request)

    def test_proxy_run_is_rejected_even_if_mass_closes(self) -> None:
        request = self.requests[0]
        run = make_complete_run(request)
        run["implementation"]["complete_pair_frontier"] = False
        run["implementation"]["proxy"] = True
        with self.assertRaisesRegex(campaign.ContractError, "forbidden or incomplete"):
            campaign.validate_run(run, request)

    def test_nearest_rank_p95_replays_from_thirty_fresh_clouds(self) -> None:
        samples = list(range(1, 31))
        self.assertEqual(campaign.nearest_rank(samples, 50), 15)
        self.assertEqual(campaign.nearest_rank(samples, 95), 29)

    def test_failed_scale_never_points_at_the_larger_scale(self) -> None:
        self.assertEqual(
            campaign.next_registered_point_count(self.requests, 30, 50_000),
            50_000,
        )
        self.assertEqual(
            campaign.next_registered_point_count(self.requests, 30, None),
            1_000_000,
        )
        self.assertIsNone(
            campaign.next_registered_point_count(
                self.requests,
                len(self.requests),
                None,
            )
        )

    def test_fresh_cloud_digests_cannot_repeat_between_runs(self) -> None:
        first = make_complete_run(self.requests[0])
        second = make_complete_run(self.requests[1])
        seen: set[str] = set()
        campaign.register_fresh_cloud_digest(seen, first)
        second["cloud"]["canonical_binary64_sha256"] = first["cloud"][
            "canonical_binary64_sha256"
        ]
        with self.assertRaisesRegex(
            campaign.ContractError, "reused one canonical cloud"
        ):
            campaign.register_fresh_cloud_digest(seen, second)

    def test_preflight_binds_target_guards_and_gate_evidence(self) -> None:
        with tempfile.TemporaryDirectory(prefix="pair-first-preflight-") as raw:
            directory = Path(raw)
            preflight_path = directory / "preflight.json"
            git_sha = "a" * 40
            binary_sha256 = "b" * 64
            value = preflight_record(binary_sha256, git_sha, directory)
            validated = campaign.validate_preflight(
                value,
                binary_sha256=binary_sha256,
                git_sha=git_sha,
                plan=self.plan,
                preflight_path=preflight_path,
            )
            self.assertEqual(validated["environment"]["project_label"], "project=e-hgp")

            hostile = copy.deepcopy(value)
            hostile["environment"]["project_label"] = "project=other"
            with self.assertRaisesRegex(campaign.ContractError, "project_label"):
                campaign.validate_preflight(
                    hostile,
                    binary_sha256=binary_sha256,
                    git_sha=git_sha,
                    plan=self.plan,
                    preflight_path=preflight_path,
                )

    def test_preflight_rejects_short_effective_guest_deadline(self) -> None:
        with tempfile.TemporaryDirectory(prefix="pair-first-deadline-") as raw:
            directory = Path(raw)
            preflight_path = directory / "preflight.json"
            git_sha = "a" * 40
            binary_sha256 = "b" * 64
            value = preflight_record(binary_sha256, git_sha, directory)
            environment = value["environment"]
            required = campaign.registered_remaining_timeout_seconds(self.plan)
            environment["guest_deadline_epoch"] = (
                environment["preflight_epoch"] + required - 1
            )
            environment["guest_shutdown_delay_seconds"] = required - 1
            with self.assertRaisesRegex(
                campaign.ContractError, "guest deadline cannot cover"
            ):
                campaign.validate_preflight(
                    value,
                    binary_sha256=binary_sha256,
                    git_sha=git_sha,
                    plan=self.plan,
                    preflight_path=preflight_path,
                    now_epoch=environment["preflight_epoch"],
                )

    def test_deadlines_are_revalidated_against_remaining_runs(self) -> None:
        with tempfile.TemporaryDirectory(prefix="pair-first-redeadline-") as raw:
            directory = Path(raw)
            value = preflight_record("b" * 64, "a" * 40, directory)
            environment = value["environment"]
            remaining = campaign.registered_remaining_timeout_seconds(self.plan, 1)
            campaign.validate_remaining_deadline_coverage(
                environment,
                self.plan,
                1,
                now_epoch=environment["guest_deadline_epoch"] - remaining,
            )
            with self.assertRaisesRegex(
                campaign.ContractError, "guest deadline cannot cover"
            ):
                campaign.validate_remaining_deadline_coverage(
                    environment,
                    self.plan,
                    1,
                    now_epoch=environment["guest_deadline_epoch"] - remaining + 1,
                )

    def test_preflight_rejects_false_gate_digest(self) -> None:
        with tempfile.TemporaryDirectory(prefix="pair-first-digest-") as raw:
            directory = Path(raw)
            preflight_path = directory / "preflight.json"
            git_sha = "a" * 40
            binary_sha256 = "b" * 64
            value = preflight_record(binary_sha256, git_sha, directory)
            first_gate = campaign.PREFLIGHT_GATES[0]
            value["gates"][first_gate]["artifact_sha256"] = "0" * 64
            with self.assertRaisesRegex(campaign.ContractError, "digest differs"):
                campaign.validate_preflight(
                    value,
                    binary_sha256=binary_sha256,
                    git_sha=git_sha,
                    plan=self.plan,
                    preflight_path=preflight_path,
                )

    def test_preflight_rejects_hostile_gate_metrics_even_with_matching_digest(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory(prefix="pair-first-hostile-gate-") as raw:
            directory = Path(raw)
            preflight_path = directory / "preflight.json"
            git_sha = "a" * 40
            binary_sha256 = "b" * 64
            value = preflight_record(binary_sha256, git_sha, directory)
            gate = "offline_k1_direct_pair_reduction_equals_emst_fusion_times"
            artifact_path = directory / value["gates"][gate]["artifact_file"]
            artifact = json.loads(artifact_path.read_text(encoding="ascii"))
            artifact["metrics"]["fusion_time_mismatch_count"] = 1
            payload = (campaign.canonical_json(artifact) + "\n").encode("ascii")
            artifact_path.write_bytes(payload)
            value["gates"][gate]["artifact_sha256"] = hashlib.sha256(
                payload
            ).hexdigest()
            with self.assertRaisesRegex(campaign.ContractError, "k=1 mismatch"):
                campaign.validate_preflight(
                    value,
                    binary_sha256=binary_sha256,
                    git_sha=git_sha,
                    plan=self.plan,
                    preflight_path=preflight_path,
                )

    def test_worker_runner_stops_at_first_censored_scale(self) -> None:
        with tempfile.TemporaryDirectory(prefix="pair-first-fake-binary-") as raw:
            directory = Path(raw)
            binary = directory / "complete-pair-first-fixture"
            binary.write_text(self._fake_binary_source(), encoding="utf-8")
            binary.chmod(binary.stat().st_mode | stat.S_IXUSR)
            binary_sha256 = hashlib.sha256(binary.read_bytes()).hexdigest()
            preflight = directory / "preflight.json"
            preflight.write_text(
                campaign.canonical_json(
                    preflight_record(binary_sha256, "a" * 40, directory)
                )
                + "\n",
                encoding="ascii",
            )
            output = directory / "result.json"
            artifact = campaign.execute_campaign(
                plan=self.plan,
                plan_path=PLAN,
                binary=binary,
                preflight_path=preflight,
                git_sha="a" * 40,
                output=output,
            )
            self.assertEqual(artifact["campaign_status"], "censored")
            self.assertEqual(artifact["censor_reason"], "run_budget_exhausted")
            self.assertEqual(len(artifact["runs"]), 1)
            self.assertEqual(artifact["next_point_count"], 50_000)
            self.assertFalse(artifact["qualification_claimed"])
            self.assertFalse(artifact["scalability_claimed"])
            self.assertIsNone(artifact["scientific_public_status"])
            self.assertEqual(
                output.read_text(encoding="ascii"),
                campaign.canonical_json(artifact) + "\n",
            )

    @staticmethod
    def _fake_binary_source() -> str:
        capabilities = capability_record()
        return textwrap.dedent(f"""\
            #!/usr/bin/env python3
            import hashlib
            import json
            from pathlib import Path
            import sys

            def emit(value):
                print(json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":")))

            if sys.argv[1] == "--phase15-pair-first-capabilities-json":
                emit({capabilities!r})
                raise SystemExit(0)
            if sys.argv[1] != "--phase15-pair-first-scale-run-json":
                raise SystemExit(2)
            request = json.load(sys.stdin)
            n = request["point_count"]
            total = n * (n - 1) // 2
            candidate = n
            unresolved = 1000
            digest = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
            emit({{
                "algorithm_scope": {campaign.ALGORITHM_SCOPE!r},
                "backend": {campaign.BACKEND!r},
                "binary_sha256": digest,
                "caps": request["caps"],
                "censor_reason": "node_visit_cap",
                "cloud": {{
                    "canonical_binary64_sha256": hashlib.sha256(request["run_id"].encode("ascii")).hexdigest(),
                    "construction_count": 1,
                    "fresh_for_measured_run": True,
                    "request_sha256": request["cloud_request_sha256"],
                }},
                "closure": {{
                    "accepted_pair_count": candidate,
                    "candidate_pair_mass": candidate,
                    "certified_pruned_pair_mass": total - candidate - unresolved,
                    "checkpoint_closed": True,
                    "classifier_session_count": candidate,
                    "exact_replay_closed": True,
                    "frontier_empty": False,
                    "payload_closed": False,
                    "rejected_pair_count": 0,
                    "total_unordered_pair_mass": total,
                    "unique_candidate_pair_count": candidate,
                    "unresolved_pair_mass": unresolved,
                }},
                "family": request["family"],
                "git_sha": "a" * 40,
                "implementation": {{
                    "complete_pair_frontier": True,
                    "component_only": False,
                    "dense_pair_scan": False,
                    "delaunay": False,
                    "emst": False,
                    "global_pair_matrix": False,
                    "offline_oracle_in_timed_path": False,
                    "proxy": False,
                    "uses_boruvka": False,
                }},
                "mode": {campaign.MODE!r},
                "point_count": n,
                "profile": {campaign.PROFILE!r},
                "requested_order_max": request["requested_order_max"],
                "run_id": request["run_id"],
                "schema": {campaign.RUN_SCHEMA!r},
                "seed": request["seed"],
                "status": "censored",
                "timings_ns": {{
                    "certified_frontier": 3000000,
                    "cold_e2e": 20000000,
                    "compaction": 500000,
                    "exact_pair_classification": 2000000,
                    "morton_lbvh": 2000000,
                    "persistence": 500000,
                    "resident_replay": 5000000,
                    "warm_e2e_fresh_cloud": 10000000,
                    "yao48_bank_fill": 1000000,
                }},
                "work": {{
                    "bank_updates": n * 10,
                    "classified_pairs": candidate,
                    "d2h_intermediate_candidate_batches": 0,
                    "dense_fallback_pair_visits": 0,
                    "device_capacity_bytes": 1000000000,
                    "device_peak_bytes": 100000000,
                    "frontier_peak_records": 1024,
                    "host_callbacks_per_pair": 0,
                    "host_capacity_bytes": 1000000000,
                    "host_peak_bytes": 100000000,
                    "node_visits": request["caps"]["node_visits"],
                    "output_pairs": n,
                    "radial_receipts": n * 3,
                }},
            }})
            """)


if __name__ == "__main__":
    unittest.main()
