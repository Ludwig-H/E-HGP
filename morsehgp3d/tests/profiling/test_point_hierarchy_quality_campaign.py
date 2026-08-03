#!/usr/bin/env python3
"""Single bounded protocol test for point-quality campaign v2."""

from __future__ import annotations

import hashlib
import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest
import zlib


DIRECTORY = Path(__file__).resolve().parent
HARNESS = DIRECTORY / "point_hierarchy_quality_campaign.py"
PLAN = DIRECTORY / "point_hierarchy_quality_campaign_v2.json"
if str(DIRECTORY) not in sys.path:
    sys.path.insert(0, str(DIRECTORY))
SPEC = importlib.util.spec_from_file_location("point_hierarchy_quality_campaign", HARNESS)
assert SPEC is not None and SPEC.loader is not None
campaign = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(campaign)
runtime = __import__("campaign_runtime")
generator = campaign.generator


def open_plan(path: Path) -> None:
    runtime.atomic_publish(path, runtime.canonical_bytes(campaign.expected_plan_document(entry_gate_satisfied=True)))


def compact_protocol_generate(*, dataset: dict[str, object], point_count: int, generator_sha256: str, output_directory: Path) -> dict[str, object]:
    """Test-only materializer: every record exists, but all coordinates are zero."""

    truth = {"point_count": point_count, "runs": generator.truth_runs(dataset, point_count), "schema": generator.LABEL_RLE_SCHEMA, "semantics": "ground_truth"}
    truth_payload = runtime.canonical_bytes(truth)
    truth_path = output_directory / "truth-labels.rle.json"
    runtime.atomic_publish(truth_path, truth_payload)
    truth_sha = runtime.sha256_bytes(truth_payload)
    coordinate_path = output_directory / "raw-cloud.i64x3.zlib"
    raw_digest = hashlib.sha256(generator.RAW_CLOUD_DIGEST_DOMAIN)
    compressed_digest = hashlib.sha256()
    compressor = zlib.compressobj(level=1)
    remaining = point_count * generator.COORDINATE_RECORD_BYTES
    size = 0
    zero_block = bytes(1024 * 1024)
    with coordinate_path.open("wb") as stream:
        while remaining:
            payload = zero_block[: min(remaining, len(zero_block))]
            raw_digest.update(payload)
            compressed = compressor.compress(payload)
            if compressed:
                stream.write(compressed)
                compressed_digest.update(compressed)
                size += len(compressed)
            remaining -= len(payload)
        compressed = compressor.flush()
        stream.write(compressed)
        compressed_digest.update(compressed)
        size += len(compressed)
        stream.flush()
        os.fsync(stream.fileno())
    coordinate = {"file": coordinate_path.name, "sha256": compressed_digest.hexdigest(), "size_bytes": size}
    raw_sha = raw_digest.hexdigest()
    cloud = {
        "coordinate_encoding": {"compression": "zlib_level_1", "coordinate_denominator_exponent": generator.COORDINATE_DENOMINATOR_EXPONENT, "point_id_domain": [0, point_count], "record_bytes": generator.COORDINATE_RECORD_BYTES, "record_layout": "little_endian_signed_int64_x_y_z"},
        "coordinate_stream": coordinate,
        "dataset": dataset,
        "generator_id": generator.GENERATOR_ID,
        "generator_sha256": generator_sha256,
        "geometry_contract": generator.geometry_contract(dataset, point_count),
        "point_count": point_count,
        "raw_cloud_sha256": raw_sha,
        "raw_coordinate_bytes": point_count * generator.COORDINATE_RECORD_BYTES,
        "schema": generator.CLOUD_SCHEMA,
        "truth_rle_sha256": truth_sha,
    }
    cloud_payload = runtime.canonical_bytes(cloud)
    cloud_path = output_directory / "raw-cloud.manifest.json"
    runtime.atomic_publish(cloud_path, cloud_payload)
    return {"cloud": {"file": cloud_path.name, "sha256": runtime.sha256_bytes(cloud_payload), "size_bytes": len(cloud_payload)}, "coordinates": coordinate, "raw_cloud_sha256": raw_sha, "truth": {"file": truth_path.name, "sha256": truth_sha, "size_bytes": len(truth_payload)}}


