#!/usr/bin/env python3
"""Focused fail-closed tests for the future true-HGP v4 contract."""

from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

DIRECTORY = Path(__file__).resolve().parent
HARNESS = DIRECTORY / "phase15_true_hgp_scale_campaign_v4.py"
PLAN = DIRECTORY / "phase15_true_hgp_scale_campaign_v4.json"
V2_HARNESS = DIRECTORY / "phase15_true_hgp_scale_campaign.py"
V1_PLAN = DIRECTORY / "phase15_true_hgp_scale_campaign_v1.json"
V2_PLAN = DIRECTORY / "phase15_true_hgp_scale_campaign_v2.json"
V2_TEST = DIRECTORY / "test_phase15_true_hgp_scale_campaign.py"
V3_HARNESS = DIRECTORY / "phase15_true_hgp_scale_campaign_v3.py"
V3_PLAN = DIRECTORY / "phase15_true_hgp_scale_campaign_v3.json"
V3_TEST = DIRECTORY / "test_phase15_true_hgp_scale_campaign_v3.py"
if str(DIRECTORY) not in sys.path:
    sys.path.insert(0, str(DIRECTORY))

import campaign_runtime as runtime

SPEC = importlib.util.spec_from_file_location(
    "phase15_true_hgp_scale_campaign_v4", HARNESS
)
assert SPEC is not None and SPEC.loader is not None
campaign = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(campaign)


BINARY_SHA256 = "b" * 64
PLAN_SHA256 = "c" * 64
GIT_SHA = "a" * 40
LEGACY_SHA256 = {
    V2_HARNESS: "fc2e21b4de22e0b1da7d59b372ebe5c0d61bf66c0642048b7dec21700c233177",
    V1_PLAN: "547f4d0ea7386600e4865d28db6f62a5a455e9dbcd19f9893629961c0f3dddb6",
    V2_PLAN: "255f7d9d1ff6f80255342f8ecbca6603cd2d0da4d0768f0dd55e047e122d6b28",
    V2_TEST: "c9f2d3c01dc72d3853d2a966261a356ca987558049f41068bdb9f70ed1641253",
    V3_HARNESS: "047294625c5cf1bc5782ac7d1f6022de4c909331389fb960d864b67aa827959c",
    V3_PLAN: "342864a71e9f47d0f4e06d054ca22620fe84173aa65f6fa7e05a252e6219c377",
    V3_TEST: "0cc095e8ec1d15763106c74a62c514556b903ae5a490b64ced806aa51294aee6",
}


def digest(label: str) -> str:
    return hashlib.sha256(label.encode("ascii")).hexdigest()


def artifact_payloads(run_id: str) -> dict[str, bytes]:
    return {
        "checkpoint": (f"checkpoint-manifest-v4:{run_id}\n").encode("ascii"),
        "normalized_source": (f"normalized-source-manifest-v4:{run_id}\n").encode(
            "ascii"
        ),
        "hartigan": (f"hartigan-rational-level-manifest-v4:{run_id}\n").encode(
            "ascii"
        ),
    }


