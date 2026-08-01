#!/usr/bin/env python3
"""Fail-closed contract tests for the future complete true-HGP campaign."""

from __future__ import annotations

import copy
import hashlib
import importlib.util
import os
from pathlib import Path
import signal
import stat
import subprocess
import sys
import tempfile
import textwrap
import time
import unittest
from unittest import mock


DIRECTORY = Path(__file__).resolve().parent
HARNESS = DIRECTORY / "phase15_true_hgp_scale_campaign.py"
PLAN = DIRECTORY / "phase15_true_hgp_scale_campaign_v1.json"
if str(DIRECTORY) not in sys.path:
    sys.path.insert(0, str(DIRECTORY))

import campaign_runtime as runtime


SPEC = importlib.util.spec_from_file_location(
    "phase15_true_hgp_scale_campaign", HARNESS
)
assert SPEC is not None and SPEC.loader is not None
campaign = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(campaign)


BINARY_SHA256 = "b" * 64
PLAN_SHA256 = "c" * 64
CAPABILITIES_SHA256 = "d" * 64
GIT_SHA = "a" * 40


def make_complete_run(
    request: dict[str, object], *, warm_e2e_ns: int = 10_000_000
) -> dict[str, object]:
    run_id = request["run_id"]
    assert isinstance(run_id, str)
    return {
        "algorithm_scope": campaign.ALGORITHM_SCOPE,
        "artifacts": {
            "checkpoint_manifest_file": f"artifacts/{run_id}-checkpoint.json",
            "checkpoint_manifest_sha256": hashlib.sha256(
                f"checkpoint:{run_id}".encode("ascii")
            ).hexdigest(),
            "output_chain_sha256": hashlib.sha256(
                f"output:{run_id}".encode("ascii")
            ).hexdigest(),
            "source_manifest_file": f"artifacts/{run_id}-source.json",
            "source_manifest_sha256": hashlib.sha256(
                f"source:{run_id}".encode("ascii")
            ).hexdigest(),
        },
        "backend": campaign.BACKEND,
        "binary_sha256": BINARY_SHA256,
        "closure": {
            "active_cross_incidences_complete": True,
            "batches_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "canonical_children_complete": True,
            "checkpoint_closed": True,
            "condensed_view_validated": True,
            "coverage_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "exact_replay_closed": True,
            "forests_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "gamma_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "numeric_failure_count": 0,
            "output_chain_closed": True,
            "silent_q1_incidences_complete": True,
            "source_archive_closed": True,
            "unsupported_degeneracy_count": 0,
            "unresolved_locus_count": 0,
            "vertical_maps_complete": True,
        },
        "cloud": {
            "canonical_binary64_sha256": hashlib.sha256(
                f"cloud:{run_id}".encode("ascii")
            ).hexdigest(),
            "construction_count": 1,
            "fresh_for_run": True,
            "request_sha256": request["cloud_request_sha256"],
        },
        "counts_by_order": [
            {
                "cofaces": order * 13,
                "coverage_deltas": order * 5,
                "equal_level_batches": order,
                "facets": order * 17,
                "horizontal_nodes_invisible": order * 3,
                "horizontal_nodes_visible": order * 7,
                "incidences": order * 23,
                "order": order,
                "silent_q1_incidences": order * 2,
                "vertical_assignments": order * 11,
            }
            for order in range(1, campaign.MAXIMUM_ORDER + 1)
        ],
        "family": copy.deepcopy(request["family"]),
        "git_sha": GIT_SHA,
        "implementation": copy.deepcopy(campaign.IMPLEMENTATION_CONTRACT),
        "maximum_order": campaign.MAXIMUM_ORDER,
        "mode": campaign.MODE,
        "plan_sha256": PLAN_SHA256,
        "point_count": request["point_count"],
        "profile": campaign.PROFILE,
        "public_status": "not_claimed",
        "qualification_claimed": False,
        "request_sha256": campaign.request_sha256(request),
        "resources": {
            "checkpoint_bytes": 4096,
            "device_capacity_bytes": 24_000_000_000,
            "device_peak_bytes": 1_000_000_000,
            "dominant_phase": "source_construction",
            "external_run_bytes_read": 1024,
            "external_run_bytes_written": 2048,
            "host_capacity_bytes": 48_000_000_000,
            "host_peak_bytes": 2_000_000_000,
            "output_bytes": 8192,
            "scratch_peak_bytes": 65_536,
        },
        "role": request["role"],
        "run_id": run_id,
        "schema": campaign.RUN_SCHEMA,
        "seed": request["seed"],
        "status": "complete",
        "timings_ns": {
            "canonicalization": 100_000,
            "cloud_generation": 100_000,
            "condensation": 100_000,
            "horizontal_reduction": 100_000,
            "output_seal": 100_000,
            "source_construction": 100_000,
            "source_recertification": 100_000,
            "vertical_maps": 100_000,
            "warm_e2e": warm_e2e_ns,
        },
        "view": copy.deepcopy(request["view"]),
    }


