#!/usr/bin/env python3
"""Validate the versioned MorseHGP3D JSON contract without third-party code."""

from __future__ import annotations

import hashlib
import json
import math
import re
import sys
from collections import Counter
from fractions import Fraction
from itertools import combinations
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
SCHEMA_PATH = ROOT / "schemas" / "morsehgp3d-contract-v2.schema.json"
LEGACY_SCHEMA_PATH = ROOT / "schemas" / "morsehgp3d-contract-v1.schema.json"
FIXTURE_DIR = ROOT / "tests" / "fixtures" / "contracts"
CONTRACT_DOMAIN = "MorseHGP3D/v2"

REQUIRED_DEFINITIONS = {
    "InputSemantics",
    "CertifiedPoint3",
    "ExactRational3",
    "ExactLevel",
    "VertexWitness",
    "CriticalEvent",
    "GammaCoface",
    "GabrielHyperedge",
    "Attachment",
    "EqualLevelBatch",
    "MergeForest",
    "VerticalMap",
    "MorseHGP3DResult",
    "RunCertificate",
    "PartialScope",
    "FragmentHint",
    "CanonicalCellCertificate",
    "BudgetPolicy",
    "BudgetSnapshot",
    "CheckpointManifest",
    "BenchmarkRecord",
}


class ContractError(ValueError):
    """Raised when a schema or contract instance violates the active v2 rules."""


def _object_without_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ContractError(f"duplicate JSON key {key!r}")
        result[key] = value
    return result


def load_json(path: Path) -> Any:
    try:
        return json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=_object_without_duplicate_keys,
        )
    except (OSError, json.JSONDecodeError, ContractError) as exc:
        raise ContractError(f"{path.relative_to(ROOT)}: {exc}") from exc


def canonical_json(value: Any) -> str:
    """Return the byte-independent canonical JSON text used by phase-0 tests."""

    return json.dumps(
        value,
        ensure_ascii=False,
        allow_nan=False,
        sort_keys=True,
        separators=(",", ":"),
    )


def _without_schema_versions(value: Any) -> Any:
    """Remove record-version metadata from a scientific identity projection."""

    if isinstance(value, dict):
        return {
            key: _without_schema_versions(child)
            for key, child in value.items()
            if key != "schema_version"
        }
    if isinstance(value, (list, tuple)):
        return [_without_schema_versions(child) for child in value]
    return value


def canonical_contract_id(record_type: str, identity_projection: Any) -> str:
    """Return a v2 domain-separated identifier for an identity projection."""

    if not record_type or "/" in record_type:
        raise ContractError("record_type must be a non-empty path component")
    digest = hashlib.sha256()
    digest.update(f"{CONTRACT_DOMAIN}/{record_type}/".encode("ascii"))
    digest.update(
        canonical_json(_without_schema_versions(identity_projection)).encode("utf-8")
    )
    return digest.hexdigest()


def _exact_level_key(level: dict[str, Any]) -> Fraction:
    """Return the mathematical value of an exact squared-level record."""

    return Fraction(int(level["numerator"]), int(level["denominator"]))


def _is_type(value: Any, expected: str) -> bool:
    if expected == "null":
        return value is None
    if expected == "boolean":
        return isinstance(value, bool)
    if expected == "integer":
        return isinstance(value, int) and not isinstance(value, bool)
    if expected == "number":
        return (
            isinstance(value, (int, float))
            and not isinstance(value, bool)
            and math.isfinite(value)
        )
    if expected == "string":
        return isinstance(value, str)
    if expected == "array":
        return isinstance(value, list)
    if expected == "object":
        return isinstance(value, dict)
    raise ContractError(f"unsupported schema type {expected!r}")