def make_complete_run(
    request: dict[str, object], *, warm_e2e_ns: int = 10_000_000
) -> dict[str, object]:
    run_id = request["run_id"]
    assert isinstance(run_id, str)
    payloads = artifact_payloads(run_id)
    counts = []
    hartigan_orders = []
    manifest_record_count = 0
    for order in range(1, campaign.MAXIMUM_ORDER + 1):
        materialized = 2
        omitted_noop = 0 if order == 1 else 1
        normalized = materialized + omitted_noop
        qr0_groups = order
        qr1_groups = omitted_noop + 2
        qr_greater_equal2_groups = order + 1
        group_count = qr0_groups + qr1_groups + qr_greater_equal2_groups
        counts.append(
            {
                "co_minimizing_first_incidences": order * 2,
                "complete_carriers": order * 5,
                "direct_events": order * 3,
                "horizontal_nodes_invisible": order,
                "horizontal_nodes_visible": order * 2,
                "materialized_equal_level_lots": materialized,
                "normalized_equal_level_lots": normalized,
                "normalized_omitted_noop_qr1_lots": omitted_noop,
                "normalized_qr0_groups": qr0_groups,
                "normalized_qr1_groups": qr1_groups,
                "normalized_qr_greater_equal2_groups": (qr_greater_equal2_groups),
                "order": order,
                "rank_window_saturated_blocks": order,
                "resident_groups": group_count,
                "vertical_assignments": order * 4,
            }
        )
        hartigan_orders.append(
            {
                "first_level": {
                    "denominator": "1",
                    "numerator": str(order),
                },
                "last_level": {
                    "denominator": "1",
                    "numerator": str(order + normalized),
                },
                "invalid_omitted_noop_lot_count": 0,
                "invalid_qr_partition_lot_count": 0,
                "normalized_equal_level_lot_count": normalized,
                "normalized_group_count": group_count,
                "normalized_omitted_noop_qr1_lot_count": omitted_noop,
                "normalized_qr0_group_count": qr0_groups,
                "normalized_qr1_group_count": qr1_groups,
                "normalized_qr_greater_equal2_group_count": (qr_greater_equal2_groups),
                "order": order,
                "partitioned_lot_count": normalized,
                "rational_record_count": normalized,
            }
        )
        manifest_record_count += normalized

    resume_required = request["resume_required"]
    assert isinstance(resume_required, bool)
    return {
        "algorithm_scope": campaign.ALGORITHM_SCOPE,
        "artifacts": {
            "checkpoint_manifest_file": f"artifacts/{run_id}-checkpoint.json",
            "checkpoint_manifest_sha256": hashlib.sha256(
                payloads["checkpoint"]
            ).hexdigest(),
            "checkpoint_manifest_size_bytes": len(payloads["checkpoint"]),
            "normalized_source_manifest_file": (
                f"artifacts/{run_id}-normalized-source.json"
            ),
            "normalized_source_manifest_sha256": hashlib.sha256(
                payloads["normalized_source"]
            ).hexdigest(),
            "normalized_source_manifest_size_bytes": len(
                payloads["normalized_source"]
            ),
            "output_chain_sha256": digest(f"output:{run_id}"),
        },
        "backend": campaign.BACKEND,
        "binary_sha256": BINARY_SHA256,
        "closure": {
            "all_co_minimizing_first_incidences_complete_by_order": (
                [True] * campaign.MAXIMUM_ORDER
            ),
            "at_least20_condensed_view_complete": True,
            "carriers_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "checkpoint_closed": True,
            "direct_events_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "exact_replay_closed": True,
            "hartigan_qr_partition_complete_by_order": (
                [True] * campaign.MAXIMUM_ORDER
            ),
            "hartigan_levels_complete": True,
            "horizontal_forests_complete_by_order": ([True] * campaign.MAXIMUM_ORDER),
            "incidence_complete_reduction_proved": True,
            "k1_closed_cut_complete": True,
            "lots_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "normalized_resident_source_v7_complete": True,
            "normalized_noop_qr1_complete_by_order": ([True] * campaign.MAXIMUM_ORDER),
            "numeric_failure_count": 0,
            "output_chain_closed": True,
            "rank_window_saturation_complete_by_order": (
                [True] * campaign.MAXIMUM_ORDER
            ),
            "source_archive_closed": True,
            "unsupported_degeneracy_count": 0,
            "unresolved_locus_count": 0,
            "vertical_maps_complete_between_adjacent_orders": (
                [True] * (campaign.MAXIMUM_ORDER - 1)
            ),
        },
        "cloud": {
            "canonical_binary64_sha256": digest(f"cloud:{run_id}"),
            "construction_count": 1,
            "fresh_for_run": True,
            "request_sha256": request["cloud_request_sha256"],
        },
        "counts_by_order": counts,
        "family": copy.deepcopy(request["family"]),
        "forest": {
            "at_least20_view_chain_sha256": digest(f"view:{run_id}"),
            "at_least20_view_complete": True,
            "horizontal_chain_sha256": digest(f"horizontal:{run_id}"),
            "horizontal_forests_complete_by_order": ([True] * campaign.MAXIMUM_ORDER),
            "k1_closed_cut_chain_sha256": digest(f"k1:{run_id}"),
            "k1_closed_cut_complete": True,
            "vertical_chain_sha256": digest(f"vertical:{run_id}"),
            "vertical_map_contract": copy.deepcopy(campaign.VERTICAL_MAP_CONTRACT),
            "vertical_maps_complete_between_adjacent_orders": (
                [True] * (campaign.MAXIMUM_ORDER - 1)
            ),
        },
        "git_sha": GIT_SHA,
        "hartigan_levels": {
            "all_normalized_equal_level_batches_covered": True,
            "binary64_level_count": 0,
            "every_record_uses_complete_batch_qr_partition": True,
            "legacy_single_group_scalar_qr_record_count": 0,
            "manifest_file": f"artifacts/{run_id}-hartigan-v4.jsonl",
            "manifest_record_count": manifest_record_count,
            "manifest_sha256": hashlib.sha256(payloads["hartigan"]).hexdigest(),
            "manifest_size_bytes": len(payloads["hartigan"]),
            "ordered_rational_chain_sha256": digest(f"hartigan-chain:{run_id}"),
            "records_by_order": hartigan_orders,
            "representation": campaign.HARTIGAN_LEVEL_REPRESENTATION,
            "schema": campaign.HARTIGAN_LEVEL_RECEIPT_SCHEMA,
        },
        "maximum_order": campaign.MAXIMUM_ORDER,
        "mode": campaign.MODE,
        "normalization": {
            "complete_qr_partition_lot_count": manifest_record_count,
            "contract": copy.deepcopy(campaign.NORMALIZED_H0_CONTRACT),
            "contract_v2_identity_compatible": False,
            "invalid_omitted_noop_lot_count": 0,
            "invalid_qr_partition_lot_count": 0,
            "legacy_single_group_scalar_qr_count": 0,
            "normalized_batch_chain_closed": True,
            "normalized_batch_chain_sha256": digest(f"normalized:{run_id}"),
            "non_qr1_noop_count": 0,
            "non_single_group_noop_count": 0,
            "nonzero_core_facet_delta_noop_count": 0,
            "nonzero_parent_or_node_delta_noop_count": 0,
            "nonzero_point_delta_noop_count": 0,
            "physical_v2_identity_reuse_count": 0,
        },
        "plan_sha256": PLAN_SHA256,
        "point_count": request["point_count"],
        "profile": campaign.PROFILE,
        "public_status": "not_claimed",
        "qualification_claimed": False,
        "request_sha256": campaign.request_sha256(request),
        "resources": {
            "checkpoint_bytes": 4_096,
            "device_capacity_bytes": 24_000_000_000,
            "device_peak_bytes": 1_000_000_000,
            "host_capacity_bytes": 48_000_000_000,
            "host_peak_bytes": 2_000_000_000,
            "output_bytes": 8_192,
            "scratch_peak_bytes": 65_536,
        },
        "resume": {
            "all_adjacent_vertical_map_digest_equivalent": resume_required,
            "at_least20_view_digest_equivalent": resume_required,
            "forced_checkpoint_count": 1 if resume_required else 0,
            "fresh_recertification_complete": resume_required,
            "fresh_replay_complete": resume_required,
            "fresh_process_resume_count": 1 if resume_required else 0,
            "hartigan_manifest_digest_equivalent": resume_required,
            "horizontal_forest_digest_equivalent": resume_required,
            "k1_closed_cut_digest_equivalent": resume_required,
            "normalized_resident_v7_digest_equivalent": resume_required,
            "required": resume_required,
            "status": "complete" if resume_required else "not_required",
        },
        "role": request["role"],
        "run_id": run_id,
        "schema": campaign.RUN_SCHEMA,
        "seed": request["seed"],
        "source": {
            "carrier_chain_sha256": digest(f"carrier:{run_id}"),
            "contract": copy.deepcopy(campaign.SOURCE_REDUCTION_CONTRACT),
            "direct_event_chain_sha256": digest(f"direct:{run_id}"),
            "first_incidence_chain_sha256": digest(f"first-incidence:{run_id}"),
            "incidence_reduction_proof_receipt_sha256": digest(
                "incidence-complete-reduction-proof"
            ),
            "normalized_resident_v7_chain_sha256": digest(f"resident-v7:{run_id}"),
            "proof_basis": (
                "horizontal_incidence_complete_rank_window_saturation_proved"
            ),
            "rank_window_saturation_chain_sha256": digest(f"rank-window:{run_id}"),
            "source_schema_version": 7,
        },
        "status": "complete",
        "timings_ns": {
            "canonicalization": 100_000,
            "cloud_generation": 100_000,
            "condensation": 100_000,
            "hartigan_manifest": 100_000,
            "horizontal_reduction": 100_000,
            "k1_closed_cut": 100_000,
            "output_seal": 100_000,
            "source_construction": 100_000,
            "source_recertification": 100_000,
            "vertical_maps": 100_000,
            "warm_e2e": warm_e2e_ns,
        },
        "view": copy.deepcopy(request["view"]),
    }


