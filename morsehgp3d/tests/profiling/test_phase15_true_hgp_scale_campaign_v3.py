#!/usr/bin/env python3
"""Focused fail-closed tests for the future true-HGP v3 contract."""

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
from unittest import mock

DIRECTORY = Path(__file__).resolve().parent
HARNESS = DIRECTORY / "phase15_true_hgp_scale_campaign_v3.py"
PLAN = DIRECTORY / "phase15_true_hgp_scale_campaign_v3.json"
V2_HARNESS = DIRECTORY / "phase15_true_hgp_scale_campaign.py"
V1_PLAN = DIRECTORY / "phase15_true_hgp_scale_campaign_v1.json"
V2_PLAN = DIRECTORY / "phase15_true_hgp_scale_campaign_v2.json"
V2_TEST = DIRECTORY / "test_phase15_true_hgp_scale_campaign.py"
if str(DIRECTORY) not in sys.path:
    sys.path.insert(0, str(DIRECTORY))

import campaign_runtime as runtime

SPEC = importlib.util.spec_from_file_location(
    "phase15_true_hgp_scale_campaign_v3", HARNESS
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
}


def digest(label: str) -> str:
    return hashlib.sha256(label.encode("ascii")).hexdigest()


def make_complete_run(
    request: dict[str, object], *, warm_e2e_ns: int = 10_000_000
) -> dict[str, object]:
    run_id = request["run_id"]
    assert isinstance(run_id, str)
    counts = []
    hartigan_orders = []
    manifest_record_count = 0
    for order in range(1, campaign.MAXIMUM_ORDER + 1):
        materialized = 2
        omitted_noop = 0 if order == 1 else 1
        normalized = materialized + omitted_noop
        counts.append(
            {
                "complete_carriers": order * 5,
                "direct_events": order * 3,
                "facet_gateways": order * 2,
                "horizontal_nodes_invisible": order,
                "horizontal_nodes_visible": order * 2,
                "materialized_equal_level_batches": materialized,
                "normalized_equal_level_batches": normalized,
                "normalized_omitted_noop_qr1_batches": omitted_noop,
                "order": order,
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
                "normalized_equal_level_batch_count": normalized,
                "normalized_omitted_noop_qr1_batch_count": omitted_noop,
                "order": order,
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
            "checkpoint_manifest_sha256": digest(f"checkpoint:{run_id}"),
            "normalized_source_manifest_file": (
                f"artifacts/{run_id}-normalized-source.json"
            ),
            "normalized_source_manifest_sha256": digest(f"source:{run_id}"),
            "output_chain_sha256": digest(f"output:{run_id}"),
        },
        "backend": campaign.BACKEND,
        "binary_sha256": BINARY_SHA256,
        "closure": {
            "at_least20_condensed_view_complete": True,
            "batches_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "carriers_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "checkpoint_closed": True,
            "direct_events_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "exact_replay_closed": True,
            "facet_gateways_complete_by_order": [True] * campaign.MAXIMUM_ORDER,
            "hartigan_levels_complete": True,
            "horizontal_forests_complete_by_order": ([True] * campaign.MAXIMUM_ORDER),
            "incidence_complete_reduction_proved": True,
            "normalized_noop_qr1_complete_by_order": ([True] * campaign.MAXIMUM_ORDER),
            "numeric_failure_count": 0,
            "output_chain_closed": True,
            "regularity_certificate_complete": True,
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
            "vertical_chain_sha256": digest(f"vertical:{run_id}"),
            "vertical_maps_complete_between_adjacent_orders": (
                [True] * (campaign.MAXIMUM_ORDER - 1)
            ),
        },
        "git_sha": GIT_SHA,
        "hartigan_levels": {
            "all_normalized_equal_level_batches_covered": True,
            "binary64_level_count": 0,
            "manifest_file": f"artifacts/{run_id}-hartigan-v3.jsonl",
            "manifest_record_count": manifest_record_count,
            "manifest_sha256": digest(f"hartigan-manifest:{run_id}"),
            "ordered_rational_chain_sha256": digest(f"hartigan-chain:{run_id}"),
            "records_by_order": hartigan_orders,
            "representation": campaign.HARTIGAN_LEVEL_REPRESENTATION,
            "schema": campaign.HARTIGAN_LEVEL_RECEIPT_SCHEMA,
        },
        "maximum_order": campaign.MAXIMUM_ORDER,
        "mode": campaign.MODE,
        "normalization": {
            "contract": copy.deepcopy(campaign.NORMALIZED_H0_CONTRACT),
            "contract_v2_identity_compatible": False,
            "normalized_batch_chain_closed": True,
            "normalized_batch_chain_sha256": digest(f"normalized:{run_id}"),
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
            "at_least20_view_digest_equivalent": resume_required,
            "forced_checkpoint_count": 1 if resume_required else 0,
            "fresh_process_resume_count": 1 if resume_required else 0,
            "horizontal_forest_digest_equivalent": resume_required,
            "normalized_source_digest_equivalent": resume_required,
            "required": resume_required,
            "status": "complete" if resume_required else "not_required",
            "vertical_map_digest_equivalent": resume_required,
        },
        "role": request["role"],
        "run_id": run_id,
        "schema": campaign.RUN_SCHEMA,
        "seed": request["seed"],
        "source": {
            "carrier_chain_sha256": digest(f"carrier:{run_id}"),
            "contract": copy.deepcopy(campaign.SOURCE_REDUCTION_CONTRACT),
            "direct_event_chain_sha256": digest(f"direct:{run_id}"),
            "facet_gateway_chain_sha256": digest(f"gateway:{run_id}"),
            "incidence_reduction_proof_receipt_sha256": digest(
                "incidence-complete-reduction-proof"
            ),
            "proof_basis": "incidence_complete_reduction_proved",
            "regularity_certificate_sha256": digest(f"regularity:{run_id}"),
            "regularity_status": "certified_regular",
        },
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


def install_fake_binary(
    directory: Path,
    *,
    timeout_session_numbers: tuple[int, ...] = (),
    invalid_session_numbers: tuple[int, ...] = (),
) -> tuple[Path, Path, Path]:
    """Install a protocol-faithful fake whose own digest is self-reported."""

    binary = directory / "fake-true-hgp-v3.py"
    state = directory / "fake-state.json"
    log = directory / "fake-session-log.jsonl"
    source = f"""#!/usr/bin/env python3
import json
from pathlib import Path
import sys
import time

DIRECTORY = Path({str(DIRECTORY)!r})
PLAN = Path({str(PLAN)!r})
STATE = Path({str(state)!r})
LOG = Path({str(log)!r})
GIT_SHA = {GIT_SHA!r}
TIMEOUT_SESSION_NUMBERS = {list(timeout_session_numbers)!r}
INVALID_SESSION_NUMBERS = {list(invalid_session_numbers)!r}
sys.path.insert(0, str(DIRECTORY))

import campaign_runtime as runtime
import phase15_true_hgp_scale_campaign_v3 as campaign
import test_phase15_true_hgp_scale_campaign_v3 as fixture

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
session = runtime.parse_json_text(raw, "fake v3 session", canonical_line=True)
if set(session) != {{
    "active_checkpoint", "identity", "ordinal", "request_chain_sha256",
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

fixture.BINARY_SHA256 = binary_sha256
fixture.PLAN_SHA256 = plan_sha256
fixture.GIT_SHA = GIT_SHA
run = fixture.make_complete_run(request)
if session_number in INVALID_SESSION_NUMBERS:
    run["public_status"] = "exact"
print(runtime.canonical_json(run))
"""
    binary.write_text(source, encoding="ascii")
    binary.chmod(0o700)
    return binary, state, log


class TrueHgpV3PlanTests(unittest.TestCase):
    def setUp(self) -> None:
        self.plan, self.plan_digest = campaign.read_plan(PLAN)

    def test_v3_gate_identity_source_scales_and_secondary_slo_are_frozen(self) -> None:
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
            "regular_compressed_direct_plus_per_facet_gateway_plus_complete_carriers_v1",
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
        with self.assertRaisesRegex(campaign.ContractError, "frozen v3"):
            campaign.validate_plan(changed)
        for legacy in (V1_PLAN, V2_PLAN):
            with self.subTest(legacy=legacy.name), self.assertRaisesRegex(
                campaign.ContractError, "frozen v3"
            ):
                campaign.validate_plan(json.loads(legacy.read_text(encoding="ascii")))

    def test_v1_v2_harness_plans_and_tests_remain_byte_identical(self) -> None:
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
        hostile = copy.deepcopy(capabilities)
        hostile["capabilities"]["incidence_complete_reduction_proved"] = False
        with self.assertRaisesRegex(campaign.ContractError, "does not satisfy"):
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
                "phase15_true_hgp_v3_entry_gate_satisfied=false", result.stderr
            )
            self.assertNotIn("binary", result.stderr.split("false", maxsplit=1)[-1])
            self.assertFalse(spool.exists())