class SchemaValidator:
    """Small Draft 2020-12 subset sufficient for the frozen local contract."""

    def __init__(self, schema: dict[str, Any]) -> None:
        self.schema = schema

    def resolve_ref(self, ref: str) -> Any:
        if not ref.startswith("#/"):
            raise ContractError(f"external schema reference is forbidden: {ref!r}")
        target: Any = self.schema
        for raw_part in ref[2:].split("/"):
            part = raw_part.replace("~1", "/").replace("~0", "~")
            if not isinstance(target, dict) or part not in target:
                raise ContractError(f"unresolved schema reference {ref!r}")
            target = target[part]
        return target

    def check(self, instance: Any, schema: Any | None = None, path: str = "$") -> None:
        current = self.schema if schema is None else schema
        if current is True:
            return
        if current is False:
            raise ContractError(f"{path}: value rejected by false schema")
        if not isinstance(current, dict):
            raise ContractError(f"{path}: schema node is not an object or boolean")

        if "$ref" in current:
            self.check(instance, self.resolve_ref(current["$ref"]), path)

        if "allOf" in current:
            for index, branch in enumerate(current["allOf"]):
                self.check(instance, branch, f"{path}.allOf[{index}]")
        if "anyOf" in current:
            matches = self._matching_branches(instance, current["anyOf"], path)
            if not matches:
                raise ContractError(f"{path}: value matches no anyOf branch")
        if "oneOf" in current:
            matches = self._matching_branches(instance, current["oneOf"], path)
            if len(matches) != 1:
                raise ContractError(
                    f"{path}: value must match exactly one oneOf branch, got {len(matches)}"
                )
        if "not" in current:
            try:
                self.check(instance, current["not"], path)
            except ContractError:
                pass
            else:
                raise ContractError(f"{path}: value matches forbidden schema")
        if "if" in current:
            try:
                self.check(instance, current["if"], path)
            except ContractError:
                branch = current.get("else")
            else:
                branch = current.get("then")
            if branch is not None:
                self.check(instance, branch, path)

        if "const" in current and instance != current["const"]:
            raise ContractError(f"{path}: expected constant {current['const']!r}")
        if "enum" in current and instance not in current["enum"]:
            raise ContractError(f"{path}: {instance!r} is not in {current['enum']!r}")

        declared_type = current.get("type")
        if declared_type is not None:
            expected_types = (
                declared_type if isinstance(declared_type, list) else [declared_type]
            )
            if not any(_is_type(instance, expected) for expected in expected_types):
                raise ContractError(
                    f"{path}: expected type {expected_types!r}, got {type(instance).__name__}"
                )

        if isinstance(instance, dict):
            self._check_object(instance, current, path)
        elif isinstance(instance, list):
            self._check_array(instance, current, path)
        elif isinstance(instance, str):
            self._check_string(instance, current, path)
        elif isinstance(instance, (int, float)) and not isinstance(instance, bool):
            self._check_number(instance, current, path)

    def _matching_branches(
        self, instance: Any, branches: list[Any], path: str
    ) -> list[int]:
        matches: list[int] = []
        for index, branch in enumerate(branches):
            try:
                self.check(instance, branch, path)
            except ContractError:
                continue
            matches.append(index)
        return matches

    def _check_object(
        self, instance: dict[str, Any], schema: dict[str, Any], path: str
    ) -> None:
        required = schema.get("required", [])
        missing = [key for key in required if key not in instance]
        if missing:
            raise ContractError(f"{path}: missing required properties {missing!r}")

        properties = schema.get("properties", {})
        pattern_properties = schema.get("patternProperties", {})
        for key, value in instance.items():
            matched = False
            if key in properties:
                self.check(value, properties[key], f"{path}.{key}")
                matched = True
            for pattern, subschema in pattern_properties.items():
                if re.search(pattern, key):
                    self.check(value, subschema, f"{path}.{key}")
                    matched = True
            if matched:
                continue
            additional = schema.get("additionalProperties", True)
            if additional is False:
                raise ContractError(f"{path}: unknown property {key!r}")
            if isinstance(additional, dict):
                self.check(value, additional, f"{path}.{key}")

        if len(instance) < schema.get("minProperties", 0):
            raise ContractError(f"{path}: too few properties")
        if "maxProperties" in schema and len(instance) > schema["maxProperties"]:
            raise ContractError(f"{path}: too many properties")

    def _check_array(
        self, instance: list[Any], schema: dict[str, Any], path: str
    ) -> None:
        if len(instance) < schema.get("minItems", 0):
            raise ContractError(f"{path}: too few items")
        if "maxItems" in schema and len(instance) > schema["maxItems"]:
            raise ContractError(f"{path}: too many items")
        if schema.get("uniqueItems"):
            encoded = [canonical_json(value) for value in instance]
            if len(encoded) != len(set(encoded)):
                raise ContractError(f"{path}: array items must be unique")
        canonical_order = schema.get("x-canonical-order")
        if canonical_order is not None:
            self._check_canonical_order(instance, canonical_order, path)

        prefix_items = schema.get("prefixItems", [])
        for index, subschema in enumerate(prefix_items):
            if index < len(instance):
                self.check(instance[index], subschema, f"{path}[{index}]")
        items = schema.get("items")
        if items is not None:
            start = len(prefix_items) if prefix_items else 0
            for index in range(start, len(instance)):
                self.check(instance[index], items, f"{path}[{index}]")

    @staticmethod
    def _check_canonical_order(
        instance: list[Any], rule: str, path: str
    ) -> None:
        if rule == "ascending":
            keys = instance
        elif rule == "lexicographic":
            keys = [canonical_json(value) for value in instance]
        elif isinstance(rule, str) and rule.startswith("by:"):
            field = rule[3:]
            if not field or any(
                not isinstance(value, dict) or field not in value for value in instance
            ):
                raise ContractError(
                    f"{path}: canonical order {rule!r} requires field {field!r}"
                )
            keys = [value[field] for value in instance]
        else:
            raise ContractError(f"{path}: unsupported x-canonical-order {rule!r}")
        try:
            ordered = all(left <= right for left, right in zip(keys, keys[1:]))
        except TypeError as exc:
            raise ContractError(
                f"{path}: values cannot be compared for canonical order {rule!r}"
            ) from exc
        if not ordered:
            raise ContractError(
                f"{path}: items do not follow canonical order {rule!r}"
            )

    @staticmethod
    def _check_string(instance: str, schema: dict[str, Any], path: str) -> None:
        if len(instance) < schema.get("minLength", 0):
            raise ContractError(f"{path}: string is too short")
        if "maxLength" in schema and len(instance) > schema["maxLength"]:
            raise ContractError(f"{path}: string is too long")
        if "pattern" in schema and re.search(schema["pattern"], instance) is None:
            raise ContractError(
                f"{path}: string {instance!r} does not match {schema['pattern']!r}"
            )

    @staticmethod
    def _check_number(
        instance: int | float, schema: dict[str, Any], path: str
    ) -> None:
        if "minimum" in schema and instance < schema["minimum"]:
            raise ContractError(f"{path}: value is below minimum")
        if "maximum" in schema and instance > schema["maximum"]:
            raise ContractError(f"{path}: value is above maximum")
        if "exclusiveMinimum" in schema and instance <= schema["exclusiveMinimum"]:
            raise ContractError(f"{path}: value is not above exclusiveMinimum")
        if "exclusiveMaximum" in schema and instance >= schema["exclusiveMaximum"]:
            raise ContractError(f"{path}: value is not below exclusiveMaximum")
        if "multipleOf" in schema:
            quotient = instance / schema["multipleOf"]
            if not math.isclose(quotient, round(quotient), rel_tol=0.0, abs_tol=1e-12):
                raise ContractError(f"{path}: value is not a multipleOf")