def install_fake_binary(
    directory: Path,
    *,
    plan_path: Path = PLAN,
    failed_session_numbers: tuple[int, ...] = (),
    timeout_session_numbers: tuple[int, ...] = (),
    invalid_session_numbers: tuple[int, ...] = (),
    missing_artifact_session_numbers: tuple[int, ...] = (),
    symlink_artifact_session_numbers: tuple[int, ...] = (),
    wrong_size_artifact_session_numbers: tuple[int, ...] = (),
    wrong_digest_artifact_session_numbers: tuple[int, ...] = (),
) -> tuple[Path, Path, Path]:
    """Install a protocol-faithful fake whose own digest is self-reported."""

    binary = directory / "fake-true-hgp-v4.py"
    state = directory / "fake-state.json"
    log = directory / "fake-session-log.jsonl"
    source = f"""#!/usr/bin/env python3
import json
from pathlib import Path
import sys
import time

DIRECTORY = Path({str(DIRECTORY)!r})
PLAN = Path({str(plan_path)!r})
STATE = Path({str(state)!r})
LOG = Path({str(log)!r})
GIT_SHA = {GIT_SHA!r}
TIMEOUT_SESSION_NUMBERS = {list(timeout_session_numbers)!r}
FAILED_SESSION_NUMBERS = {list(failed_session_numbers)!r}
INVALID_SESSION_NUMBERS = {list(invalid_session_numbers)!r}
MISSING_ARTIFACT_SESSION_NUMBERS = {list(missing_artifact_session_numbers)!r}
SYMLINK_ARTIFACT_SESSION_NUMBERS = {list(symlink_artifact_session_numbers)!r}
WRONG_SIZE_ARTIFACT_SESSION_NUMBERS = {list(wrong_size_artifact_session_numbers)!r}
WRONG_DIGEST_ARTIFACT_SESSION_NUMBERS = {list(wrong_digest_artifact_session_numbers)!r}
sys.path.insert(0, str(DIRECTORY))

import campaign_runtime as runtime
import phase15_true_hgp_scale_campaign_v4 as campaign
import test_phase15_true_hgp_scale_campaign_v4 as fixture

binary = Path(__file__)
binary_sha256 = runtime.sha256_file(binary, "fake binary")
plan, plan_sha256 = campaign.read_plan(PLAN)
capabilities = campaign.expected_capabilities(
    plan=plan,
    plan_sha256=plan_sha256,
    binary_sha256=binary_sha256,
    binary_size_bytes=binary.stat().st_size,
    git_sha=GIT_SHA,
)

if sys.argv[1:] == [campaign.CAPABILITY_ARGUMENT]:
    print(runtime.canonical_json(capabilities))
    raise SystemExit(0)
if sys.argv[1:] != [campaign.SESSION_ARGUMENT]:
    raise SystemExit(64)

raw = sys.stdin.buffer.read().decode("ascii")
session = runtime.parse_json_text(raw, "fake v4 session", canonical_line=True)
if set(session) != {{
    "active_checkpoint", "filesystem", "identity", "ordinal", "request_chain_sha256",
    "requests", "schema"
}} or session["schema"] != campaign.SESSION_REQUEST_SCHEMA:
    raise RuntimeError("fake received an invalid session wrapper")
if not isinstance(session["requests"], list) or len(session["requests"]) != 1:
    raise RuntimeError("fake session is not mono-request")
request = session["requests"][0]
if session["request_chain_sha256"] != runtime.sha256_bytes(
    runtime.canonical_bytes(request)
):
    raise RuntimeError("fake request chain differs")
expected_identity = runtime.campaign_identity(
    git_sha=GIT_SHA,
    binary_sha256=binary_sha256,
    plan_sha256=plan_sha256,
    capabilities_sha256=runtime.sha256_bytes(runtime.canonical_bytes(capabilities)),
)
if session["identity"] != expected_identity:
    raise RuntimeError("fake campaign identity differs")
artifact_root = Path(session["filesystem"]["artifact_staging_root"])
spool_root = Path(session["filesystem"]["spool_root"])
checkpoint = Path(session["active_checkpoint"]["checkpoint_file"])
if (
    not artifact_root.is_absolute()
    or artifact_root.resolve(strict=True) != artifact_root
    or Path.cwd() != artifact_root
    or not spool_root.is_absolute()
    or spool_root.resolve(strict=True) != spool_root
    or artifact_root.parent != spool_root / "attempts"
    or checkpoint.parent != spool_root / "checkpoints"
    or runtime.sha256_file(checkpoint, "fake active checkpoint")
    != session["active_checkpoint"]["checkpoint_sha256"]
):
    raise RuntimeError("fake received a non-physical filesystem authority")

with LOG.open("ab") as stream:
    stream.write(raw.encode("ascii"))
if STATE.exists():
    state = json.loads(STATE.read_text(encoding="ascii"))
else:
    state = {{"session_count": 0}}
session_number = state["session_count"]
state["session_count"] = session_number + 1
STATE.write_text(runtime.canonical_json(state) + "\\n", encoding="ascii")
if session_number in TIMEOUT_SESSION_NUMBERS:
    time.sleep(2.0)
if session_number in FAILED_SESSION_NUMBERS:
    raise SystemExit(75)

fixture.BINARY_SHA256 = binary_sha256
fixture.PLAN_SHA256 = plan_sha256
fixture.GIT_SHA = GIT_SHA
run = fixture.make_complete_run(request)
payloads = fixture.artifact_payloads(request["run_id"])
artifact_files = [
    (run["artifacts"]["checkpoint_manifest_file"], payloads["checkpoint"]),
    (
        run["artifacts"]["normalized_source_manifest_file"],
        payloads["normalized_source"],
    ),
    (run["hartigan_levels"]["manifest_file"], payloads["hartigan"]),
]
for relative, payload in artifact_files:
    path = artifact_root / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)
fault_path = artifact_root / artifact_files[0][0]
if session_number in MISSING_ARTIFACT_SESSION_NUMBERS:
    fault_path.unlink()
if session_number in SYMLINK_ARTIFACT_SESSION_NUMBERS:
    fault_path.unlink()
    fault_path.symlink_to(artifact_root / artifact_files[1][0])
if session_number in WRONG_SIZE_ARTIFACT_SESSION_NUMBERS:
    fault_path.write_bytes(payloads["checkpoint"] + b"x")
if session_number in WRONG_DIGEST_ARTIFACT_SESSION_NUMBERS:
    fault_path.write_bytes(b"X" + payloads["checkpoint"][1:])
if session_number in INVALID_SESSION_NUMBERS:
    run["public_status"] = "exact"
print(runtime.canonical_json(run))
"""
    binary.write_text(source, encoding="ascii")
    binary.chmod(0o700)
    return binary, state, log