def install_producer(directory: Path, plan: Path, *, mutation: str = "", fail_once_run_id: str = "") -> tuple[Path, Path]:
    binary = directory / f"fake-producer-{mutation or 'valid'}.py"
    state = directory / f"producer-state-{mutation or 'valid'}.json"
    log = directory / f"producer-log-{mutation or 'valid'}.jsonl"
    source = f'''#!/usr/bin/env python3
from pathlib import Path
import sys

DIRECTORY = Path({str(DIRECTORY)!r})
PLAN = Path({str(plan)!r})
STATE = Path({str(state)!r})
LOG = Path({str(log)!r})
MUTATION = {mutation!r}
FAIL_ONCE_RUN_ID = {fail_once_run_id!r}
sys.path.insert(0, str(DIRECTORY))
import campaign_runtime as runtime
import point_hierarchy_quality_campaign as campaign
import point_hierarchy_quality_generator as generator

binary = Path(__file__)
binary_sha = runtime.sha256_file(binary)
_, plan_sha = campaign.read_plan(PLAN)

def write_artifact(name, value):
    path = Path.cwd() / name
    payload = runtime.canonical_bytes(value)
    runtime.atomic_publish(path, payload)
    return {{"file": name, "sha256": runtime.sha256_bytes(payload), "size_bytes": len(payload)}}

def resources():
    return {{"peak_device_bytes": 1024, "peak_host_bytes": 2048, "wall_time_ns": 4096}}

def log_and_maybe_fail(transaction, operation):
    record = {{"operation": operation, "run_id": transaction["run_id"]}}
    with LOG.open("ab") as stream:
        stream.write(runtime.canonical_bytes(record))
    state = {{"failed": False}}
    if STATE.exists():
        state = runtime.read_canonical_json(STATE, "producer state")[0]
    if transaction["run_id"] == FAIL_ONCE_RUN_ID and not state["failed"]:
        state["failed"] = True
        runtime.atomic_publish(STATE, runtime.canonical_bytes(state))
        raise SystemExit(75)

if sys.argv[1:] == [campaign.PRODUCER_CAPABILITY_ARGUMENT]:
    capability = {{"backend": campaign.BACKEND, "binary_sha256": binary_sha, "binary_size_bytes": binary.stat().st_size, "contract": campaign.producer_contract(), "git_sha": "a" * 40, "mode": campaign.MODE, "plan_sha256": plan_sha, "profile": campaign.PROFILE, "schema": campaign.PRODUCER_CAPABILITY_SCHEMA}}
    print(runtime.canonical_json(capability))
    raise SystemExit(0)

request = runtime.parse_json_text(sys.stdin.read(), "fake producer request", canonical_line=True)
transaction = request["transaction"]
if sys.argv[1:] == [campaign.SOURCE_ARGUMENT]:
    log_and_maybe_fail(transaction, "source")
    key = transaction["dataset_id"]
    authority = runtime.sha256_bytes(("authority:" + key).encode("ascii"))
    orders = [{{"forest_sha256": runtime.sha256_bytes(f"forest:{{key}}:{{order}}".encode("ascii")), "order": order}} for order in range(1, 11)]
    verticals = [{{"from_order": order, "to_order": order - 1, "vertical_sha256": runtime.sha256_bytes(f"vertical:{{key}}:{{order}}".encode("ascii"))}} for order in range(2, 11)]
    bundle = {{"cloud_manifest_sha256": request["cloud_manifest_sha256"], "order_forest_digests": orders, "raw_cloud_sha256": request["raw_cloud_sha256"], "schema": "fake.exact_t1_t10.bundle.v2", "vertical_map_digests": verticals}}
    bundle_descriptor = write_artifact("source-bundle.json", bundle)
    certificate = {{"authority_id": authority, "cloud_authority_binding_sha256": "0" * 64, "cloud_manifest_sha256": request["cloud_manifest_sha256"], "complete_order_range": [1, 10], "exact_source_certified": True, "maximum_order": 10, "order_forest_digests": orders, "public_exact_status_claimed": False, "raw_cloud_sha256": request["raw_cloud_sha256"], "schema": campaign.SOURCE_CERTIFICATE_SCHEMA, "source_bundle_sha256": bundle_descriptor["sha256"], "source_bundle_size_bytes": bundle_descriptor["size_bytes"], "surrogate_source_used": False, "truth_rle_sha256": request["truth_sha256"], "vertical_map_digests": verticals}}
    certificate["cloud_authority_binding_sha256"] = runtime.sha256_bytes(runtime.canonical_bytes({{"authority_id": authority, "cloud_manifest_sha256": request["cloud_manifest_sha256"], "order_forest_digests": orders, "raw_cloud_sha256": request["raw_cloud_sha256"], "source_bundle_sha256": bundle_descriptor["sha256"], "source_bundle_size_bytes": bundle_descriptor["size_bytes"], "truth_rle_sha256": request["truth_sha256"], "vertical_map_digests": verticals}}))
    if MUTATION == "false_source_binding":
        certificate["cloud_authority_binding_sha256"] = "f" * 64
    print(runtime.canonical_json({{"resources": resources(), "source_bundle": bundle_descriptor, "source_certificate": write_artifact("source-certificate.json", certificate)}}))
    raise SystemExit(0)

if sys.argv[1:] == [campaign.REDUCTION_ARGUMENT]:
    log_and_maybe_fail(transaction, "reduction")
    source = runtime.read_canonical_json(Path(request["source_certificate_file"]), "fake source")[0]
    truth_runs = generator.truth_runs(transaction["dataset"], transaction["point_count"])
    children = list(range(1, len(truth_runs) + 1))
    nodes = [{{"children": children, "coverage": [[0, transaction["point_count"]]], "id": 0, "parent": None, "terminal": False}}]
    for node_id, (begin, end, _label) in enumerate(truth_runs, start=1):
        nodes.append({{"children": [], "coverage": [[begin, end]], "id": node_id, "parent": 0, "terminal": True}})
    order_prefix = source["order_forest_digests"][: transaction["maximum_order"]]
    if MUTATION == "change_k_prefix" and transaction["maximum_order"] == 1:
        order_prefix[0] = dict(order_prefix[0])
        order_prefix[0]["forest_sha256"] = "f" * 64
    selected = [{{"label": label, "node_id": node_id}} for node_id, (_begin, _end, label) in enumerate(truth_runs, start=1) if label >= 0]
    dbscan = {{"point_count": transaction["point_count"], "runs": truth_runs, "schema": generator.LABEL_RLE_SCHEMA, "selected_nodes": selected, "semantics": "dbscan_radius"}}
    eom = {{"point_count": transaction["point_count"], "runs": truth_runs, "schema": generator.LABEL_RLE_SCHEMA, "selected_nodes": selected, "semantics": "excess_of_mass"}}
    if MUTATION == "invalid_labels" and transaction["maximum_order"] == 1 and transaction["exp_z"]["numerator"] == 1:
        dbscan["runs"] = [list(record) for record in truth_runs]
        dbscan["runs"][0][0] = 1
    dbscan_descriptor = write_artifact("dbscan-labels.json", dbscan)
    eom_descriptor = write_artifact("eom-labels.json", eom)
    k_value = transaction["maximum_order"]
    calibration = {{"epsilon_squared": {{"denominator": str(1 << (k_value + 4)), "numerator": "1"}}, "neighbor_rank": 1 if k_value == 1 else k_value - 1, "quantile_denominator": 100, "quantile_numerator": 90, "raw_cloud_sha256": source["raw_cloud_sha256"], "reference_order": k_value, "tie_policy": "include_complete_equal_distance_shell"}}
    calibration["calculation_receipt_sha256"] = runtime.sha256_bytes(runtime.canonical_bytes(calibration))
    hierarchy = {{"dbscan_calibration": calibration, "dbscan_labels_sha256": dbscan_descriptor["sha256"], "eom_labels_sha256": eom_descriptor["sha256"], "exp_z": transaction["exp_z"], "nodes": nodes, "options": transaction["options"], "order_forest_digests": order_prefix, "point_count": transaction["point_count"], "public_exact_status_claimed": False, "reduction_receipt_sha256": runtime.sha256_bytes(("reduction:" + transaction["run_id"]).encode("ascii")), "root_id": 0, "schema": campaign.HIERARCHY_CERTIFICATE_SCHEMA, "source_bundle_sha256": request["source_bundle_sha256"], "source_certificate_sha256": request["source_certificate_sha256"], "vertical_map_digests": source["vertical_map_digests"][: max(0, transaction["maximum_order"] - 1)]}}
    output = {{"dbscan_labels": dbscan_descriptor, "eom_labels": eom_descriptor, "hierarchy_certificate": write_artifact("hierarchy-certificate.json", hierarchy), "resources": resources()}}
    if MUTATION == "self_declared_metrics" and transaction["maximum_order"] == 1 and transaction["exp_z"]["numerator"] == 1:
        output["metrics"] = {{"adjusted_rand_all": 2.0}}
    print(runtime.canonical_json(output))
    raise SystemExit(0)

if sys.argv[1:] == [campaign.BASELINE_ARGUMENT]:
    log_and_maybe_fail(transaction, "baseline")
    if transaction["scale_role"] == "massive":
        print(runtime.canonical_json({{"reason": "bounded fake baseline intentionally omitted", "resources": resources(), "status": "not_run_resource_cap"}}))
        raise SystemExit(0)
    runs = generator.truth_runs(transaction["dataset"], transaction["point_count"])
    dbscan = {{"point_count": transaction["point_count"], "runs": runs, "schema": generator.LABEL_RLE_SCHEMA, "semantics": "sklearn_dbscan"}}
    hdbscan = {{"point_count": transaction["point_count"], "runs": runs, "schema": generator.LABEL_RLE_SCHEMA, "semantics": "sklearn_hdbscan_eom"}}
    print(runtime.canonical_json({{"dbscan_labels": write_artifact("baseline-dbscan.json", dbscan), "hdbscan_labels": write_artifact("baseline-hdbscan.json", hdbscan), "resources": resources(), "status": "complete"}}))
    raise SystemExit(0)
raise SystemExit(64)
'''
    runtime.atomic_publish(binary, source.encode("ascii"))
    binary.chmod(0o755)
    return binary, log