def make_spool_identity() -> dict[str, str]:
    return runtime.campaign_identity(
        git_sha=GIT_SHA,
        binary_sha256=BINARY_SHA256,
        plan_sha256=PLAN_SHA256,
        capabilities_sha256=CAPABILITIES_SHA256,
    )


def spool_request(run_id: str = "run-0") -> dict[str, object]:
    return {"role": "measured", "run_id": run_id, "schema": "test.request.v1"}


def spool_receipt(run_id: str, request_sha256: str) -> dict[str, object]:
    return {
        "request_sha256": request_sha256,
        "role": "measured",
        "run_id": run_id,
        "status": "complete",
    }


def pid_is_running(pid: int) -> bool:
    try:
        fields = Path(f"/proc/{pid}/stat").read_text(encoding="ascii").split()
    except (FileNotFoundError, ProcessLookupError):
        return False
    return len(fields) >= 3 and fields[2] not in {"X", "Z"}


class TrueHgpPlanTests(unittest.TestCase):
    def setUp(self) -> None:
        self.plan, self.plan_digest = campaign.read_plan(PLAN)

    def test_gate_protocol_and_exact_scale_order_are_frozen(self) -> None:
        self.assertFalse(self.plan["entry_gate_satisfied"])
        self.assertEqual(self.plan, campaign.expected_plan_document())
        self.assertEqual(self.plan_digest, runtime.sha256_file(PLAN, "test plan"))

        requests = campaign.expected_50k_requests(self.plan)
        self.assertEqual(len(requests), 36)
        self.assertEqual([request["run_index"] for request in requests], list(range(36)))
        for family_id in campaign.FAMILY_IDS:
            family_requests = [
                request for request in requests if request["family"]["id"] == family_id
            ]
            self.assertEqual(
                [request["role"] for request in family_requests],
                ["warmup", "warmup", *("measured" for _ in range(10))],
            )
            self.assertTrue(all(request["fresh_cloud"] for request in family_requests))

        massive = campaign.expected_massive_requests(self.plan)
        self.assertEqual(
            [request["point_count"] for request in massive],
            [1_000_000, 10_000_001, 30_000_000],
        )
        self.assertTrue(all(request["resume_required"] for request in massive))
        self.assertEqual(
            [scale["required_previous_point_count"] for scale in self.plan["massive_scales"]],
            [50_000, 1_000_000, 10_000_001],
        )

        recovery = campaign.recovery_warmup_requests(
            self.plan, campaign.FAMILY_IDS[0], 2
        )
        self.assertEqual(len(recovery), 2)
        self.assertTrue(all(request["role"] == "recovery_warmup" for request in recovery))
        self.assertTrue(all(request["measured_index"] is None for request in recovery))

    def test_plan_mutation_and_all_existing_component_shapes_are_rejected(self) -> None:
        changed = copy.deepcopy(self.plan)
        changed["entry_gate_satisfied"] = True
        with self.assertRaisesRegex(campaign.ContractError, "differs from frozen"):
            campaign.validate_plan(changed)
        changed["entry_gate_satisfied"] = 0
        with self.assertRaisesRegex(campaign.ContractError, "differs from frozen"):
            campaign.validate_plan(changed)

        capabilities = campaign.expected_capabilities(
            plan=self.plan,
            plan_sha256=self.plan_digest,
            binary_sha256=BINARY_SHA256,
            binary_size_bytes=4096,
            git_sha=GIT_SHA,
        )
        self.assertEqual(
            campaign.validate_capabilities(
                capabilities,
                plan=self.plan,
                plan_sha256=self.plan_digest,
                binary_sha256=BINARY_SHA256,
                binary_size_bytes=4096,
                git_sha=GIT_SHA,
            ),
            capabilities,
        )
        for flag in ("component_only", "pair_catalog_only", "point_mst_surrogate"):
            hostile = copy.deepcopy(capabilities)
            hostile["capabilities"][flag] = True
            with self.subTest(flag=flag), self.assertRaisesRegex(
                campaign.ContractError, "not the complete"
            ):
                campaign.validate_capabilities(
                    hostile,
                    plan=self.plan,
                    plan_sha256=self.plan_digest,
                    binary_sha256=BINARY_SHA256,
                    binary_size_bytes=4096,
                    git_sha=GIT_SHA,
                )
        disguised_boolean = copy.deepcopy(capabilities)
        disguised_boolean["capabilities"]["component_only"] = 0
        with self.assertRaisesRegex(campaign.ContractError, "not the complete"):
            campaign.validate_capabilities(
                disguised_boolean,
                plan=self.plan,
                plan_sha256=self.plan_digest,
                binary_sha256=BINARY_SHA256,
                binary_size_bytes=4096,
                git_sha=GIT_SHA,
            )

        component_shape = {
            "schema": "morsehgp3d.phase15.component_binary_capabilities.v1",
            "component_only": True,
        }
        with self.assertRaisesRegex(campaign.ContractError, "not the complete"):
            campaign.validate_capabilities(
                component_shape,
                plan=self.plan,
                plan_sha256=self.plan_digest,
                binary_sha256=BINARY_SHA256,
                binary_size_bytes=4096,
                git_sha=GIT_SHA,
            )

    def test_closed_gate_precedes_binary_execution_and_spool_creation(self) -> None:
        with tempfile.TemporaryDirectory(prefix="true-hgp-closed-gate-") as raw:
            directory = Path(raw)
            marker = directory / "binary-was-run"
            binary = directory / "existing-component-binary"
            binary.write_text(
                f"#!/bin/sh\nprintf invoked > {marker}\n", encoding="ascii"
            )
            binary.chmod(binary.stat().st_mode | stat.S_IXUSR)
            spool_parent = directory / "spool"
            result = subprocess.run(
                [
                    sys.executable,
                    "-B",
                    str(HARNESS),
                    "--plan",
                    str(PLAN),
                    "run",
                    "--binary",
                    str(binary),
                    "--git-sha",
                    GIT_SHA,
                    "--spool-parent",
                    str(spool_parent),
                ],
                check=False,
                capture_output=True,
                text=True,
                timeout=5,
            )
            self.assertEqual(result.returncode, 2)
            self.assertIn("phase15_true_hgp_50k_entry_gate_satisfied=false", result.stderr)
            self.assertFalse(marker.exists())
            self.assertFalse(spool_parent.exists())