def validate_round_trip(value: Any, label: str) -> None:
    encoded = canonical_json(value)
    decoded = json.loads(encoded, object_pairs_hook=_object_without_duplicate_keys)
    if decoded != value:
        raise ContractError(f"{label}: canonical JSON round-trip changed the value")
    if canonical_json(decoded) != encoded:
        raise ContractError(f"{label}: canonical JSON serialization is unstable")


def validate_certificate_semantics(certificate: dict[str, Any]) -> None:
    """Check cross-field guarantees that JSON Schema cannot express readably."""

    n = certificate["input_point_count"]
    k_eff = certificate["k_eff"]
    expected_k_eff = min(certificate["k_max"], n)
    if k_eff != expected_k_eff:
        raise ContractError(
            f"run_certificate.k_eff={k_eff} but min(k_max,n)={expected_k_eff}"
        )
    expected_s_max = min(k_eff + 1, n)
    if certificate["s_max"] != expected_s_max:
        raise ContractError(
            "run_certificate.s_max does not equal min(k_eff + 1, input_point_count)"
        )
    expected_m_star = 0 if n == 1 else min(k_eff - 1, n - 2)
    if certificate["m_star"] != expected_m_star:
        raise ContractError(
            "run_certificate.m_star does not match the frozen shallow-depth rule"
        )

    expected_lengths = {
        "catalog_complete_by_rank": expected_s_max,
        "attachments_complete_by_order": k_eff,
        "gamma_complete_by_order": k_eff,
        "batches_complete_by_order": k_eff,
    }
    for field, expected in expected_lengths.items():
        if len(certificate[field]) != expected:
            raise ContractError(
                f"run_certificate.{field} must contain {expected} positional entries"
            )

    mode = certificate["requested_mode"]
    status = certificate["public_status"]
    semantics = certificate.get("forest_semantics")
    proof_basis = certificate["proof_basis"]
    profile = certificate["effective_profile"]

    exact_markers = {
        "forest_semantics=exact": semantics == "exact",
        "public_status=exact": status == "exact",
        "result_kind=hierarchy": certificate["result_kind"] == "hierarchy",
    }
    if len(set(exact_markers.values())) != 1:
        raise ContractError(
            "exact forest semantics, exact public status and hierarchy result kind "
            "must be equivalent"
        )
    if any(exact_markers.values()) and not certificate["require_exact"]:
        raise ContractError("exact hierarchy publication requires require_exact=true")

    if certificate["budget_policy"]["mode"] != mode:
        raise ContractError("budget_policy.mode must match requested_mode")
    if certificate["budget_policy"]["require_exact"] != certificate["require_exact"]:
        raise ContractError("budget_policy.require_exact must match run certificate")
    if mode == "budgeted" and status == "exact":
        raise ContractError("a budgeted run can never publish public_status=exact")
    if semantics == "partial_refinement" and certificate["require_exact"]:
        raise ContractError("partial_refinement is incompatible with require_exact=true")
    if semantics == "partial_refinement" and status not in {
        "conditional",
        "budget_exhausted",
    }:
        raise ContractError("partial_refinement requires a conditional or budget status")

    if proof_basis == "gabriel_positive_connectivity":
        if profile != "hgp_reduced":
            raise ContractError(
                "gabriel_positive_connectivity is reserved for hgp_reduced"
            )
        if certificate["result_kind"] != "partial_hierarchy":
            raise ContractError(
                "gabriel_positive_connectivity requires result_kind=partial_hierarchy"
            )
        if semantics != "partial_refinement":
            raise ContractError(
                "gabriel_positive_connectivity requires partial_refinement semantics"
            )
        if certificate["require_exact"]:
            raise ContractError(
                "gabriel_positive_connectivity is incompatible with require_exact=true"
            )
        if status not in {"conditional", "budget_exhausted"}:
            raise ContractError(
                "gabriel_positive_connectivity permits only conditional or budget status"
            )
        if "positive_connectivity" not in certificate["partial_guarantees"]:
            raise ContractError(
                "gabriel_positive_connectivity requires its positive guarantee"
            )

    if profile == "full_pi0" and semantics == "exact":
        raise ContractError("the active v2 contract cannot publish full_pi0 exact")
    if status == "exact":
        if certificate["result_kind"] != "hierarchy":
            raise ContractError("public_status=exact requires a hierarchy result")
        if semantics != "exact":
            raise ContractError("public_status=exact requires forest_semantics=exact")
        if certificate["stop_reason"] != "completed":
            raise ContractError("public_status=exact requires stop_reason=completed")
        if certificate["general_position_status"] not in {
            "verified_relevant",
            "not_applicable",
        }:
            raise ContractError("public_status=exact requires a closed input domain")
        scalar_gates = (
            "relevant_gp_complete",
            "canonical_children_complete",
            "active_cross_incidences_complete",
            "vertical_maps_complete",
        )
        if not all(certificate[field] for field in scalar_gates):
            raise ContractError("public_status=exact has a false completeness gate")
        vector_gates = (
            "catalog_complete_by_rank",
            "attachments_complete_by_order",
            "gamma_complete_by_order",
            "batches_complete_by_order",
        )
        if not all(all(certificate[field]) for field in vector_gates):
            raise ContractError("public_status=exact has an incomplete positional gate")
        if certificate["fallback_counts"]["unsupported_degeneracies"] != 0:
            raise ContractError("public_status=exact has unsupported degeneracies")
        if certificate["fallback_counts"]["numeric_failures"] != 0:
            raise ContractError("public_status=exact has numeric failures")
        if certificate["exact_predicate_counts"]["remaining_unknown"] != 0:
            raise ContractError("public_status=exact has unresolved exact predicates")
        if certificate["unresolved_locus_ids"]:
            raise ContractError("public_status=exact has unresolved loci")
        if profile != "hgp_reduced":
            raise ContractError("the active v2 contract cannot publish full_pi0 exact")
        expected_basis = "gamma_exhaustive_reference"
        if proof_basis != expected_basis:
            raise ContractError(
                f"public_status=exact for {profile} requires proof_basis={expected_basis}"
            )
        if profile == "hgp_reduced" and certificate["effective_backend"] != "reference_cpu":
            raise ContractError(
                "exact hgp_reduced with gamma_exhaustive_reference is restricted to reference_cpu"
            )

    expected_contract = {
        "hgp_reduced": "hgp-reduced-v2",
        "full_pi0": "M1-reconstruction-v1",
    }[profile]
    if certificate["reconstruction_contract_id"] != expected_contract:
        raise ContractError(
            f"{profile} requires reconstruction_contract_id={expected_contract}"
        )
    allowed_bases = {
        "hgp_reduced": {
            "gamma_exhaustive_reference",
            "gabriel_positive_connectivity",
        },
        "full_pi0": {"m1_conditional_contract"},
    }[profile]
    if proof_basis not in allowed_bases:
        raise ContractError(f"proof_basis={proof_basis} is invalid for {profile}")

    budget_reasons = {
        "device_budget",
        "host_budget",
        "scratch_budget",
        "output_budget",
        "time_budget",
        "preempted",
        "user_cancelled",
    }
    if status == "budget_exhausted" and certificate["stop_reason"] not in budget_reasons:
        raise ContractError("budget_exhausted requires an explicit budget-like stop reason")
    if status == "unsupported_degeneracy" and certificate["stop_reason"] != status:
        raise ContractError("unsupported_degeneracy status and stop reason must agree")
    if status == "numeric_failure" and certificate["stop_reason"] != status:
        raise ContractError("numeric_failure status and stop reason must agree")


