from __future__ import annotations

import copy
import sys
import unittest
from pathlib import Path
from typing import Any, Iterator


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from check_contracts import (  # noqa: E402
    REQUIRED_DEFINITIONS,
    SCHEMA_PATH,
    ContractError,
    SchemaValidator,
    canonical_json,
    load_json,
    validate_benchmark_semantics,
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
            "batches_complete_by_order",
        ):
            self.assertTrue(certificate[field])
            mutated = copy.deepcopy(root)
            mutated["run_certificate"][field][0] = False
            with self.subTest(gate=field):
                with self.assertRaises(ContractError):
                    validate_result_semantics(mutated)

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