class TrueHgpRunContractTests(unittest.TestCase):
    def setUp(self) -> None:
        self.plan, _ = campaign.read_plan(PLAN)
        self.requests = campaign.expected_50k_requests(self.plan)

    def validate(self, run: dict[str, object], request: dict[str, object]) -> None:
        campaign.validate_run(
            run,
            request,
            binary_sha256=BINARY_SHA256,
            plan_sha256=PLAN_SHA256,
            git_sha=GIT_SHA,
        )

    def test_complete_source_receipt_passes_and_open_or_surrogate_receipts_fail(self) -> None:
        request = self.requests[2]
        valid = make_complete_run(request)
        self.validate(valid, request)

        mutations = {
            "silent incidence closure": lambda run: run["closure"].__setitem__(
                "silent_q1_incidences_complete", False
            ),
            "vertical closure": lambda run: run["closure"].__setitem__(
                "vertical_maps_complete", False
            ),
            "surrogate implementation": lambda run: run["implementation"].__setitem__(
                "point_mst_surrogate", True
            ),
            "missing order": lambda run: run["counts_by_order"].pop(),
            "integer closure mask": lambda run: run["closure"].__setitem__(
                "coverage_complete_by_order", [1] * campaign.MAXIMUM_ORDER
            ),
            "timed stages outside e2e": lambda run: run["timings_ns"].__setitem__(
                "source_construction", run["timings_ns"]["warm_e2e"]
            ),
            "artifact traversal": lambda run: run["artifacts"].__setitem__(
                "source_manifest_file", "../source.json"
            ),
            "public exact claim": lambda run: run.__setitem__("public_status", "exact"),
            "qualification claim": lambda run: run.__setitem__(
                "qualification_claimed", True
            ),
        }
        for label, mutate in mutations.items():
            hostile = copy.deepcopy(valid)
            mutate(hostile)
            with self.subTest(label=label), self.assertRaises(campaign.ContractError):
                self.validate(hostile, request)

    def test_nearest_rank_uses_only_thirty_fresh_measured_clouds(self) -> None:
        primary = []
        measured_index = 0
        for request in self.requests:
            if request["role"] == "measured":
                measured_index += 1
                run = make_complete_run(
                    request, warm_e2e_ns=measured_index * 1_000_000
                )
            else:
                run = make_complete_run(request)
            self.validate(run, request)
            primary.append(run)

        recovery_requests = campaign.recovery_warmup_requests(
            self.plan, campaign.FAMILY_IDS[0], 1
        )
        recovery = [make_complete_run(request) for request in recovery_requests]
        summary = campaign.build_50k_summary([*primary, *recovery], self.plan)
        self.assertEqual(campaign.nearest_rank(list(range(1, 31)), 95), 29)
        self.assertEqual(
            [item["p95_warm_e2e_ns"] for item in summary["family_summaries"]],
            [10_000_000, 20_000_000, 30_000_000],
        )
        self.assertEqual(summary["aggregate"]["p95_warm_e2e_ns"], 29_000_000)
        self.assertTrue(summary["gate_passed"])

        threshold = copy.deepcopy(primary)
        threshold[-1]["timings_ns"]["warm_e2e"] = campaign.SLO_P95_NS
        self.assertFalse(campaign.build_50k_summary(threshold, self.plan)["gate_passed"])

        reused = copy.deepcopy(primary)
        measured = [run for run in reused if run["role"] == "measured"]
        measured[1]["cloud"]["canonical_binary64_sha256"] = measured[0]["cloud"][
            "canonical_binary64_sha256"
        ]
        with self.assertRaisesRegex(campaign.ContractError, "reused one cloud"):
            campaign.build_50k_summary(reused, self.plan)

    def test_massive_progression_is_an_exact_contiguous_prefix(self) -> None:
        self.assertEqual(campaign.validate_scale_progression([]), 50_000)
        self.assertEqual(campaign.validate_scale_progression([50_000]), 1_000_000)
        self.assertEqual(
            campaign.validate_scale_progression([50_000, 1_000_000]),
            10_000_001,
        )
        self.assertIsNone(
            campaign.validate_scale_progression(
                [50_000, 1_000_000, 10_000_001, 30_000_000]
            )
        )
        for invalid in (
            [1_000_000],
            [50_000, 10_000_001],
            [50_000, 1_000_000, 30_000_000],
            [50_000, 1_000_000, 10_000_000],
        ):
            with self.subTest(invalid=invalid), self.assertRaisesRegex(
                campaign.ContractError, "contiguous registered prefix"
            ):
                campaign.validate_scale_progression(invalid)