def validate_result_semantics(result: dict[str, Any]) -> None:
    certificate = result["run_certificate"]
    validate_certificate_semantics(certificate)

    equal_fields = {
        "run_id": "run_id",
        "result_kind": "result_kind",
        "backend": "effective_backend",
        "profile": "effective_profile",
        "mode": "requested_mode",
        "k_eff": "k_eff",
    }
    for result_field, certificate_field in equal_fields.items():
        if result[result_field] != certificate[certificate_field]:
            raise ContractError(
                f"result.{result_field} must match run_certificate.{certificate_field}"
            )
    if result.get("forest_semantics") != certificate.get("forest_semantics"):
        raise ContractError("result and certificate forest_semantics must agree")

    n = certificate["input_point_count"]
    k_eff = certificate["k_eff"]
    cofaces = result["gamma_cofaces"]
    coface_by_id: dict[str, dict[str, Any]] = {}
    coface_labels: set[tuple[int, tuple[int, ...]]] = set()
    for coface in cofaces:
        coface_id = coface["coface_id"]
        if coface_id in coface_by_id:
            raise ContractError("gamma_cofaces must have unique coface_id values")

        order = coface["order"]
        if not isinstance(order, int) or isinstance(order, bool) or not 1 <= order <= k_eff:
            raise ContractError("a Gamma coface order is outside 1..k_eff")
        simplex = tuple(coface["simplex_point_ids"])
        if len(simplex) != order + 1:
            raise ContractError("a Gamma coface simplex has the wrong cardinality")
        if any(
            not isinstance(point_id, int)
            or isinstance(point_id, bool)
            or point_id < 0
            or point_id >= n
            for point_id in simplex
        ):
            raise ContractError("a Gamma coface simplex has an out-of-range point ID")
        if list(simplex) != sorted(set(simplex)):
            raise ContractError("a Gamma coface simplex is not a canonical point label")

        identity = (order, simplex)
        if identity in coface_labels:
            raise ContractError("Gamma cofaces must have unique (order, simplex) labels")
        coface_labels.add(identity)

        expected_facets = sorted(
            [list(facet) for facet in combinations(simplex, order)],
            key=canonical_json,
        )
        if coface["facet_point_ids"] != expected_facets:
            raise ContractError("a Gamma coface does not list all canonical facets")

        identity_projection = {
            "schema_version": coface["schema_version"],
            "order": order,
            "simplex_point_ids": list(simplex),
            "facet_point_ids": coface["facet_point_ids"],
            "squared_level_exact": coface["squared_level_exact"],
        }
        expected_coface_id = canonical_contract_id(
            "GammaCoface", identity_projection
        )
        if coface_id != expected_coface_id:
            raise ContractError("a Gamma coface_id is not its canonical v2 identifier")
        coface_by_id[coface_id] = coface

    if certificate["work_and_memory_counters"]["gamma_cofaces_emitted"] != len(
        cofaces
    ):
        raise ContractError("gamma_cofaces_emitted must equal the serialized count")

    batch_keys: set[tuple[int, Fraction]] = set()
    assigned_gamma_ids: list[str] = []
    for batch in result["equal_level_batches"]:
        batch_order = batch["order"]
        if (
            not isinstance(batch_order, int)
            or isinstance(batch_order, bool)
            or not 1 <= batch_order <= k_eff
        ):
            raise ContractError("an equal-level batch order is outside 1..k_eff")
        batch_level = batch["squared_level_exact"]
        batch_key = (batch_order, _exact_level_key(batch_level))
        if batch_key in batch_keys:
            raise ContractError("equal-level batches must have unique (order, level) keys")
        batch_keys.add(batch_key)
        for coface_id in batch["gamma_coface_ids"]:
            coface = coface_by_id.get(coface_id)
            if coface is None:
                raise ContractError("a batch references an unknown Gamma coface")
            if coface["order"] != batch_order:
                raise ContractError("a Gamma coface is assigned to the wrong order")
            if _exact_level_key(coface["squared_level_exact"]) != batch_key[1]:
                raise ContractError("a Gamma coface is assigned to the wrong exact level")
            assigned_gamma_ids.append(coface_id)

    counts = Counter(assigned_gamma_ids)
    if any(count > 1 for count in counts.values()):
        raise ContractError("a Gamma coface may occur in at most one batch")
    for order in range(1, certificate["k_eff"] + 1):
        if certificate["gamma_complete_by_order"][order - 1]:
            observed_labels = {
                simplex
                for coface_order, simplex in coface_labels
                if coface_order == order
            }
            expected_count = math.comb(n, order + 1) if order < n else 0
            if len(observed_labels) != expected_count:
                raise ContractError(
                    f"complete Gamma order {order} is missing or inventing cofaces"
                )
            if any(
                counts[coface["coface_id"]] != 1
                for coface in cofaces
                if coface["order"] == order
            ):
                raise ContractError(
                    "a complete Gamma coface must occur in exactly one batch"
                )

    result_kind = result["result_kind"]
    if result_kind == "hierarchy":
        if "forest_semantics" not in result:
            raise ContractError("a hierarchy result must declare forest_semantics")
        if len(result["forests"]) != result["k_eff"]:
            raise ContractError("a hierarchy result must contain one forest per order")
        if len(result["vertical_maps"]) != max(0, result["k_eff"] - 1):
            raise ContractError("a hierarchy result must contain one adjacent vertical map")
    elif result_kind == "partial_hierarchy":
        if result.get("forest_semantics") != "partial_refinement":
            raise ContractError("partial_hierarchy requires partial_refinement semantics")
        if "partial_scope" not in result:
            raise ContractError("partial_hierarchy requires partial_scope")
    elif result_kind == "verified_events_only":
        if "forest_semantics" in result:
            raise ContractError("verified_events_only must omit forest_semantics")
        if "partial_scope" not in result:
            raise ContractError("verified_events_only requires partial_scope")

    forest_orders = [forest["order"] for forest in result["forests"]]
    if result_kind == "hierarchy":
        expected_orders = list(range(1, result["k_eff"] + 1))
        if forest_orders != expected_orders:
            raise ContractError("forests must be canonically ordered from 1 through k_eff")
    elif forest_orders != sorted(set(forest_orders)) or any(
        order < 1 or order > result["k_eff"] for order in forest_orders
    ):
        raise ContractError("partial forests must have sorted unique valid orders")
    for forest in result["forests"]:
        if forest["profile"] != result["profile"]:
            raise ContractError("every forest profile must match the result profile")
        if forest["forest_semantics"] != result.get("forest_semantics"):
            raise ContractError("every forest semantic must match the result semantic")

    source_orders = [vertical_map["source_order"] for vertical_map in result["vertical_maps"]]
    if result_kind == "hierarchy":
        expected_sources = list(range(2, result["k_eff"] + 1))
        if source_orders != expected_sources:
            raise ContractError("vertical maps must be ordered by source order 2 through k_eff")
    elif source_orders != sorted(set(source_orders)):
        raise ContractError("partial vertical maps must have sorted unique source orders")
    for vertical_map in result["vertical_maps"]:
        if vertical_map["target_order"] != vertical_map["source_order"] - 1:
            raise ContractError("vertical maps must target the adjacent lower order")