def install_verifier(directory: Path, plan: Path, *, mutation: str = "") -> tuple[Path, Path]:
    binary = directory / f"fake-verifier-{mutation or 'valid'}.py"
    log = directory / f"verifier-log-{mutation or 'valid'}.jsonl"
    source = f'''#!/usr/bin/env python3
from pathlib import Path
import sys

DIRECTORY = Path({str(DIRECTORY)!r})
PLAN = Path({str(plan)!r})
LOG = Path({str(log)!r})
MUTATION = {mutation!r}
sys.path.insert(0, str(DIRECTORY))
import campaign_runtime as runtime
import point_hierarchy_quality_campaign as campaign

binary = Path(__file__)
binary_sha = runtime.sha256_file(binary)
_, plan_sha = campaign.read_plan(PLAN)
if sys.argv[1:] == [campaign.VERIFIER_CAPABILITY_ARGUMENT]:
    capability = {{"backend": campaign.BACKEND, "binary_sha256": binary_sha, "binary_size_bytes": binary.stat().st_size, "contract": campaign.verifier_contract(), "git_sha": "a" * 40, "mode": campaign.MODE, "plan_sha256": plan_sha, "profile": campaign.PROFILE, "schema": campaign.VERIFIER_CAPABILITY_SCHEMA}}
    print(runtime.canonical_json(capability))
    raise SystemExit(0)
request = runtime.parse_json_text(sys.stdin.read(), "fake verifier request", canonical_line=True)
operation = "source" if sys.argv[1:] == [campaign.VERIFY_SOURCE_ARGUMENT] else "hierarchy"
with LOG.open("ab") as stream:
    stream.write(runtime.canonical_bytes({{"operation": operation}}))
if sys.argv[1:] == [campaign.VERIFY_SOURCE_ARGUMENT]:
    if runtime.sha256_file(Path(request["cloud_manifest_file"])) != request["cloud_manifest_sha256"] or runtime.sha256_file(Path(request["coordinate_stream_file"])) != request["coordinate_stream_sha256"] or runtime.sha256_file(Path(request["source_bundle_file"])) != request["source_bundle_sha256"] or runtime.sha256_file(Path(request["source_certificate_file"])) != request["source_certificate_sha256"]:
        raise SystemExit(65)
    receipt = {{"all_orders_replayed": True, "bundle_replayed": True, "cloud_binding_replayed": True, "cloud_manifest_sha256": request["cloud_manifest_sha256"], "coordinate_stream_sha256": request["coordinate_stream_sha256"], "public_exact_status_claimed": False, "raw_cloud_sha256": request["raw_cloud_sha256"], "schema": campaign.SOURCE_VERIFICATION_SCHEMA, "source_bundle_sha256": request["source_bundle_sha256"], "source_certificate_sha256": request["source_certificate_sha256"], "verifier_sha256": binary_sha, "vertical_maps_replayed": True}}
    receipt["receipt_sha256"] = runtime.sha256_bytes(runtime.canonical_bytes(receipt))
    print(runtime.canonical_json(receipt))
    raise SystemExit(0)
if sys.argv[1:] == [campaign.VERIFY_HIERARCHY_ARGUMENT]:
    if runtime.sha256_file(Path(request["source_bundle_file"])) != request["source_bundle_sha256"] or runtime.sha256_file(Path(request["source_certificate_file"])) != request["source_certificate_sha256"] or runtime.sha256_file(Path(request["hierarchy_certificate_file"])) != request["hierarchy_certificate_sha256"]:
        raise SystemExit(65)
    receipt = {{"acyclic": True, "all_cuts_laminar": True, "children_disjoint": MUTATION != "false_laminarity", "hierarchy_certificate_sha256": request["hierarchy_certificate_sha256"], "parent_unique": True, "public_exact_status_claimed": False, "root_covers_all_points": True, "schema": campaign.HIERARCHY_VERIFICATION_SCHEMA, "source_bundle_sha256": request["source_bundle_sha256"], "source_certificate_sha256": request["source_certificate_sha256"], "terminal_unique": True, "verifier_sha256": binary_sha}}
    receipt["receipt_sha256"] = runtime.sha256_bytes(runtime.canonical_bytes(receipt))
    print(runtime.canonical_json(receipt))
    raise SystemExit(0)
raise SystemExit(64)
'''
    runtime.atomic_publish(binary, source.encode("ascii"))
    binary.chmod(0o755)
    return binary, log