class TrueHgpV4PlanTests(unittest.TestCase):
    def setUp(self) -> None:
        self.plan, self.plan_digest = campaign.read_plan(PLAN)

    def test_v4_gate_identity_source_scales_and_secondary_slo_are_frozen(self) -> None:
        self.assertEqual(self.plan, campaign.expected_plan_document())
        self.assertFalse(self.plan["entry_gate_satisfied"])
        self.assertFalse(
            self.plan["normalized_h0_contract"]["contract_v2_identity_compatible"]
        )
        self.assertTrue(
            self.plan["source_reduction_contract"][
                "incidence_complete_reduction_proved"
            ]
        )
        self.assertEqual(
            self.plan["source_reduction_contract"]["source_kind"],
            "normalized_resident_v7_direct_plus_all_co_minimizing_first_"
            "incidences_plus_rank_window_saturation",
        )
        self.assertFalse(
            self.plan["source_reduction_contract"]["global_regularity_required"]
        )
        self.assertTrue(
            self.plan["source_reduction_contract"][
                "irregular_above_rank_window_h0_saturated_blocks_allowed"
            ]
        )
        self.assertEqual(
            self.plan["source_reduction_contract"][
                "normalized_resident_source_schema_version"
            ],
            7,
        )
        self.assertEqual(
            self.plan["vertical_map_contract"]["adjacent_order_pairs"],
            [[order, order - 1] for order in range(2, 11)],
        )
        self.assertEqual(
            self.plan["fifty_k_protocol"]["slo_p95_warm_e2e_ns"],
            1_000_000_000,
        )
        self.assertEqual(
            self.plan["fifty_k_protocol"]["slo_role"],
            "secondary_progression_gate",
        )
        self.assertEqual(
            [scale["point_count"] for scale in self.plan["massive_scales"]],
            [1_000_000, 10_000_001, 30_000_000],
        )
        self.assertEqual(
            [
                scale["required_previous_point_count"]
                for scale in self.plan["massive_scales"]
            ],
            [50_000, 1_000_000, 10_000_001],
        )
        self.assertEqual(self.plan["view"]["relation"], "at_least")
        self.assertEqual(self.plan["view"]["min_cluster_size"], 20)

        changed = copy.deepcopy(self.plan)
        changed["entry_gate_satisfied"] = True
        with self.assertRaisesRegex(campaign.ContractError, "frozen v4"):
            campaign.validate_plan(changed)
        opened = copy.deepcopy(changed)
        opened["execution_status"] = "ready_for_true_hgp_v4_product_execution"
        self.assertEqual(campaign.validate_plan(opened), opened)
        for legacy in (V1_PLAN, V2_PLAN, V3_PLAN):
            with self.subTest(legacy=legacy.name), self.assertRaisesRegex(
                campaign.ContractError, "frozen v4"
            ):
                campaign.validate_plan(json.loads(legacy.read_text(encoding="ascii")))

    def test_v1_v2_v3_harness_plans_and_tests_remain_byte_identical(self) -> None:
        for path, expected_digest in LEGACY_SHA256.items():
            with self.subTest(path=path.name):
                self.assertEqual(runtime.sha256_file(path, path.name), expected_digest)

    def test_requests_capabilities_and_closed_gate_are_fail_closed(self) -> None:
        requests = campaign.expected_50k_requests(self.plan)
        self.assertEqual(len(requests), 36)
        self.assertEqual(
            [request["run_index"] for request in requests], list(range(36))
        )
        self.assertTrue(
            all(
                request["source_reduction_contract"]
                == campaign.SOURCE_REDUCTION_CONTRACT
                and request["normalized_h0_contract"] == campaign.NORMALIZED_H0_CONTRACT
                and request["vertical_map_contract"] == campaign.VERTICAL_MAP_CONTRACT
                for request in requests
            )
        )
        massive = campaign.expected_massive_requests(self.plan)
        self.assertEqual(
            [request["point_count"] for request in massive],
            [1_000_000, 10_000_001, 30_000_000],
        )
        self.assertTrue(all(request["resume_required"] for request in massive))

        capabilities = campaign.expected_capabilities(
            plan=self.plan,
            plan_sha256=self.plan_digest,
            binary_sha256=BINARY_SHA256,
            binary_size_bytes=4_096,
            git_sha=GIT_SHA,
        )
        campaign.validate_capabilities(
            capabilities,
            plan=self.plan,
            plan_sha256=self.plan_digest,
            binary_sha256=BINARY_SHA256,
            binary_size_bytes=4_096,
            git_sha=GIT_SHA,
        )
        for flag in (
            "incidence_complete_reduction_proved",
            "true_hgp_v4_product_coordinator_complete",
            "runtime_session_protocol_complete",
        ):
            hostile = copy.deepcopy(capabilities)
            hostile["capabilities"][flag] = False
            with self.subTest(flag=flag), self.assertRaisesRegex(
                campaign.ContractError, "does not satisfy"
            ):
                campaign.validate_capabilities(
                    hostile,
                    plan=self.plan,
                    plan_sha256=self.plan_digest,
                    binary_sha256=BINARY_SHA256,
                    binary_size_bytes=4_096,
                    git_sha=GIT_SHA,
                )

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            spool = root / "must-not-exist"
            result = subprocess.run(
                [
                    sys.executable,
                    "-B",
                    str(HARNESS),
                    "--plan",
                    str(PLAN),
                    "run",
                    "--binary",
                    str(root / "missing-binary"),
                    "--git-sha",
                    "not-a-git-sha",
                    "--spool-parent",
                    str(spool),
                ],
                check=False,
                capture_output=True,
                text=True,
                timeout=5,
            )
            self.assertEqual(result.returncode, 2)
            self.assertIn(
                "phase15_true_hgp_v4_entry_gate_satisfied=false", result.stderr
            )
            self.assertNotIn("binary", result.stderr.split("false", maxsplit=1)[-1])
            self.assertFalse(spool.exists())