def validate_benchmark_semantics(record: dict[str, Any]) -> None:
    safety = record["gcp_safety"]
    target_fields = (
        "project",
        "zone",
        "instance_name",
        "machine_type",
        "provisioning_model",
        "instance_termination_action",
        "max_run_duration_s",
    )
    if safety["used"]:
        if any(safety[field] is None for field in target_fields):
            raise ContractError("a GCP benchmark must identify its exact guarded target")
        expected = {
            "machine_type": "g4-standard-48",
            "provisioning_model": "SPOT",
            "instance_termination_action": "STOP",
            "guest_shutdown_armed": True,
            "final_state": "TERMINATED",
        }
        if any(safety[field] != value for field, value in expected.items()):
            raise ContractError("GCP benchmark safety evidence is not fail-closed")
        if not 30 <= safety["max_run_duration_s"] <= 28_800:
            raise ContractError("GCP max run duration must be between 30s and 8h")
    else:
        if any(safety[field] is not None for field in target_fields):
            raise ContractError("an unused GCP record must not claim a target")
        if safety["guest_shutdown_armed"]:
            raise ContractError("an unused GCP record cannot claim a guest guard")
        if safety["initial_state"] != "NOT_USED" or safety["final_state"] != "NOT_USED":
            raise ContractError("an unused GCP record must use NOT_USED states")