def run_campaign(root: Path, plan: Path, producer: Path, verifier: Path, name: str, *, spool_name: str | None = None, stop_before_epoch: int | None = None) -> dict[str, object]:
    return campaign.execute(plan_path=plan, producer=producer, verifier=verifier, spool_parent=root / f"spool-{spool_name or name}", output=root / f"output-{name}.json", stop_before_epoch=stop_before_epoch)


class PointHierarchyQualityCampaignV2Test(unittest.TestCase):
    def test_single_bounded_v2_protocol(self) -> None:
        plan, _ = campaign.read_plan(PLAN)
        self.assertFalse(plan["campaign_entry_gate_satisfied"])
        transactions = campaign.expected_transactions(plan)
        self.assertEqual((len(transactions), sum(item["role"] == "source" for item in transactions), sum(item["role"] == "reduction" for item in transactions), sum(item["role"] == "baseline" for item in transactions)), (84, 6, 54, 18))
        self.assertAlmostEqual(campaign._ari_nmi({(0, 0): 1, (0, 1): 1, (1, 0): 1, (1, 1): 2})[0], -0.25)

        separated, multiscale, filaments, bridged = plan["datasets"]["fifty_k"]["profiles"]
        separated_geometry = generator.geometry_contract(separated, 50_000)["shape"]
        multiscale_geometry = generator.geometry_contract(multiscale, 50_000)
        filament_geometry = generator.geometry_contract(filaments, 50_000)["shape"]
        bridged_geometry = generator.geometry_contract(bridged, 50_000)
        self.assertLessEqual(3 * (separated_geometry["euclidean_ball_radius_upper_bound_numerator"] // 2) ** 2, separated_geometry["euclidean_ball_radius_upper_bound_numerator"] ** 2)
        radii = multiscale_geometry["shape"]["euclidean_radius_upper_bound_numerators"]
        self.assertEqual((len(set(radii)), radii[-1] // radii[0], multiscale_geometry["cluster_size_profile"]["maximum"] // multiscale_geometry["cluster_size_profile"]["minimum"]), (8, 64, 8))
        self.assertEqual((filament_geometry["segment_count"], filament_geometry["tube_radius_numerator"]), (8, 1 << 40))
        self.assertEqual((bridged_geometry["cluster_size_profile"]["maximum"] // bridged_geometry["cluster_size_profile"]["minimum"], bridged_geometry["shape"]["bridge_point_count"]), (16, (45_000 * 2) // 100))

        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            actual = root / "actual-generator"
            actual.mkdir()
            actual_descriptors = generator.generate(dataset=separated, point_count=50_000, generator_sha256="1" * 64, output_directory=actual)
            actual_cloud = campaign._read_local_artifact(actual, actual_descriptors["cloud"], "actual generated cloud")
            campaign.validate_materialized_cloud(actual_cloud, root=actual, coordinate_reference=actual_descriptors["coordinates"], transaction=transactions[0], generator_sha256="1" * 64, truth_sha256=actual_descriptors["truth"]["sha256"], label="actual generated cloud")

            with self.assertRaisesRegex(campaign.ContractError, "entry_gate_satisfied=false"):
                campaign.execute(plan_path=PLAN, producer=root / "missing-producer", verifier=root / "missing-verifier", spool_parent=root / "closed-spool", output=root / "closed-output.json")

            open_path = root / "open-plan.json"
            open_plan(open_path)
            producer, producer_log = install_producer(root, open_path, fail_once_run_id="50k-separated_8-k1-z1")
            verifier, verifier_log = install_verifier(root, open_path)
            with self.assertRaisesRegex(campaign.ContractError, "distinct executable"):
                run_campaign(root, open_path, producer, producer, "producer-alone")

            original_generate = generator.generate
            original_retire = campaign._retire_committed_attempt
            generator.generate = compact_protocol_generate
            crash = {"raised": False}
            def crash_after_first_commit(*args: object, **kwargs: object) -> None:
                if not crash["raised"]:
                    crash["raised"] = True
                    raise RuntimeError("injected crash after committed receipt")
                original_retire(*args, **kwargs)
            campaign._retire_committed_attempt = crash_after_first_commit
            try:
                with self.assertRaisesRegex(RuntimeError, "after committed receipt"):
                    run_campaign(root, open_path, producer, verifier, "resumable-success")
                campaign._retire_committed_attempt = original_retire
                campaign_root = next((root / "spool-resumable-success").glob("campaign-*"))
                self.assertTrue(any(any(path.iterdir()) for path in (campaign_root / "attempts").iterdir()))
                orphan = campaign_root / "artifacts" / "orphan-precommit.bin"
                runtime.atomic_publish(orphan, b"orphan\n")
                partial = run_campaign(root, open_path, producer, verifier, "resumable-success-partial", spool_name="resumable-success", stop_before_epoch=int(time.time()) + 800)
                self.assertEqual((partial["status"], partial["completed_transaction_count"], partial["next_transaction_ordinal"], partial["total_transaction_count"]), ("partial_resumable", 1, 1, 84))
                self.assertFalse(orphan.exists())
                with self.assertRaises(runtime.RuntimeContractError):
                    run_campaign(root, open_path, producer, verifier, "resumable-success")
                summary = run_campaign(root, open_path, producer, verifier, "resumable-success-final", spool_name="resumable-success")
                self.assertEqual((summary["status"], summary["completed_transaction_count"], summary["exact_source_build_count"], summary["next_transaction_ordinal"]), ("complete", 84, 6, 84))
                self.assertFalse(summary["claims"]["public_exact_status_claimed"])
                producer_records = [runtime.parse_json_text(line, "producer log", canonical_line=False) for line in producer_log.read_text(encoding="ascii").splitlines()]
                verifier_records = [runtime.parse_json_text(line, "verifier log", canonical_line=False) for line in verifier_log.read_text(encoding="ascii").splitlines()]
                self.assertEqual(sum(record["operation"] == "source" for record in producer_records), 6)
                self.assertEqual(sum(record["operation"] == "source" for record in verifier_records), 6)
                self.assertTrue(all(not any(path.iterdir()) for path in (campaign_root / "attempts").iterdir()))

                for mutation, expected in (("change_k_prefix", "source order prefix"), ("invalid_labels", "partition PointId"), ("self_declared_metrics", "labels and resources only"), ("false_source_binding", "authority binding")):
                    case = root / mutation
                    case.mkdir()
                    case_plan = case / "open-plan.json"
                    open_plan(case_plan)
                    bad_producer, _ = install_producer(case, case_plan, mutation=mutation)
                    case_verifier, _ = install_verifier(case, case_plan)
                    with self.assertRaisesRegex(campaign.ContractError, expected):
                        run_campaign(case, case_plan, bad_producer, case_verifier, mutation)

                laminar = root / "false-laminarity"
                laminar.mkdir()
                laminar_plan = laminar / "open-plan.json"
                open_plan(laminar_plan)
                laminar_producer, _ = install_producer(laminar, laminar_plan)
                laminar_verifier, _ = install_verifier(laminar, laminar_plan, mutation="false_laminarity")
                with self.assertRaisesRegex(campaign.ContractError, "children_disjoint"):
                    run_campaign(laminar, laminar_plan, laminar_producer, laminar_verifier, "false-laminarity")
            finally:
                campaign._retire_committed_attempt = original_retire
                generator.generate = original_generate


if __name__ == "__main__":
    unittest.main()