class CampaignSpoolTests(unittest.TestCase):
    def test_checkpoint_receipt_chain_and_request_authority_survive_reopen(self) -> None:
        with tempfile.TemporaryDirectory(prefix="true-hgp-spool-") as raw:
            parent = Path(raw) / "spool"
            identity = make_spool_identity()
            request = spool_request()
            with runtime.CampaignSpool(parent, identity) as spool:
                request_file, request_digest = spool.publish_request(0, request)
                active = spool.commit_checkpoint(
                    ordinal=0,
                    run_id=request["run_id"],
                    request_file=request_file,
                    request_sha256=request_digest,
                    checkpoint={"cursor": 17, "schema": "test.checkpoint.v1"},
                )
                self.assertEqual(active["request_file"], request_file)
                root = spool.root

            with runtime.CampaignSpool(parent, identity) as resumed:
                self.assertEqual(resumed.next_ordinal, 0)
                self.assertEqual(resumed.head["active_run"]["request_file"], request_file)
                commit = resumed.commit_receipt(
                    ordinal=0,
                    request_file=request_file,
                    request_sha256=request_digest,
                    receipt=spool_receipt(request["run_id"], request_digest),
                )
                self.assertEqual(commit["request_file"], request_file)
                self.assertEqual(resumed.next_ordinal, 1)
                self.assertIsNone(resumed.head["active_run"])

            with runtime.CampaignSpool(parent, identity) as verified:
                self.assertEqual(verified.next_ordinal, 1)
                self.assertEqual(
                    verified.head["commits"][0]["request_file"], request_file
                )
                self.assertTrue((root / request_file).is_file())

    def test_reopen_rehashes_referenced_request_and_rejects_corruption(self) -> None:
        with tempfile.TemporaryDirectory(prefix="true-hgp-rehash-") as raw:
            parent = Path(raw) / "spool"
            identity = make_spool_identity()
            request = spool_request()
            with runtime.CampaignSpool(parent, identity) as spool:
                request_file, request_digest = spool.publish_request(0, request)
                spool.commit_receipt(
                    ordinal=0,
                    request_file=request_file,
                    request_sha256=request_digest,
                    receipt=spool_receipt(request["run_id"], request_digest),
                )
                root = spool.root
            runtime.atomic_publish(
                root / request_file,
                runtime.canonical_bytes({**request, "tampered": True}),
            )
            with self.assertRaisesRegex(runtime.RuntimeContractError, "digest differs"):
                with runtime.CampaignSpool(parent, identity):
                    pass

    def test_orphan_receipt_after_failpoint_does_not_advance_head(self) -> None:
        with tempfile.TemporaryDirectory(prefix="true-hgp-failpoint-") as raw:
            parent = Path(raw) / "spool"
            identity = make_spool_identity()
            request = spool_request()
            with runtime.CampaignSpool(parent, identity) as spool:
                request_file, request_digest = spool.publish_request(0, request)
                receipt = spool_receipt(request["run_id"], request_digest)
                with mock.patch.dict(
                    os.environ,
                    {runtime.FAILPOINT_ENVIRONMENT: "receipt_artifact"},
                    clear=False,
                ):
                    with self.assertRaisesRegex(
                        runtime.RuntimeContractError, "injected"
                    ):
                        spool.commit_receipt(
                            ordinal=0,
                            request_file=request_file,
                            request_sha256=request_digest,
                            receipt=receipt,
                        )
                self.assertEqual(spool.next_ordinal, 0)
                self.assertEqual(spool.head["commits"], [])

            with runtime.CampaignSpool(parent, identity) as resumed:
                self.assertEqual(resumed.next_ordinal, 0)
                resumed.commit_receipt(
                    ordinal=0,
                    request_file=request_file,
                    request_sha256=request_digest,
                    receipt=receipt,
                )
                self.assertEqual(resumed.next_ordinal, 1)

    def test_single_writer_symlink_traversal_and_noncanonical_json_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory(prefix="true-hgp-hostile-spool-") as raw:
            directory = Path(raw)
            parent = directory / "spool"
            identity = make_spool_identity()
            first = runtime.CampaignSpool(parent, identity)
            first.__enter__()
            root = first.root
            try:
                with self.assertRaisesRegex(
                    runtime.RuntimeContractError, "another writer"
                ):
                    with runtime.CampaignSpool(parent, identity):
                        pass
            finally:
                first.close()

            requests = root / "requests"
            requests.rename(root / "requests-real")
            requests.symlink_to(root / "requests-real", target_is_directory=True)
            with self.assertRaisesRegex(runtime.RuntimeContractError, "requests directory"):
                with runtime.CampaignSpool(parent, identity):
                    pass

            with self.assertRaisesRegex(runtime.RuntimeContractError, "escapes"):
                runtime.safe_relative_file("../outside.json", "hostile path")

            noncanonical = directory / "noncanonical.json"
            noncanonical.write_text('{\n  "a": 1\n}\n', encoding="ascii")
            with self.assertRaisesRegex(runtime.RuntimeContractError, "not canonical"):
                runtime.read_canonical_json(noncanonical, "noncanonical test")
            duplicate = '{"a":1,"a":2}\n'
            with self.assertRaisesRegex(runtime.RuntimeContractError, "duplicate"):
                runtime.parse_json_text(
                    duplicate, "duplicate test", canonical_line=False
                )
            with self.assertRaisesRegex(runtime.RuntimeContractError, "non-finite"):
                runtime.parse_json_text('{"a":NaN}\n', "NaN test", canonical_line=False)

            target = directory / "canonical.json"
            runtime.atomic_publish(target, runtime.canonical_bytes({"a": 1}))
            link = directory / "canonical-link.json"
            link.symlink_to(target)
            with self.assertRaisesRegex(runtime.RuntimeContractError, "regular file"):
                runtime.read_canonical_json(link, "symbolic JSON")