def validate_repository_contract() -> tuple[int, int]:
    schema = load_json(SCHEMA_PATH)
    if not isinstance(schema, dict):
        raise ContractError("root schema must be an object")
    if schema.get("$schema") != "https://json-schema.org/draft/2020-12/schema":
        raise ContractError("contract must declare JSON Schema Draft 2020-12")
    if schema.get("$id") != (
        "https://e-hgp.invalid/schemas/morsehgp3d-contract-v2.schema.json"
    ):
        raise ContractError("the active contract must declare the v2 schema identifier")
    definitions = schema.get("$defs")
    if not isinstance(definitions, dict):
        raise ContractError("contract must contain an object-valued $defs")
    missing = sorted(REQUIRED_DEFINITIONS - definitions.keys())
    if missing:
        raise ContractError(f"contract is missing required definitions: {missing!r}")
    if definitions.get("SchemaVersion", {}).get("const") != "2.0.0":
        raise ContractError("the active contract must require schema_version=2.0.0")

    legacy_schema = load_json(LEGACY_SCHEMA_PATH)
    legacy_definitions = legacy_schema.get("$defs", {})
    if legacy_schema.get("$id") != (
        "https://e-hgp.invalid/schemas/morsehgp3d-contract-v1.schema.json"
    ) or legacy_definitions.get("SchemaVersion", {}).get("const") != "1.0.0":
        raise ContractError("the archived v1 schema must remain identifiable and immutable")

    validator = SchemaValidator(schema)
    sample_count = 0
    for name in sorted(REQUIRED_DEFINITIONS):
        definition = definitions[name]
        examples = definition.get("examples") if isinstance(definition, dict) else None
        if not isinstance(examples, list) or not examples:
            raise ContractError(f"$defs/{name}: at least one example is required")
        for index, example in enumerate(examples):
            validator.check(example, definition, f"$defs.{name}.examples[{index}]")
            validate_round_trip(example, f"$defs/{name}/examples/{index}")
            if name == "RunCertificate":
                validate_certificate_semantics(example)
            elif name == "MorseHGP3DResult":
                validate_result_semantics(example)
            elif name == "BenchmarkRecord":
                validate_benchmark_semantics(example)
            sample_count += 1

    fixture_count = 0
    if FIXTURE_DIR.is_dir():
        for path in sorted(FIXTURE_DIR.glob("*.json")):
            fixture = load_json(path)
            validator.check(fixture, path=f"fixture:{path.name}")
            validate_result_semantics(fixture)
            validate_round_trip(fixture, str(path.relative_to(ROOT)))
            fixture_count += 1
    return sample_count, fixture_count


def main() -> int:
    try:
        samples, fixtures = validate_repository_contract()
    except ContractError as exc:
        print(f"Contract validation failed: {exc}", file=sys.stderr)
        return 1
    print(
        f"Validated {len(REQUIRED_DEFINITIONS)} contract definitions, "
        f"{samples} schema examples and {fixtures} result fixtures."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