class TrueHgpV4ExecutionTests(unittest.TestCase):
    def setUp(self) -> None:
        self.closed_plan, self.plan_digest = campaign.read_plan(PLAN)

    def publish_open_plan(self, root: Path) -> tuple[dict[str, object], Path, str]:
        open_plan = copy.deepcopy(self.closed_plan)
        open_plan["entry_gate_satisfied"] = True
        open_plan["execution_status"] = "ready_for_true_hgp_v4_product_execution"
        path = root / "phase15-true-hgp-v4-open.json"
        path.write_text(runtime.canonical_json(open_plan) + "\n", encoding="ascii")
        loaded, digest_value = campaign.read_plan(path)
        return loaded, path, digest_value

    def execute_open(
        self,
        *,
        plan: dict[str, object],
        plan_path: Path,
        plan_digest: str,
        binary: Path,
        spool_parent: Path,
    ) -> None:
        campaign.execute_campaign(
            plan=plan,
            plan_path=plan_path,
            plan_sha256=plan_digest,
            binary=binary,
            git_sha=GIT_SHA,
            spool_parent=spool_parent,
        )

    def read_only_head(self, spool_parent: Path) -> dict[str, object]:
        roots = list(spool_parent.glob("campaign-*"))
        self.assertEqual(len(roots), 1)
        head, _ = runtime.read_canonical_json(roots[0] / "HEAD.json", "test HEAD")
        return head

    def test_fake_binary_runs_contiguous_50k_gate_then_registered_massive_scales(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            open_plan, open_plan_path, open_plan_digest = self.publish_open_plan(root)
            binary, state, log = install_fake_binary(root, plan_path=open_plan_path)
            spool_parent = root / "spool"

            self.execute_open(
                plan=open_plan,
                plan_path=open_plan_path,
                plan_digest=open_plan_digest,
                binary=binary,
                spool_parent=spool_parent,
            )
            head = self.read_only_head(spool_parent)
            self.assertEqual(head["next_ordinal"], 39)
            self.assertIsNone(head["active_run"])
            self.assertEqual(
                json.loads(state.read_text(encoding="ascii"))["session_count"], 39
            )
            sessions = [
                runtime.parse_json_text(line, "fake session log", canonical_line=True)
                for line in log.read_text(encoding="ascii").splitlines(keepends=True)
            ]
            self.assertEqual(
                [session["requests"][0]["point_count"] for session in sessions],
                [campaign.FIFTY_K_POINT_COUNT] * 36
                + list(campaign.MASSIVE_POINT_COUNTS),
            )
            self.assertTrue(
                all(
                    session["schema"] == campaign.SESSION_REQUEST_SCHEMA
                    and session["ordinal"] == ordinal
                    and Path(session["filesystem"]["spool_root"]).is_absolute()
                    and Path(
                        session["filesystem"]["artifact_staging_root"]
                    ).is_absolute()
                    and Path(
                        session["active_checkpoint"]["checkpoint_file"]
                    ).is_absolute()
                    for ordinal, session in enumerate(sessions)
                )
            )
            campaign_root = next(spool_parent.glob("campaign-*"))
            self.assertEqual(len(list((campaign_root / "artifacts").iterdir())), 117)
            for commit in head["commits"]:
                receipt, _ = runtime.read_canonical_json(
                    campaign_root / commit["receipt_file"], "test receipt"
                )
                declarations = (
                    (
                        receipt["artifacts"],
                        "checkpoint_manifest_file",
                        "checkpoint_manifest_size_bytes",
                        "checkpoint_manifest_sha256",
                    ),
                    (
                        receipt["artifacts"],
                        "normalized_source_manifest_file",
                        "normalized_source_manifest_size_bytes",
                        "normalized_source_manifest_sha256",
                    ),
                    (
                        receipt["hartigan_levels"],
                        "manifest_file",
                        "manifest_size_bytes",
                        "manifest_sha256",
                    ),
                )
                for container, file_key, size_key, digest_key in declarations:
                    artifact = campaign_root / container[file_key]
                    self.assertTrue(artifact.is_file())
                    self.assertFalse(artifact.is_symlink())
                    self.assertEqual(artifact.stat().st_size, container[size_key])
                    self.assertEqual(
                        runtime.sha256_file(artifact, "test archived artifact"),
                        container[digest_key],
                    )

            # A complete spool is a no-op after the mandatory fresh handshake.
            self.execute_open(
                plan=open_plan,
                plan_path=open_plan_path,
                plan_digest=open_plan_digest,
                binary=binary,
                spool_parent=spool_parent,
            )
            self.assertEqual(
                json.loads(state.read_text(encoding="ascii"))["session_count"], 39
            )

    def test_interruption_resumes_same_checkpoint_and_invalid_receipt_is_not_committed(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            open_plan, open_plan_path, open_plan_digest = self.publish_open_plan(root)
            binary, _, log = install_fake_binary(
                root,
                plan_path=open_plan_path,
                failed_session_numbers=(0,),
                invalid_session_numbers=(2,),
            )
            spool_parent = root / "spool"

            with self.assertRaisesRegex(
                runtime.RuntimeContractError, "failed with status 75"
            ):
                self.execute_open(
                    plan=open_plan,
                    plan_path=open_plan_path,
                    plan_digest=open_plan_digest,
                    binary=binary,
                    spool_parent=spool_parent,
                )
            timed_out_head = self.read_only_head(spool_parent)
            self.assertEqual(timed_out_head["next_ordinal"], 0)
            self.assertEqual(timed_out_head["active_run"]["ordinal"], 0)

            with self.assertRaisesRegex(campaign.ContractError, "public exactness"):
                self.execute_open(
                    plan=open_plan,
                    plan_path=open_plan_path,
                    plan_digest=open_plan_digest,
                    binary=binary,
                    spool_parent=spool_parent,
                )
            invalid_head = self.read_only_head(spool_parent)
            self.assertEqual(invalid_head["next_ordinal"], 1)
            self.assertEqual(invalid_head["active_run"]["ordinal"], 1)
            sessions = [
                runtime.parse_json_text(line, "fake session log", canonical_line=True)
                for line in log.read_text(encoding="ascii").splitlines(keepends=True)
            ]
            self.assertEqual([session["ordinal"] for session in sessions], [0, 0, 1])
            self.assertEqual(sessions[0], sessions[1])

    def test_missing_symlink_wrong_size_and_wrong_digest_artifacts_never_commit(
        self,
    ) -> None:
        faults = {
            "missing": {"missing_artifact_session_numbers": (0,)},
            "symlink": {"symlink_artifact_session_numbers": (0,)},
            "wrong size": {"wrong_size_artifact_session_numbers": (0,)},
            "wrong digest": {"wrong_digest_artifact_session_numbers": (0,)},
        }
        for label, options in faults.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                open_plan, open_plan_path, open_plan_digest = self.publish_open_plan(root)
                binary, _, _ = install_fake_binary(
                    root, plan_path=open_plan_path, **options
                )
                spool_parent = root / "spool"
                with self.assertRaisesRegex(
                    runtime.RuntimeContractError,
                    "checkpoint-manifest",
                ):
                    self.execute_open(
                        plan=open_plan,
                        plan_path=open_plan_path,
                        plan_digest=open_plan_digest,
                        binary=binary,
                        spool_parent=spool_parent,
                    )
                head = self.read_only_head(spool_parent)
                self.assertEqual(head["next_ordinal"], 0)
                self.assertEqual(head["active_run"]["ordinal"], 0)
                campaign_root = next(spool_parent.glob("campaign-*"))
                self.assertEqual(list((campaign_root / "artifacts").iterdir()), [])


class TrueHgpV4RunTests(unittest.TestCase):
    def setUp(self) -> None:
        self.plan, _ = campaign.read_plan(PLAN)
        self.request = campaign.expected_50k_requests(self.plan)[2]

    def validate(self, run: dict[str, object], request: dict[str, object]) -> None:
        campaign.validate_run(
            run,
            request,
            binary_sha256=BINARY_SHA256,
            plan_sha256=PLAN_SHA256,
            git_sha=GIT_SHA,
        )

    def test_complete_normalized_forest_passes_and_scientific_mutations_fail(
        self,
    ) -> None:
        valid = make_complete_run(self.request)
        self.validate(valid, self.request)
        mutations = {
            "missing incidence proof": lambda run: run["closure"].__setitem__(
                "incidence_complete_reduction_proved", False
            ),
            "incomplete carrier order": lambda run: run["closure"][
                "carriers_complete_by_order"
            ].__setitem__(4, False),
            "incomplete first incidence order": lambda run: run["closure"][
                "all_co_minimizing_first_incidences_complete_by_order"
            ].__setitem__(4, False),
            "incomplete rank window order": lambda run: run["closure"][
                "rank_window_saturation_complete_by_order"
            ].__setitem__(4, False),
            "wrong resident source version": lambda run: run["source"].__setitem__(
                "source_schema_version", 6
            ),
            "global regularity requirement": lambda run: run["source"][
                "contract"
            ].__setitem__("global_regularity_required", True),
            "global Gamma": lambda run: run["source"]["contract"].__setitem__(
                "global_gamma_materialized", True
            ),
            "v2 identity": lambda run: run["normalization"].__setitem__(
                "contract_v2_identity_compatible", True
            ),
            "nonzero point no-op delta": lambda run: run["normalization"].__setitem__(
                "nonzero_point_delta_noop_count", 1
            ),
            "nonzero core-facet no-op delta": lambda run: run[
                "normalization"
            ].__setitem__("nonzero_core_facet_delta_noop_count", 1),
            "nonzero parent no-op delta": lambda run: run["normalization"].__setitem__(
                "nonzero_parent_or_node_delta_noop_count", 1
            ),
            "lot normalization mismatch": lambda run: run["counts_by_order"][
                1
            ].__setitem__("normalized_equal_level_lots", 9),
            "incomplete qR partition": lambda run: run["hartigan_levels"][
                "records_by_order"
            ][1].__setitem__("normalized_qr0_group_count", 99),
            "invalid omitted multi-group no-op": lambda run: run[
                "normalization"
            ].__setitem__("non_single_group_noop_count", 1),
            "invalid omitted non-qR1 no-op": lambda run: run[
                "normalization"
            ].__setitem__("non_qr1_noop_count", 1),
            "Hartigan record without complete qR partition": lambda run: run[
                "hartigan_levels"
            ].__setitem__("every_record_uses_complete_batch_qr_partition", False),
            "legacy scalar qR": lambda run: run["hartigan_levels"].__setitem__(
                "legacy_single_group_scalar_qr_record_count", 1
            ),
            "binary64 Hartigan level": lambda run: run["hartigan_levels"].__setitem__(
                "binary64_level_count", 1
            ),
            "missing no-op level": lambda run: run["hartigan_levels"][
                "records_by_order"
            ][1].__setitem__("rational_record_count", 2),
            "unreduced Hartigan level": lambda run: run["hartigan_levels"][
                "records_by_order"
            ][1].__setitem__("first_level", {"denominator": "2", "numerator": "2"}),
            "decreasing Hartigan levels": lambda run: run["hartigan_levels"][
                "records_by_order"
            ][1].__setitem__("last_level", {"denominator": "1", "numerator": "1"}),
            "vertical map open": lambda run: run["forest"][
                "vertical_maps_complete_between_adjacent_orders"
            ].__setitem__(0, False),
            "K1 closed cut open": lambda run: run["forest"].__setitem__(
                "k1_closed_cut_complete", False
            ),
            "wrong view": lambda run: run["view"].__setitem__(
                "relation", "greater_than"
            ),
            "public exact": lambda run: run.__setitem__("public_status", "exact"),
        }
        for label, mutate in mutations.items():
            hostile = copy.deepcopy(valid)
            mutate(hostile)
            with self.subTest(label=label), self.assertRaises(campaign.ContractError):
                self.validate(hostile, self.request)

    def test_massive_receipt_requires_one_fresh_resume_and_all_digest_equalities(
        self,
    ) -> None:
        massive_request = campaign.expected_massive_requests(self.plan)[0]
        valid = make_complete_run(massive_request)
        self.validate(valid, massive_request)
        for key in (
            "normalized_resident_v7_digest_equivalent",
            "k1_closed_cut_digest_equivalent",
            "horizontal_forest_digest_equivalent",
            "all_adjacent_vertical_map_digest_equivalent",
            "hartigan_manifest_digest_equivalent",
            "at_least20_view_digest_equivalent",
            "fresh_recertification_complete",
            "fresh_replay_complete",
        ):
            hostile = copy.deepcopy(valid)
            hostile["resume"][key] = False
            with self.subTest(key=key), self.assertRaisesRegex(
                campaign.ContractError, "resume"
            ):
                self.validate(hostile, massive_request)
        hostile = copy.deepcopy(valid)
        hostile["resume"]["fresh_process_resume_count"] = 0
        with self.assertRaisesRegex(campaign.ContractError, "resume"):
            self.validate(hostile, massive_request)

    def test_secondary_p95_and_massive_progression_are_exact(self) -> None:
        measured_requests = [
            request
            for request in campaign.expected_50k_requests(self.plan)
            if request["role"] == "measured"
        ]
        runs = [
            make_complete_run(request, warm_e2e_ns=(index + 1) * 10_000_000)
            for index, request in enumerate(measured_requests)
        ]
        summary = campaign.build_50k_summary(runs, self.plan)
        self.assertEqual(summary["aggregate"]["p95_warm_e2e_ns"], 290_000_000)
        self.assertTrue(summary["gate_passed"])
        threshold = copy.deepcopy(runs)
        threshold[-1]["timings_ns"]["warm_e2e"] = campaign.SLO_P95_NS
        self.assertFalse(
            campaign.build_50k_summary(threshold, self.plan)["gate_passed"]
        )
        complete_primary = [
            make_complete_run(
                request,
                warm_e2e_ns=(
                    campaign.SLO_P95_NS if request["role"] == "measured" else 10_000_000
                ),
            )
            for request in campaign.expected_50k_requests(self.plan)
        ]
        with self.assertRaisesRegex(campaign.ContractError, "massive scales"):
            campaign._validate_committed_prefix(complete_primary, self.plan)

        self.assertEqual(campaign.validate_scale_progression([]), 50_000)
        self.assertEqual(campaign.validate_scale_progression([50_000]), 1_000_000)
        self.assertEqual(
            campaign.validate_scale_progression([50_000, 1_000_000]),
            10_000_001,
        )
        self.assertEqual(
            campaign.validate_scale_progression([50_000, 1_000_000, 10_000_001]),
            30_000_000,
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
        ):
            with self.subTest(invalid=invalid), self.assertRaisesRegex(
                campaign.ContractError, "contiguous registered prefix"
            ):
                campaign.validate_scale_progression(invalid)


if __name__ == "__main__":
    unittest.main(verbosity=2)