class ProcessGroupTests(unittest.TestCase):
    def test_timeout_kills_grandchild_even_after_parent_closes_group_pipes(self) -> None:
        with tempfile.TemporaryDirectory(prefix="true-hgp-process-group-") as raw:
            directory = Path(raw)
            pid_file = directory / "pids"
            program = directory / "forking_parent.py"
            program.write_text(
                textwrap.dedent(
                    """
                    import os
                    from pathlib import Path
                    import signal
                    import subprocess
                    import sys
                    import time

                    child_code = (
                        "import signal,time; "
                        "signal.signal(signal.SIGTERM, signal.SIG_IGN); "
                        "time.sleep(60)"
                    )
                    child = subprocess.Popen(
                        [sys.executable, "-c", child_code],
                        stdin=subprocess.DEVNULL,
                        stdout=subprocess.DEVNULL,
                        stderr=subprocess.DEVNULL,
                    )
                    Path(sys.argv[1]).write_text(
                        f"{os.getpid()} {child.pid}", encoding="ascii"
                    )
                    while True:
                        time.sleep(1)
                    """
                ).lstrip(),
                encoding="ascii",
            )
            result = runtime.run_process_group(
                [sys.executable, "-B", str(program), str(pid_file)],
                input_bytes=None,
                timeout_seconds=0.75,
                terminate_grace_seconds=0.15,
            )
            self.assertTrue(result.timed_out)
            self.assertTrue(result.term_sent)
            self.assertTrue(result.kill_sent)
            self.assertEqual(result.returncode, -signal.SIGTERM)
            parent_pid, child_pid = map(int, pid_file.read_text(encoding="ascii").split())
            deadline = time.monotonic() + 2.0
            while time.monotonic() < deadline and (
                pid_is_running(parent_pid) or pid_is_running(child_pid)
            ):
                time.sleep(0.02)
            self.assertFalse(pid_is_running(parent_pid))
            self.assertFalse(pid_is_running(child_pid))


if __name__ == "__main__":
    unittest.main(verbosity=2)