class TrueHgpV3ExecutionTests(unittest.TestCase):
    def setUp(self) -> None:
        self.closed_plan, self.plan_digest = campaign.read_plan(PLAN)

    def execute_open(
        self,
        *,
        plan: dict[str, object],
        binary: Path,
        spool_parent: Path,
    ) -> None:
        with mock.patch.object(
            campaign, "validate_plan", side_effect=lambda value: value
        ):
            campaign.execute_campaign(
                plan=plan,
                plan_path=PLAN,
                plan_sha256=self.plan_digest,
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
            binary, state, log = install_fake_binary(root)
            spool_parent = root / "spool"
            open_plan = copy.deepcopy(self.closed_plan)
            open_plan["entry_gate_satisfied"] = True

            self.execute_open(
                plan=open_plan,
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
                    for ordinal, session in enumerate(sessions)
                )
            )

            # A complete spool is a no-op after the mandatory fresh handshake.
            self.execute_open(
                plan=open_plan,
                binary=binary,
                spool_parent=spool_parent,
            )
            self.assertEqual(
                json.loads(state.read_text(encoding="ascii"))["session_count"], 39
            )

    def test_timeout_resumes_same_checkpoint_and_invalid_receipt_is_not_committed(
        self,
    ) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            binary, _, log = install_fake_binary(
                root,
                timeout_session_numbers=(0,),
                invalid_session_numbers=(2,),
            )
            spool_parent = root / "spool"
            open_plan = copy.deepcopy(self.closed_plan)
            open_plan["entry_gate_satisfied"] = True
            open_plan["fifty_k_protocol"]["wall_time_cap_ms_per_run"] = 500

            with self.assertRaisesRegex(runtime.RuntimeContractError, "timeout"):
                self.execute_open(
                    plan=open_plan,
                    binary=binary,
                    spool_parent=spool_parent,
                )
            timed_out_head = self.read_only_head(spool_parent)
            self.assertEqual(timed_out_head["next_ordinal"], 0)
            self.assertEqual(timed_out_head["active_run"]["ordinal"], 0)

            with self.assertRaisesRegex(campaign.ContractError, "public exactness"):
                self.execute_open(
                    plan=open_plan,
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


class TrueHgpV3RunTests(unittest.TestCase):
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
            "irregular source": lambda run: run["source"].__setitem__(
                "regularity_status", "unsupported_degeneracy"
            ),
            "global Gamma": lambda run: run["source"]["contract"].__setitem__(
                "global_gamma_materialized", True
            ),
            "v2 identity": lambda run: run["normalization"].__setitem__(
                "contract_v2_identity_compatible", True
            ),
            "nonzero point no-op delta": lambda run: run["normalization"].__setitem__(
                "nonzero_point_delta_noop_count", 1
            ),
            "nonzero parent no-op delta": lambda run: run["normalization"].__setitem__(
                "nonzero_parent_or_node_delta_noop_count", 1
            ),
            "batch normalization mismatch": lambda run: run["counts_by_order"][
                1
            ].__setitem__("normalized_equal_level_batches", 9),
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
            "normalized_source_digest_equivalent",
            "horizontal_forest_digest_equivalent",
            "vertical_map_digest_equivalent",
            "at_least20_view_digest_equivalent",
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
