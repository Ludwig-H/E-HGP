from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path
from typing import Any, Iterator


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from check_contracts import (  # noqa: E402
    FIXTURE_DIR,
    REQUIRED_DEFINITIONS,
    SCHEMA_PATH,
    ContractError,
    SchemaValidator,
    canonical_contract_id,
    canonical_json,
    load_json,
    validate_benchmark_semantics,
    validate_certificate_semantics,
    validate_result_semantics,
    validate_repository_contract,
    validate_round_trip,
)


def walk_keyed_values(value: Any) -> Iterator[tuple[dict[str, Any], str]]:
    if isinstance(value, dict):
        for key, child in value.items():
            yield value, key
            yield from walk_keyed_values(child)
    elif isinstance(value, list):
        for child in value:
            yield from walk_keyed_values(child)


def canonicalize_certificate_ids(value: dict[str, Any]) -> None:
    """Rekey a certificate after a test intentionally changes its evidence."""

    result = value if "run_certificate" in value else None
    certificate = value["run_certificate"] if result is not None else value
    snapshot = certificate["final_budget_snapshot"]
    snapshot["snapshot_id"] = canonical_contract_id(
        "BudgetSnapshot",
        {key: child for key, child in snapshot.items() if key != "snapshot_id"},
    )
    certificate["run_id"] = canonical_contract_id(
        "Run",
        {
            "input_sha256": certificate["input_semantics"]["input_sha256"],
            "k_max": certificate["k_max"],
            "profile": certificate["effective_profile"],
            "backend": certificate["effective_backend"],
            "mode": certificate["requested_mode"],
        },
    )
    certificate["certificate_id"] = canonical_contract_id(
        "RunCertificate",
        {
            key: child
            for key, child in certificate.items()
            if key != "certificate_id"
        },
    )
    if result is not None:
        result["run_id"] = certificate["run_id"]


class ContractSchemaTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.schema = load_json(SCHEMA_PATH)
        cls.validator = SchemaValidator(cls.schema)
        cls.definitions = cls.schema["$defs"]

    def test_repository_contract_and_fixtures_validate(self) -> None:
        sample_count, fixture_count = validate_repository_contract()
        self.assertGreaterEqual(sample_count, len(REQUIRED_DEFINITIONS))
        self.assertGreaterEqual(fixture_count, 5)

    def test_every_definition_example_round_trips_canonically(self) -> None:
        for name in sorted(REQUIRED_DEFINITIONS):
            with self.subTest(definition=name):
                for index, example in enumerate(self.definitions[name]["examples"]):
                    self.validator.check(example, self.definitions[name])
                    validate_round_trip(example, f"{name}[{index}]")

    def test_unknown_fields_are_rejected_by_critical_objects(self) -> None:
        checked = 0
        for name in sorted(REQUIRED_DEFINITIONS):
            definition = self.definitions[name]
            for example in definition["examples"]:
                if not isinstance(example, dict):
                    continue
                mutated = copy.deepcopy(example)
                mutated["unexpected_critical_field"] = True
                with self.subTest(definition=name):
                    with self.assertRaises(ContractError):
                        self.validator.check(mutated, definition)
                checked += 1
        self.assertEqual(checked, len(REQUIRED_DEFINITIONS))

    def test_incoherent_declared_units_are_rejected(self) -> None:
        root = copy.deepcopy(self.definitions["MorseHGP3DResult"]["examples"][0])
        candidates = [
            (owner, key)
            for owner, key in walk_keyed_values(root)
            if "unit" in key and isinstance(owner[key], str)
        ]
        self.assertTrue(candidates, "the public result must carry explicit unit fields")

        rejected = 0
        for owner, key in candidates:
            original = owner[key]
            owner[key] = "incoherent_unit"
            try:
                self.validator.check(root)
            except ContractError:
                rejected += 1
            finally:
                owner[key] = original
        self.assertGreater(rejected, 0, "at least one normative unit must be constrained")

    def test_canonical_json_rejects_non_finite_numbers(self) -> None:
        with self.assertRaises(ValueError):
            canonical_json({"value": float("nan")})

    def test_exact_status_is_a_function_of_completeness_gates(self) -> None:
        root = self.definitions["MorseHGP3DResult"]["examples"][0]
        certificate = root["run_certificate"]
        scalar_gates = (
            "relevant_gp_complete",
            "canonical_children_complete",
            "active_cross_incidences_complete",
            "vertical_maps_complete",
        )
        for field in scalar_gates:
            mutated = copy.deepcopy(root)
            mutated["run_certificate"][field] = False
            with self.subTest(gate=field):
                with self.assertRaises(ContractError):
                    validate_result_semantics(mutated)

        for field in (
            "catalog_complete_by_rank",
            "attachments_complete_by_order",
            "gamma_complete_by_order",
            "batches_complete_by_order",
        ):
            self.assertTrue(certificate[field])
            mutated = copy.deepcopy(root)
            mutated["run_certificate"][field][0] = False
            with self.subTest(gate=field):
                with self.assertRaises(ContractError):
                    validate_result_semantics(mutated)

    def test_exact_semantics_status_and_kind_are_equivalent(self) -> None:
        exact = self.definitions["MorseHGP3DResult"]["examples"][0]
        for field in (
            "forest_semantics",
            "public_status",
            "result_kind",
        ):
            mutated = copy.deepcopy(exact)
            certificate = mutated["run_certificate"]
            if field == "forest_semantics":
                certificate[field] = "partial_refinement"
            elif field == "public_status":
                certificate[field] = "conditional"
            elif field == "result_kind":
                certificate[field] = "partial_hierarchy"
            with self.subTest(exact_marker=field):
                with self.assertRaisesRegex(ContractError, "must be equivalent"):
                    validate_result_semantics(mutated)

        missing_requirement = copy.deepcopy(exact)
        missing_requirement["run_certificate"]["require_exact"] = False
        missing_requirement["run_certificate"]["budget_policy"][
            "require_exact"
        ] = False
        with self.assertRaisesRegex(ContractError, "requires require_exact=true"):
            validate_result_semantics(missing_requirement)

        partial = load_json(FIXTURE_DIR / "isolated-birth-k2.json")
        for field in (
            "forest_semantics",
            "public_status",
            "result_kind",
        ):
            mutated = copy.deepcopy(partial)
            certificate = mutated["run_certificate"]
            if field == "forest_semantics":
                certificate[field] = "exact"
            elif field == "public_status":
                certificate[field] = "exact"
            elif field == "result_kind":
                certificate[field] = "hierarchy"
            with self.subTest(partial_marker=field):
                with self.assertRaisesRegex(ContractError, "must be equivalent"):
                    validate_result_semantics(mutated)

    def test_fail_closed_requirement_may_end_without_an_exact_hierarchy(self) -> None:
        certificate = copy.deepcopy(
            self.definitions["RunCertificate"]["examples"][0]
        )
        certificate.pop("forest_semantics")
        certificate["result_kind"] = "verified_events_only"
        certificate["public_status"] = "numeric_failure"
        certificate["stop_reason"] = "numeric_failure"
        canonicalize_certificate_ids(certificate)
        self.assertTrue(certificate["require_exact"])
        validate_certificate_semantics(certificate)

    def test_exact_status_rejects_remaining_unknown_predicates(self) -> None:
        mutated = copy.deepcopy(
            self.definitions["MorseHGP3DResult"]["examples"][0]
        )
        mutated["run_certificate"]["exact_predicate_counts"][
            "remaining_unknown"
        ] = 1
        with self.assertRaisesRegex(ContractError, "unresolved exact predicates"):
            validate_result_semantics(mutated)

    def test_exact_gamma_requires_every_coface_and_batch_assignment(self) -> None:
        exact = load_json(FIXTURE_DIR / "k1-emst.json")
        self.assertTrue(exact["gamma_cofaces"])
        validate_result_semantics(exact)

        missing_record = copy.deepcopy(exact)
        removed = missing_record["gamma_cofaces"].pop()
        missing_record["run_certificate"]["work_and_memory_counters"][
            "gamma_cofaces_emitted"
        ] -= 1
        for batch in missing_record["equal_level_batches"]:
            if removed["coface_id"] in batch["gamma_coface_ids"]:
                batch["gamma_coface_ids"].remove(removed["coface_id"])
        canonicalize_certificate_ids(missing_record)
        with self.assertRaisesRegex(ContractError, "missing or inventing cofaces"):
            validate_result_semantics(missing_record)

        missing_assignment = copy.deepcopy(exact)
        assigned = next(
            batch
            for batch in missing_assignment["equal_level_batches"]
            if batch["gamma_coface_ids"]
        )
        assigned["gamma_coface_ids"].pop()
        with self.assertRaisesRegex(ContractError, "exactly one batch"):
            validate_result_semantics(missing_assignment)

    def test_gamma_structure_is_checked_even_when_completeness_is_false(self) -> None:
        base = load_json(FIXTURE_DIR / "isolated-birth-k2.json")
        base["run_certificate"]["gamma_complete_by_order"] = [False, False]
        canonicalize_certificate_ids(base)
        validate_result_semantics(base)

        mutations: list[tuple[str, str, Any]] = []

        bad_order = copy.deepcopy(base)
        bad_order["gamma_cofaces"][0]["order"] = bad_order["k_eff"] + 1
        mutations.append(("order", "outside 1..k_eff", bad_order))

        bad_point = copy.deepcopy(base)
        bad_point["gamma_cofaces"][0]["simplex_point_ids"][-1] = bad_point[
            "run_certificate"
        ]["input_point_count"]
        mutations.append(("point", "out-of-range point ID", bad_point))

        bad_cardinality = copy.deepcopy(base)
        bad_cardinality["gamma_cofaces"][0]["simplex_point_ids"].pop()
        mutations.append(("cardinality", "wrong cardinality", bad_cardinality))

        bad_facets = copy.deepcopy(base)
        bad_facets["gamma_cofaces"][0]["facet_point_ids"].reverse()
        mutations.append(("facets", "all canonical facets", bad_facets))

        duplicate_label = copy.deepcopy(base)
        duplicate = copy.deepcopy(duplicate_label["gamma_cofaces"][0])
        duplicate["coface_id"] = "0" * 64
        duplicate_label["gamma_cofaces"].append(duplicate)
        duplicate_label["run_certificate"]["work_and_memory_counters"][
            "gamma_cofaces_emitted"
        ] += 1
        canonicalize_certificate_ids(duplicate_label)
        mutations.append(
            ("duplicate-label", "unique \\(order, simplex\\) labels", duplicate_label)
        )

        bad_id = copy.deepcopy(base)
        bad_id["gamma_cofaces"][0]["coface_id"] = "0" * 64
        mutations.append(("canonical-id", "canonical v2 identifier", bad_id))

        for label, message, mutated in mutations:
            with self.subTest(mutation=label):
                with self.assertRaisesRegex(ContractError, message):
                    validate_result_semantics(mutated)

    def test_gamma_batch_keys_and_assignments_are_unambiguous(self) -> None:
        base = load_json(FIXTURE_DIR / "isolated-birth-k2.json")
        base["run_certificate"]["gamma_complete_by_order"] = [False, False]
        canonicalize_certificate_ids(base)
        validate_result_semantics(base)

        duplicate_assignment = copy.deepcopy(base)
        assigned_batch = next(
            batch
            for batch in duplicate_assignment["equal_level_batches"]
            if batch["gamma_coface_ids"]
        )
        assigned_batch["gamma_coface_ids"].append(
            assigned_batch["gamma_coface_ids"][0]
        )
        with self.assertRaisesRegex(ContractError, "at most one batch"):
            validate_result_semantics(duplicate_assignment)

        duplicate_batch = copy.deepcopy(base)
        copied_batch = copy.deepcopy(duplicate_batch["equal_level_batches"][0])
        copied_batch["batch_id"] = "f" * 64
        copied_batch["gamma_coface_ids"] = []
        duplicate_batch["equal_level_batches"].append(copied_batch)
        with self.assertRaisesRegex(ContractError, "unique \\(order, level\\) keys"):
            validate_result_semantics(duplicate_batch)

    def test_gamma_coface_identity_excludes_schema_versions(self) -> None:
        result = load_json(FIXTURE_DIR / "isolated-birth-k2.json")
        coface = result["gamma_cofaces"][0]
        projection = {
            "schema_version": coface["schema_version"],
            "order": coface["order"],
            "simplex_point_ids": coface["simplex_point_ids"],
            "facet_point_ids": coface["facet_point_ids"],
            "squared_level_exact": coface["squared_level_exact"],
        }
        changed_versions = copy.deepcopy(projection)
        changed_versions["schema_version"] = "999.0.0"
        changed_versions["squared_level_exact"]["schema_version"] = "999.0.0"
        self.assertEqual(
            canonical_contract_id("GammaCoface", projection),
            canonical_contract_id("GammaCoface", changed_versions),
        )

    def test_full_pi0_cannot_be_exact_on_the_conditional_m1_basis(self) -> None:
        mutated = copy.deepcopy(
            self.definitions["MorseHGP3DResult"]["examples"][0]
        )
        mutated["profile"] = "full_pi0"
        for forest in mutated["forests"]:
            forest["profile"] = "full_pi0"
        certificate = mutated["run_certificate"]
        certificate["requested_profile"] = "full_pi0"
        certificate["effective_profile"] = "full_pi0"
        certificate["reconstruction_contract_id"] = "M1-reconstruction-v1"
        certificate["proof_basis"] = "m1_conditional_contract"
        with self.assertRaises(ContractError):
            validate_result_semantics(mutated)

        unavailable_basis = copy.deepcopy(
            self.definitions["MorseHGP3DResult"]["examples"][0]
        )
        unavailable_basis["run_certificate"]["proof_basis"] = "full_pi0_proved"
        with self.assertRaises(ContractError):
            self.validator.check(unavailable_basis)

    def test_exact_hgp_reduced_requires_exhaustive_gamma_on_reference_cpu(self) -> None:
        root = copy.deepcopy(
            self.definitions["MorseHGP3DResult"]["examples"][0]
        )
        validate_result_semantics(root)

        legacy_basis = copy.deepcopy(root)
        legacy_basis["run_certificate"]["proof_basis"] = (
            "reduced_manuscript_theorem_5"
        )
        with self.assertRaises(ContractError):
            self.validator.check(legacy_basis)

        cuda_result = copy.deepcopy(root)
        cuda_result["backend"] = "cuda_g4"
        cuda_result["run_certificate"]["requested_backend"] = "cuda_g4"
        cuda_result["run_certificate"]["effective_backend"] = "cuda_g4"
        with self.assertRaises(ContractError):
            validate_result_semantics(cuda_result)

    def test_raw_gabriel_is_only_a_conditional_partial_refinement(self) -> None:
        partial = load_json(FIXTURE_DIR / "isolated-birth-k2.json")
        partial["profile"] = "hgp_reduced"
        for forest in partial["forests"]:
            forest["profile"] = "hgp_reduced"
        certificate = partial["run_certificate"]
        certificate["requested_profile"] = "hgp_reduced"
        certificate["effective_profile"] = "hgp_reduced"
        certificate["reconstruction_contract_id"] = "hgp-reduced-v2"
        certificate["proof_basis"] = "gabriel_positive_connectivity"
        certificate["partial_guarantees"] = [
            "partial_forest_refines_exact",
            "positive_connectivity",
            "verified_events",
        ]
        partial["partial_scope"]["positive_guarantees"] = list(
            certificate["partial_guarantees"]
        )
        # This test isolates the proof/status lattice; the strict identity graph is
        # covered independently below and would require rekeying every forest node.
        validate_result_semantics(partial, check_canonical_ids=False)

        exact_claim = copy.deepcopy(partial)
        exact_claim["result_kind"] = "hierarchy"
        exact_claim["forest_semantics"] = "exact"
        exact_certificate = exact_claim["run_certificate"]
        exact_certificate["result_kind"] = "hierarchy"
        exact_certificate["forest_semantics"] = "exact"
        exact_certificate["public_status"] = "exact"
        exact_certificate["require_exact"] = True
        exact_certificate["budget_policy"]["require_exact"] = True
        for forest in exact_claim["forests"]:
            forest["forest_semantics"] = "exact"
        with self.assertRaises(ContractError):
            validate_result_semantics(exact_claim, check_canonical_ids=False)

    def test_budgeted_mode_cannot_publish_exact(self) -> None:
        mutated = copy.deepcopy(
            self.definitions["MorseHGP3DResult"]["examples"][0]
        )
        mutated["mode"] = "budgeted"
        certificate = mutated["run_certificate"]
        certificate["requested_mode"] = "budgeted"
        certificate["budget_policy"]["mode"] = "budgeted"
        with self.assertRaises(ContractError):
            validate_result_semantics(mutated)

    def test_exact_payload_completeness_is_fail_closed(self) -> None:
        base = load_json(FIXTURE_DIR / "overlap-k2.json")
        mutations: list[tuple[str, Any]] = []

        incomplete_forest = copy.deepcopy(base)
        incomplete_forest["forests"][0]["complete"] = False
        mutations.append(("forest", incomplete_forest))

        incomplete_events = copy.deepcopy(base)
        incomplete_events["equal_level_batches"][0]["events_complete"] = False
        mutations.append(("batch-events", incomplete_events))

        incomplete_attachments = copy.deepcopy(base)
        incomplete_attachments["equal_level_batches"][0][
            "attachments_complete"
        ] = False
        mutations.append(("batch-attachments", incomplete_attachments))

        partial_batch = copy.deepcopy(base)
        partial_batch["equal_level_batches"][0]["batch_status"] = "partial"
        mutations.append(("batch-status", partial_batch))

        incomplete_vertical = copy.deepcopy(base)
        incomplete_vertical["vertical_maps"][0]["complete"] = False
        mutations.append(("vertical-complete", incomplete_vertical))

        failed_naturality = copy.deepcopy(base)
        failed_naturality["vertical_maps"][0]["naturality_failures"] = 1
        mutations.append(("vertical-naturality", failed_naturality))

        partial_scope = copy.deepcopy(base)
        partial_scope["partial_scope"] = {}
        mutations.append(("partial-scope", partial_scope))

        for label, mutated in mutations:
            with self.subTest(mutation=label):
                with self.assertRaises(ContractError):
                    validate_result_semantics(
                        mutated, check_canonical_ids=False
                    )

    def test_every_scientific_identifier_is_content_addressed(self) -> None:
        base = load_json(FIXTURE_DIR / "overlap-k2.json")
        mutations: list[tuple[str, Any]] = []

        result_id = copy.deepcopy(base)
        result_id["result_id"] = "0" * 64
        mutations.append(("result", result_id))

        certificate_id = copy.deepcopy(base)
        certificate_id["run_certificate"]["certificate_id"] = "0" * 64
        mutations.append(("certificate", certificate_id))

        run_id = copy.deepcopy(base)
        run_id["run_id"] = "0" * 64
        run_id["run_certificate"]["run_id"] = "0" * 64
        mutations.append(("run", run_id))

        snapshot_id = copy.deepcopy(base)
        snapshot_id["run_certificate"]["final_budget_snapshot"][
            "snapshot_id"
        ] = "0" * 64
        mutations.append(("snapshot", snapshot_id))

        event_id = copy.deepcopy(base)
        event_id["critical_catalog"][0]["event_id"] = "0" * 64
        mutations.append(("event", event_id))

        hyperedge_id = copy.deepcopy(base)
        hyperedge_id["gabriel_hyperedges"][0]["hyperedge_id"] = "0" * 64
        mutations.append(("hyperedge", hyperedge_id))

        batch_id = copy.deepcopy(base)
        batch_id["equal_level_batches"][0]["batch_id"] = "0" * 64
        mutations.append(("batch", batch_id))

        node_id = copy.deepcopy(base)
        node_id["forests"][0]["nodes"][0]["node_id"] = "0" * 64
        mutations.append(("node", node_id))

        forest_id = copy.deepcopy(base)
        forest_id["forests"][0]["forest_id"] = "0" * 64
        mutations.append(("forest", forest_id))

        map_id = copy.deepcopy(base)
        map_id["vertical_maps"][0]["map_id"] = "0" * 64
        mutations.append(("vertical-map", map_id))

        for label, mutated in mutations:
            with self.subTest(identifier=label):
                with self.assertRaises(ContractError):
                    validate_result_semantics(mutated)

    def test_every_typed_reference_must_resolve(self) -> None:
        base = load_json(FIXTURE_DIR / "overlap-k2.json")
        unknown = "0" * 64
        mutations: list[tuple[str, Any]] = []

        hyperedge_event = copy.deepcopy(base)
        hyperedge_event["gabriel_hyperedges"][0]["event_id"] = unknown
        mutations.append(("hyperedge-event", hyperedge_event))

        batch_event = copy.deepcopy(base)
        batch = next(
            item for item in batch_event["equal_level_batches"] if item["event_ids"]
        )
        batch["event_ids"][0] = unknown
        mutations.append(("batch-event", batch_event))

        forest_root = copy.deepcopy(base)
        forest_root["forests"][0]["root_ids"][0] = unknown
        mutations.append(("forest-root", forest_root))

        node_child = copy.deepcopy(base)
        merge = next(
            node
            for forest in node_child["forests"]
            for node in forest["nodes"]
            if node["child_ids"]
        )
        merge["child_ids"][0] = unknown
        mutations.append(("node-child", node_child))

        coverage_batch = copy.deepcopy(base)
        coverage_batch["coverage_log"][0]["batch_id"] = unknown
        mutations.append(("coverage-batch", coverage_batch))

        vertical_target = copy.deepcopy(base)
        vertical_target["vertical_maps"][0]["assignments"][0][
            "target_node_id"
        ] = unknown
        mutations.append(("vertical-target", vertical_target))

        materialized_node = copy.deepcopy(base)
        materialized_node["materialized_point_sets"][0]["node_id"] = unknown
        mutations.append(("materialized-node", materialized_node))

        attachment_target = load_json(FIXTURE_DIR / "binary-merge-k2.json")
        attachment_target["attachments"][0]["target_node_id"] = unknown
        mutations.append(("attachment-target", attachment_target))

        for label, mutated in mutations:
            with self.subTest(reference=label):
                with self.assertRaises(ContractError):
                    validate_result_semantics(
                        mutated, check_canonical_ids=False
                    )

    def test_input_events_and_rationals_remain_semantically_canonical(self) -> None:
        base = load_json(FIXTURE_DIR / "overlap-k2.json")
        mutations: list[tuple[str, Any]] = []

        multiplicity = copy.deepcopy(base)
        multiplicity["embedded_input_points"][0]["multiplicity"] += 1
        mutations.append(("multiplicity", multiplicity))

        input_hash = copy.deepcopy(base)
        input_hash["run_certificate"]["input_semantics"]["input_sha256"] = (
            "0" * 64
        )
        mutations.append(("input-hash", input_hash))

        perturbation = copy.deepcopy(base)
        perturbation["run_certificate"]["input_semantics"][
            "perturbation_policy"
        ] = "explicit_symbolic_diagnostic"
        mutations.append(("perturbation", perturbation))

        similarity = copy.deepcopy(base)
        similarity["run_certificate"]["input_semantics"][
            "similarity_transform"
        ]["scale"]["numerator"] = "2"
        mutations.append(("similarity", similarity))

        rank = copy.deepcopy(base)
        rank["critical_catalog"][0]["closed_rank"] += 1
        mutations.append(("closed-rank", rank))

        support = copy.deepcopy(base)
        support["critical_catalog"][0]["minimal_support_ids"] = [
            support["run_certificate"]["input_point_count"] - 1
        ]
        mutations.append(("support", support))

        barycentric = copy.deepcopy(base)
        barycentric["critical_catalog"][0]["barycentric_signs"][0] = -1
        mutations.append(("barycentric", barycentric))

        role = copy.deepcopy(base)
        role["critical_catalog"][0]["morse_roles"][0][
            "local_multiplicity"
        ] += 1
        mutations.append(("morse-role", role))

        predicate = copy.deepcopy(base)
        predicate["critical_catalog"][0]["predicate_status"] = "numeric_failure"
        mutations.append(("predicate", predicate))

        degeneracy = copy.deepcopy(base)
        degeneracy["critical_catalog"][0]["degeneracy_class"] = "unsupported"
        mutations.append(("degeneracy", degeneracy))

        level = copy.deepcopy(base)
        exact_level = level["critical_catalog"][0]["squared_level_exact"]
        exact_level["numerator"] = str(2 * int(exact_level["numerator"]))
        exact_level["denominator"] = str(2 * int(exact_level["denominator"]))
        mutations.append(("exact-level", level))

        center = copy.deepcopy(base)
        exact_center = center["critical_catalog"][0][
            "center_witness_homogeneous"
        ]
        for field in ("x_numerator", "y_numerator", "z_numerator"):
            exact_center[field] = str(2 * int(exact_center[field]))
        exact_center["denominator"] = str(2 * int(exact_center["denominator"]))
        mutations.append(("exact-center", center))

        scale = copy.deepcopy(base)
        exact_scale = scale["run_certificate"]["input_semantics"][
            "similarity_transform"
        ]["scale"]
        exact_scale["numerator"] = "2"
        exact_scale["denominator"] = "2"
        mutations.append(("exact-scale", scale))

        for label, mutated in mutations:
            with self.subTest(invariant=label):
                with self.assertRaises(ContractError):
                    validate_result_semantics(
                        mutated, check_canonical_ids=False
                    )

    def test_exact_payload_objects_cannot_be_orphaned_from_replay(self) -> None:
        base = load_json(FIXTURE_DIR / "overlap-k2.json")
        mutations: list[tuple[str, Any]] = []

        orphan_event = copy.deepcopy(base)
        event_id = next(
            event_id
            for batch in orphan_event["equal_level_batches"]
            for event_id in batch["event_ids"]
        )
        for batch in orphan_event["equal_level_batches"]:
            batch["event_ids"] = [
                candidate for candidate in batch["event_ids"] if candidate != event_id
            ]
        mutations.append(("event", orphan_event))

        orphan_hyperedge = copy.deepcopy(base)
        hyperedge_id = next(
            hyperedge_id
            for batch in orphan_hyperedge["equal_level_batches"]
            for hyperedge_id in batch["hyperedge_ids"]
        )
        for batch in orphan_hyperedge["equal_level_batches"]:
            batch["hyperedge_ids"] = [
                candidate
                for candidate in batch["hyperedge_ids"]
                if candidate != hyperedge_id
            ]
        mutations.append(("hyperedge", orphan_hyperedge))

        missing_coverage = copy.deepcopy(base)
        missing_coverage["coverage_log"].pop()
        mutations.append(("coverage", missing_coverage))

        wrong_materialization = copy.deepcopy(base)
        point_set = wrong_materialization["materialized_point_sets"][0]
        point_set["point_ids"] = []
        mutations.append(("materialization", wrong_materialization))

        wrong_vertical_level = copy.deepcopy(base)
        assignment = wrong_vertical_level["vertical_maps"][0]["assignments"][0]
        assignment["at_squared_level"]["numerator"] = str(
            int(assignment["at_squared_level"]["numerator"]) + 1
        )
        mutations.append(("vertical-level", wrong_vertical_level))

        for label, mutated in mutations:
            with self.subTest(orphan=label):
                with self.assertRaises(ContractError):
                    validate_result_semantics(
                        mutated, check_canonical_ids=False
                    )

    def test_gcp_benchmark_requires_both_cutoffs_and_targeted_shutdown(self) -> None:
        benchmark = copy.deepcopy(self.definitions["BenchmarkRecord"]["examples"][0])
        safety = benchmark["gcp_safety"]
        safety.update(
            {
                "used": True,
                "project": "devpod-gpu-exploration",
                "zone": "europe-west4-a",
                "instance_name": "ehgp-blackwell-spot",
                "machine_type": "g4-standard-48",
                "provisioning_model": "SPOT",
                "instance_termination_action": "STOP",
                "max_run_duration_s": 3600,
                "guest_shutdown_armed": True,
                "initial_state": "TERMINATED",
                "final_state": "TERMINATED",
            }
        )
        validate_benchmark_semantics(benchmark)
        safety["guest_shutdown_armed"] = False
        with self.assertRaises(ContractError):
            validate_benchmark_semantics(benchmark)


if __name__ == "__main__":
    unittest.main()
